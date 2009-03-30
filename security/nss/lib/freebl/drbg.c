/*
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 *
 * The Initial Developer this code is Red Hat, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * 
 * Portions creaated by Netscape Communications Corporation.
 * are Copyright (C) 1994-2000 by Netscape Communications Corporation
 * All Rights Reserved.
 *
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
/* $Id: drbg.c,v 1.6 2009/03/30 19:21:08 wtc%google.com Exp $ */

#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerror.h"
#include "secerr.h"

#include "prtypes.h"
#include "prinit.h"
#include "blapi.h"
#include "blapii.h"
#include "nssilock.h"
#include "secitem.h"
#include "sha_fast.h"
#include "sha256.h"
#include "secrng.h"	/* for RNG_SystemRNG() */
#include "secmpi.h"

/* PRNG_SEEDLEN defined in NIST SP 800-90 section 10.1 
 * for SHA-1, SHA-224, and SHA-256 it's 440 bits.
 * for SHA-384 and SHA-512 it's 888 bits */
#define PRNG_SEEDLEN      (440/PR_BITS_PER_BYTE)
static const PRInt64 PRNG_MAX_ADDITIONAL_BYTES = LL_INIT(0x1, 0x0);
						/* 2^35 bits or 2^32 bytes */
#define PRNG_MAX_REQUEST_SIZE 0x10000		/* 2^19 bits or 2^16 bytes */
#define PRNG_ADDITONAL_DATA_CACHE_SIZE (8*1024) /* must be less than
						 *  PRNG_MAX_ADDITIONAL_BYTES
						 */


/* RESEED_COUNT is how many calls to the prng before we need to reseed 
 * under normal NIST rules, you must return an error. In the NSS case, we
 * self-reseed with RNG_SystemRNG(). Count can be a large number. For code
 * simplicity, we specify count with 2 components: RESEED_BYTE (which is 
 * the same as LOG256(RESEED_COUNT)) and RESEED_VALUE (which is the same as
 * RESEED_COUNT / (256 ^ RESEED_BYTE)). Another way to look at this is
 * RESEED_COUNT = RESEED_VALUE * (256 ^ RESEED_BYTE). For Hash based DRBG
 * we use the maximum count value, 2^48, or RESEED_BYTE=6 and RESEED_VALUE=1
 */
#define RESEED_BYTE 6
#define RESEED_VALUE 1

#define PRNG_RESET_RESEED_COUNT(rng) \
	PORT_Memset((rng)->reseed_counter, 0, sizeof (rng)->reseed_counter); \
	(rng)->reseed_counter[RESEED_BYTE] = 1;


/*
 * The actual values of this enum are specified in SP 800-90, 10.1.1.*
 * The spec does not name the types, it only uses bare values 
 */
typedef enum {
   prngCGenerateType = 0,   	/* used when creating a new 'C' */
   prngReseedType = 1,	    	/* used in reseeding */
   prngAdditionalDataType = 2,  /* used in mixing additional data */
   prngGenerateByteType = 3	/* used when mixing internal state while
				 * generating bytes */
} prngVTypes;

/*
 * Global RNG context
 */ 
struct RNGContextStr {
    PZLock   *lock;        /* Lock to serialize access to global rng */
    /*
     * NOTE, a number of steps in the drbg algorithm need to hash 
     * V_type || V. The code, therefore, depends on the V array following 
     * immediately after V_type to avoid extra copies. To accomplish this
     * in a way that compiliers can't perturb, we declare V_type and V
     * as a V_Data array and reference them by macros */
    PRUint8  V_Data[PRNG_SEEDLEN+1]; /* internal state variables */
#define  V_type  V_Data[0]
#define  V(rng)       (((rng)->V_Data)+1)
#define  VSize(rng)   ((sizeof (rng)->V_Data) -1)
    PRUint8  C[PRNG_SEEDLEN];        /* internal state variables */
    PRUint8  oldV[PRNG_SEEDLEN];     /* for continuous rng checking */
    /* If we get calls for the PRNG to return less than the length of our
     * hash, we extend the request for a full hash (since we'll be doing
     * the full hash anyway). Future requests for random numbers are fulfilled
     * from the remainder of the bytes we generated. Requests for bytes longer
     * than the hash size are fulfilled directly from the HashGen function
     * of the random number generator. */
    PRUint8  reseed_counter[RESEED_BYTE+1]; /* number of requests since the 
					     * last reseed. Need only be
					     * big enough to hold the whole
					     * reseed count */
    PRUint8  data[SHA256_LENGTH];	/* when we request less than a block
					 * save the rest of the rng output for 
					 * another partial block */
    PRUint8  dataAvail;            /* # bytes of output available in our cache,
	                            * [0...SHA256_LENGTH] */
    /* store additional data that has been shovelled off to us by
     * RNG_RandomUpdate. */
    PRUint8  additionalDataCache[PRNG_ADDITONAL_DATA_CACHE_SIZE];
    PRUint32 additionalAvail;
    PRBool   isValid;          /* false if RNG reaches an invalid state */
};

typedef struct RNGContextStr RNGContext;
static RNGContext *globalrng = NULL;
static RNGContext theGlobalRng;


/*
 * The next several functions are derived from the NIST SP 800-90
 * spec. In these functions, an attempt was made to use names consistent
 * with the names in the spec, even if they differ from normal NSS usage.
 */

/*
 * Hash Derive function defined in NISP SP 800-90 Section 10.4.1.
 * This function is used in the Instantiate and Reseed functions.
 * 
 * NOTE: requested_bytes cannot overlap with input_string_1 or input_string_2.
 * input_string_1 and input_string_2 are logically concatentated. 
 * input_string_1 must be supplied.
 * if input_string_2 is not supplied, NULL should be passed for this parameter.
 */
static SECStatus
prng_Hash_df(PRUint8 *requested_bytes, unsigned int no_of_bytes_to_return, 
	const PRUint8 *input_string_1, unsigned int input_string_1_len, 
	const PRUint8 *input_string_2, unsigned int input_string_2_len)
{
    SHA256Context ctx;
    PRUint32 tmp;
    PRUint8 counter;

    tmp=SHA_HTONL(no_of_bytes_to_return*8);

    for (counter = 1 ; no_of_bytes_to_return > 0; counter++) {
	unsigned int hash_return_len;
 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, &counter, 1);
 	SHA256_Update(&ctx, (unsigned char *)&tmp, sizeof tmp);
 	SHA256_Update(&ctx, input_string_1, input_string_1_len);
	if (input_string_2) {
 	    SHA256_Update(&ctx, input_string_2, input_string_2_len);
	}
	SHA256_End(&ctx, requested_bytes, &hash_return_len,
		no_of_bytes_to_return);
	requested_bytes += hash_return_len;
	no_of_bytes_to_return -= hash_return_len;
    }
    return SECSuccess;
}


/*
 * Hash_DRBG Instantiate NIST SP 800-80 10.1.1.2
 *
 * NOTE: bytes & len are entropy || nonce || personalization_string. In
 * normal operation, NSS calculates them all together in a single call.
 */
static SECStatus
prng_instantiate(RNGContext *rng, PRUint8 *bytes, unsigned int len)
{
    prng_Hash_df(V(rng), VSize(rng), bytes, len, NULL, 0);
    rng->V_type = prngCGenerateType;
    prng_Hash_df(rng->C,sizeof rng->C,rng->V_Data,sizeof rng->V_Data,NULL,0);
    PRNG_RESET_RESEED_COUNT(rng)
    return SECSuccess;
}
    

/*
 * Update the global random number generator with more seeding
 * material. Use the Hash_DRBG reseed algorithm from NIST SP-800-90
 * section 10.1.1.3
 *
 * If entropy is NULL, it is fetched from the noise generator.
 */
static
SECStatus
prng_reseed(RNGContext *rng, const PRUint8 *entropy, unsigned int entropy_len,
	const PRUint8 *additional_input, unsigned int additional_input_len)
{
    PRUint8 noiseData[(sizeof rng->V_Data)+PRNG_SEEDLEN];
    PRUint8 *noise = &noiseData[0];

    /* if entropy wasn't supplied, fetch it. (normal operation case) */
    if (entropy == NULL) {
    	entropy_len = (unsigned int) RNG_SystemRNG(
			&noiseData[sizeof rng->V_Data], PRNG_SEEDLEN);
    } else {
	/* NOTE: this code is only available for testing, not to applications */
	/* if entropy was too big for the stack variable, get it from malloc */
	if (entropy_len > PRNG_SEEDLEN) {
	    noise = PORT_Alloc(entropy_len + (sizeof rng->V_Data));
	    if (noise == NULL) {
		return SECFailure;
	    }
	}
	PORT_Memcpy(&noise[sizeof rng->V_Data],entropy, entropy_len);
    }

    rng->V_type = prngReseedType;
    PORT_Memcpy(noise, rng->V_Data, sizeof rng->V_Data);
    prng_Hash_df(V(rng), VSize(rng), noise, (sizeof rng->V_Data) + entropy_len,
		additional_input, additional_input_len);
    /* clear potential CSP */
    PORT_Memset(noise, 0, (sizeof rng->V_Data) + entropy_len); 
    rng->V_type = prngCGenerateType;
    prng_Hash_df(rng->C,sizeof rng->C,rng->V_Data,sizeof rng->V_Data,NULL,0);
    PRNG_RESET_RESEED_COUNT(rng)

    if (noise != &noiseData[0]) {
	PORT_Free(noise);
    }
    return SECSuccess;
}

/*
 * build some fast inline functions for adding.
 */
#define PRNG_ADD_CARRY_ONLY(dest, start, cy) \
   carry = cy; \
   for (k1=start; carry && k1 >=0 ; k1--) { \
	carry = !(++dest[k1]); \
   } 

/*
 * NOTE: dest must be an array for the following to work.
 */
#define PRNG_ADD_BITS(dest, dest_len, add, len) \
    carry = 0; \
    for (k1=dest_len -1, k2=len-1; k2 >= 0; --k1, --k2) { \
	carry += dest[k1]+ add[k2]; \
	dest[k1] = (PRUint8) carry; \
	carry >>= 8; \
    }

#define PRNG_ADD_BITS_AND_CARRY(dest, dest_len, add, len) \
    PRNG_ADD_BITS(dest, dest_len, add, len) \
    PRNG_ADD_CARRY_ONLY(dest, k1, carry)

/*
 * This function expands the internal state of the prng to fulfill any number
 * of bytes we need for this request. We only use this call if we need more
 * than can be supplied by a single call to SHA256_HashBuf. 
 *
 * This function is specified in NIST SP 800-90 section 10.1.1.4, Hashgen
 */
static void
prng_Hashgen(RNGContext *rng, PRUint8 *returned_bytes, 
	     unsigned int no_of_returned_bytes)
{
    PRUint8 data[VSize(rng)];

    PORT_Memcpy(data, V(rng), VSize(rng));
    while (no_of_returned_bytes) {
	SHA256Context ctx;
	unsigned int len;
	unsigned int carry;
	int k1;

 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, data, sizeof data);
	SHA256_End(&ctx, returned_bytes, &len, no_of_returned_bytes);
	returned_bytes += len;
	no_of_returned_bytes -= len;
	/* The carry parameter is a bool (increment or not). 
	 * This increments data if no_of_returned_bytes is not zero */
	PRNG_ADD_CARRY_ONLY(data, (sizeof data)- 1, no_of_returned_bytes);
    }
    PORT_Memset(data, 0, sizeof data); 
}

/* 
 * Generates new random bytes and advances the internal prng state.	
 * additional bytes are only used in algorithm testing.
 * 
 * This function is specified in NIST SP 800-90 section 10.1.1.4
 */
static SECStatus
prng_generateNewBytes(RNGContext *rng, 
		PRUint8 *returned_bytes, unsigned int no_of_returned_bytes,
		const PRUint8 *additional_input,
		unsigned int additional_input_len)
{
    PRUint8 H[SHA256_LENGTH]; /* both H and w since they 
			       * aren't used concurrently */
    unsigned int carry;
    int k1, k2;

    if (!rng->isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    /* This code only triggers during tests, normal
     * prng operation does not use additional_input */
    if (additional_input){
	SHA256Context ctx;
	/* NIST SP 800-90 defines two temporaries in their calculations,
	 * w and H. These temporaries are the same lengths, and used
	 * at different times, so we use the following macro to collapse
	 * them to the same variable, but keeping their unique names for
	 * easy comparison to the spec */
#define w H
	rng->V_type = prngAdditionalDataType;
 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, rng->V_Data, sizeof rng->V_Data);
 	SHA256_Update(&ctx, additional_input, additional_input_len);
	SHA256_End(&ctx, w, NULL, sizeof w);
	PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), w, sizeof w)
	PORT_Memset(w, 0, sizeof w);
#undef w 
    }

    if (no_of_returned_bytes == SHA256_LENGTH) {
	/* short_cut to hashbuf and save a copy and a clear */
	SHA256_HashBuf(returned_bytes, V(rng), VSize(rng) );
    } else {
    	prng_Hashgen(rng, returned_bytes, no_of_returned_bytes);
    }
    /* advance our internal state... */
    rng->V_type = prngGenerateByteType;
    SHA256_HashBuf(H, rng->V_Data, sizeof rng->V_Data);
    PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), H, sizeof H)
    PRNG_ADD_BITS(V(rng), VSize(rng), rng->C, sizeof rng->C);
    PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), rng->reseed_counter, 
					sizeof rng->reseed_counter)
    PRNG_ADD_CARRY_ONLY(rng->reseed_counter,(sizeof rng->reseed_counter)-1, 1);

    /* continuous rng check */
    if (memcmp(V(rng), rng->oldV, sizeof rng->oldV) == 0) {
	rng->isValid = PR_FALSE;
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    PORT_Memcpy(rng->oldV, V(rng), sizeof rng->oldV);
    return SECSuccess;
}

/* Use NSPR to prevent RNG_RNGInit from being called from separate
 * threads, creating a race condition.
 */
static const PRCallOnceType pristineCallOnce;
static PRCallOnceType coRNGInit;
static PRStatus rng_init(void)
{
    PRUint8 bytes[PRNG_SEEDLEN*2]; /* entropy + nonce */
    unsigned int numBytes;
    if (globalrng == NULL) {
	/* create a new global RNG context */
	globalrng = &theGlobalRng;
        PORT_Assert(NULL == globalrng->lock);
	/* create a lock for it */
	globalrng->lock = PZ_NewLock(nssILockOther);
	if (globalrng->lock == NULL) {
	    globalrng = NULL;
	    PORT_SetError(PR_OUT_OF_MEMORY_ERROR);
	    return PR_FAILURE;
	}

	/* Try to get some seed data for the RNG */
	numBytes = (unsigned int) RNG_SystemRNG(bytes, sizeof bytes);
	PORT_Assert(numBytes == 0 || numBytes == sizeof bytes);
	if (numBytes != 0) {
	    /* if this is our first call,  instantiate, otherwise reseed 
	     * prng_instantiate gets a new clean state, we want to mix
	     * any previous entropy we may have collected */
	    if (V(globalrng)[0] == 0) {
		prng_instantiate(globalrng, bytes, numBytes);
	    } else {
		prng_reseed(globalrng, bytes, numBytes, NULL, 0);
	    }
	    memset(bytes, 0, numBytes);
	} else {
	    PZ_DestroyLock(globalrng->lock);
	    globalrng->lock = NULL;
	    globalrng->isValid = PR_FALSE;
	    globalrng = NULL;
	    return PR_FAILURE;
	}
	/* the RNG is in a valid state */
	globalrng->isValid = PR_TRUE;

	/* Fetch more entropy into the PRNG */
	RNG_SystemInfoForRNG();
    }
    return PR_SUCCESS;
}

/*
 * Clean up the global RNG context
 */
static void
prng_freeRNGContext(RNGContext *rng)
{
    PRUint8 inputhash[VSize(rng) + (sizeof rng->C)];

    /* destroy context lock */
    SKIP_AFTER_FORK(PZ_DestroyLock(globalrng->lock));

    /* zero global RNG context except for C & V to preserve entropy */
    prng_Hash_df(inputhash, sizeof rng->C, rng->C, sizeof rng->C, NULL, 0); 
    prng_Hash_df(&inputhash[sizeof rng->C], VSize(rng), V(rng), VSize(rng), 
								  NULL, 0); 
    memset(rng, 0, sizeof *rng);
    memcpy(rng->C, inputhash, sizeof rng->C); 
    memcpy(V(rng), &inputhash[sizeof rng->C], VSize(rng)); 

    globalrng = NULL;
}

/*
 * Public functions
 */

/*
 * Initialize the global RNG context and give it some seed input taken
 * from the system.  This function is thread-safe and will only allow
 * the global context to be initialized once.  The seed input is likely
 * small, so it is imperative that RNG_RandomUpdate() be called with
 * additional seed data before the generator is used.  A good way to
 * provide the generator with additional entropy is to call
 * RNG_SystemInfoForRNG().  Note that C_Initialize() does exactly that.
 */
SECStatus 
RNG_RNGInit(void)
{
    /* Allow only one call to initialize the context */
    PR_CallOnce(&coRNGInit, rng_init);
    /* Make sure there is a context */
    return (globalrng != NULL) ? PR_SUCCESS : PR_FAILURE;
}

/*
** Update the global random number generator with more seeding
** material.
*/
SECStatus 
RNG_RandomUpdate(const void *data, size_t bytes)
{
    SECStatus rv;

    /* Make sure our assumption that size_t is unsigned is true */
    PR_STATIC_ASSERT(((size_t)-1) > (size_t)1);

#if defined(NS_PTR_GT_32) || (defined(NSS_USE_64) && !defined(NS_PTR_LE_32))
    /*
     * NIST 800-90 requires us to verify our inputs. This value can
     * come from the application, so we need to make sure it's within the
     * spec. The spec says it must be less than 2^32 bytes (2^35 bits).
     * This can only happen if size_t is greater than 32 bits (i.e. on
     * most 64 bit platforms). The 90% case (perhaps 100% case), size_t
     * is less than or equal to 32 bits if the platform is not 64 bits, and
     * greater than 32 bits if it is a 64 bit platform. The corner
     * cases are handled with explicit defines NS_PTR_GT_32 and NS_PTR_LE_32.
     *
     * In general, neither NS_PTR_GT_32 nor NS_PTR_LE_32 will need to be 
     * defined. If you trip over the next two size ASSERTS at compile time,
     * you will need to define them for your platform.
     *
     * if 'sizeof(size_t) > 4' is triggered it means that we were expecting
     *   sizeof(size_t) to be greater than 4, but it wasn't. Setting 
     *   NS_PTR_LE_32 will correct that mistake.
     *
     * if 'sizeof(size_t) <= 4' is triggered, it means that we were expecting
     *   sizeof(size_t) to be less than or equal to 4, but it wasn't. Setting 
     *   NS_PTR_GT_32 will correct that mistake.
     */

    PR_STATIC_ASSERT(sizeof(size_t) > 4);

    if (bytes > PRNG_MAX_ADDITIONAL_BYTES) {
	bytes = PRNG_MAX_ADDITIONAL_BYTES;
    }
#else
    PR_STATIC_ASSERT(sizeof(size_t) <= 4);
#endif

    PZ_Lock(globalrng->lock);
    /* if we're passed more than our additionalDataCache, simply
     * call reseed with that data */
    if (bytes > sizeof (globalrng->additionalDataCache)) {
	rv = prng_reseed(globalrng, NULL, 0, data, (unsigned int) bytes);
    /* if we aren't going to fill or overflow the buffer, just cache it */
    } else if (bytes < ((sizeof globalrng->additionalDataCache)
				- globalrng->additionalAvail)) {
	PORT_Memcpy(globalrng->additionalDataCache+globalrng->additionalAvail,
		    data, bytes);
	globalrng->additionalAvail += (PRUint32) bytes;
	rv = SECSuccess;
    } else {
	/* we are going to fill or overflow the buffer. In this case we will
	 * fill the entropy buffer, reseed with it, start a new buffer with the
	 * remainder. We know the remainder will fit in the buffer because
	 * we already handled the case where bytes > the size of the buffer.
	 */
	size_t bufRemain = (sizeof globalrng->additionalDataCache) 
					- globalrng->additionalAvail;
	/* fill the rest of the buffer */
	if (bufRemain) {
	    PORT_Memcpy(globalrng->additionalDataCache
			+globalrng->additionalAvail, 
			data, bufRemain);
	    data = ((unsigned char *)data) + bufRemain;
	    bytes -= bufRemain;
	}
	/* reseed from buffer */
	rv = prng_reseed(globalrng, NULL, 0, globalrng->additionalDataCache, 
					sizeof globalrng->additionalDataCache);

	/* copy the rest into the cache */
	PORT_Memcpy(globalrng->additionalDataCache, data, bytes);
	globalrng->additionalAvail = (PRUint32) bytes;
    }
		
    PZ_Unlock(globalrng->lock);
    return rv;
}

/*
** Generate some random bytes, using the global random number generator
** object.
*/
static SECStatus 
prng_GenerateGlobalRandomBytes(RNGContext *rng,
                               void *dest, size_t len)
{
    SECStatus rv = SECSuccess;
    PRUint8 *output = dest;
    /* check for a valid global RNG context */
    PORT_Assert(rng != NULL);
    if (rng == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    /* FIPS limits the amount of entropy available in a single request */
    if (len > PRNG_MAX_REQUEST_SIZE) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    /* --- LOCKED --- */
    PZ_Lock(rng->lock);
    /* Check the amount of seed data in the generator.  If not enough,
     * don't produce any data.
     */
    if (rng->reseed_counter[0] >= RESEED_VALUE) {
	rv = prng_reseed(rng, NULL, 0, NULL, 0);
	PZ_Unlock(rng->lock);
	if (rv != SECSuccess) {
	    return rv;
	}
	RNG_SystemInfoForRNG();
	PZ_Lock(rng->lock);
    }
    /*
     * see if we have enough bytes to fulfill the request.
     */
    if (len <= rng->dataAvail) {
	memcpy(output, rng->data + ((sizeof rng->data) - rng->dataAvail), len);
	memset(rng->data + ((sizeof rng->data) - rng->dataAvail), 0, len);
	rng->dataAvail -= len;
	rv = SECSuccess;
    /* if we are asking for a small number of bytes, cache the rest of 
     * the bytes */
    } else if (len < sizeof rng->data) {
	rv = prng_generateNewBytes(rng, rng->data, sizeof rng->data, 
			rng->additionalAvail ? rng->additionalDataCache : NULL,
			rng->additionalAvail);
	rng->additionalAvail = 0;
	if (rv == SECSuccess) {
	    memcpy(output, rng->data, len);
	    memset(rng->data, 0, len); 
	    rng->dataAvail = (sizeof rng->data) - len;
	}
    /* we are asking for lots of bytes, just ask the generator to pass them */
    } else {
	rv = prng_generateNewBytes(rng, output, len,
			rng->additionalAvail ? rng->additionalDataCache : NULL,
			rng->additionalAvail);
	rng->additionalAvail = 0;
    }
    PZ_Unlock(rng->lock);
    /* --- UNLOCKED --- */
    return rv;
}

/*
** Generate some random bytes, using the global random number generator
** object.
*/
SECStatus 
RNG_GenerateGlobalRandomBytes(void *dest, size_t len)
{
    return prng_GenerateGlobalRandomBytes(globalrng, dest, len);
}

void
RNG_RNGShutdown(void)
{
    /* check for a valid global RNG context */
    PORT_Assert(globalrng != NULL);
    if (globalrng == NULL) {
	/* Should set a "not initialized" error code. */
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return;
    }
    /* clear */
    prng_freeRNGContext(globalrng);
    globalrng = NULL;
    /* reset the callonce struct to allow a new call to RNG_RNGInit() */
    coRNGInit = pristineCallOnce;
}

/*
 * Test case interface. used by fips testing and power on self test
 */
 /* make sure the test context is separate from the global context, This
  * allows us to test the internal random number generator without losing
  * entropy we may have previously collected. */
RNGContext testContext;

/*
 * Test vector API. Use NIST SP 800-90 general interface so one of the
 * other NIST SP 800-90 algorithms may be used in the future.
 */
SECStatus
PRNGTEST_Instantiate(const PRUint8 *entropy, unsigned int entropy_len, 
		const PRUint8 *nonce, unsigned int nonce_len,
		const PRUint8 *personal_string, unsigned int ps_len)
{
   int bytes_len = entropy_len + nonce_len + ps_len;
   PRUint8 *bytes = PORT_Alloc(bytes_len);

   if (bytes == NULL) {
	return SECFailure;
   }
   /* concatenate the various inputs, internally NSS only instantiates with
    * a single long string */
   PORT_Memcpy(bytes, entropy, entropy_len);
   if (nonce) {
	PORT_Memcpy(&bytes[entropy_len], nonce, nonce_len);
   } else {
	PORT_Assert(nonce_len == 0);
   }
   if (personal_string) {
       PORT_Memcpy(&bytes[entropy_len+nonce_len], personal_string, ps_len);
   } else {
	PORT_Assert(ps_len == 0);
   }
   prng_instantiate(&testContext, bytes, bytes_len);
   testContext.isValid = PR_TRUE;
   PORT_ZFree(bytes, bytes_len);
   return SECSuccess;
}

SECStatus
PRNGTEST_Reseed(const PRUint8 *entropy, unsigned int entropy_len, 
		  const PRUint8 *additional, unsigned int additional_len)
{
    if (!testContext.isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return prng_reseed(&testContext, entropy, entropy_len, additional,
			additional_len);

}

SECStatus
PRNGTEST_Generate(PRUint8 *bytes, unsigned int bytes_len, 
		  const PRUint8 *additional, unsigned int additional_len)
{
    if (!testContext.isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    return prng_generateNewBytes(&testContext, bytes, bytes_len,
			additional, additional_len);

}

SECStatus
PRNGTEST_Uninstantiate()
{
   PORT_Memset(&testContext, 0, sizeof testContext);
   return SECSuccess;
}


