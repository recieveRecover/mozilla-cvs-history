/*
 * vtables (and methods that call through them) for the 4 types of 
 * SSLSockets supported.  Only one type is still supported.
 * Various other functions.
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 *
 * $Id: sslsock.c,v 1.7 2001/01/05 01:38:26 nelsonb%netscape.com Exp $
 */
#include "seccomon.h"
#include "cert.h"
#include "keyhi.h"
#include "ssl.h"
#include "sslimpl.h"
#include "sslproto.h"
#include "nspr.h"

#define SET_ERROR_CODE   /* reminder */

struct cipherPolicyStr {
	int		cipher;
	unsigned char 	export;	/* policy value for export policy */
	unsigned char 	france;	/* policy value for france policy */
};

typedef struct cipherPolicyStr cipherPolicy;

/* this table reflects Netscape's browser policies. */
static cipherPolicy ssl_ciphers[] = {	   /*   Export           France   */
 {  SSL_EN_RC4_128_WITH_MD5,		    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_EN_RC4_128_EXPORT40_WITH_MD5,	    SSL_ALLOWED,     SSL_ALLOWED },
 {  SSL_EN_RC2_128_CBC_WITH_MD5,	    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_EN_RC2_128_CBC_EXPORT40_WITH_MD5,   SSL_ALLOWED,     SSL_ALLOWED },
 {  SSL_EN_DES_64_CBC_WITH_MD5,		    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_EN_DES_192_EDE3_CBC_WITH_MD5,	    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_FORTEZZA_DMS_WITH_FORTEZZA_CBC_SHA, SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_FORTEZZA_DMS_WITH_RC4_128_SHA,      SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_RSA_WITH_RC4_128_MD5,		    SSL_RESTRICTED,  SSL_NOT_ALLOWED },
 {  SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA,	    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_RSA_WITH_3DES_EDE_CBC_SHA,	    SSL_RESTRICTED,  SSL_NOT_ALLOWED },
 {  SSL_RSA_FIPS_WITH_DES_CBC_SHA,	    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_RSA_WITH_DES_CBC_SHA,		    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_RSA_EXPORT_WITH_RC4_40_MD5,	    SSL_ALLOWED,     SSL_ALLOWED },
 {  SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5,	    SSL_ALLOWED,     SSL_ALLOWED },
 {  SSL_FORTEZZA_DMS_WITH_NULL_SHA,	    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED },
 {  SSL_RSA_WITH_NULL_MD5,		    SSL_ALLOWED,     SSL_ALLOWED },
 {  TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA,    SSL_ALLOWED,     SSL_NOT_ALLOWED },
 {  TLS_RSA_EXPORT1024_WITH_RC4_56_SHA,     SSL_ALLOWED,     SSL_NOT_ALLOWED },
 {  0,					    SSL_NOT_ALLOWED, SSL_NOT_ALLOWED }
};

static const sslSocketOps ssl_default_ops = {	/* No SSL, No Socks. */
    ssl_DefConnect,
    NULL,
    ssl_DefBind,
    ssl_DefListen,
    ssl_DefShutdown,
    ssl_DefClose,
    ssl_DefRecv,
    ssl_DefSend,
    ssl_DefRead,
    ssl_DefWrite,
    ssl_DefGetpeername,
    ssl_DefGetsockname
};

static const sslSocketOps ssl_socks_ops = {	/* No SSL, has socks. */
    ssl_SocksConnect,
    ssl_SocksAccept,
    ssl_SocksBind,
    ssl_SocksListen,
    ssl_DefShutdown,
    ssl_DefClose,
    ssl_SocksRecv,
    ssl_SocksSend,
    ssl_SocksRead,
    ssl_SocksWrite,
    ssl_DefGetpeername,
    ssl_SocksGetsockname
};

static const sslSocketOps ssl_secure_ops = {	/* SSL, no socks. */
    ssl_SecureConnect,
    NULL,
    ssl_DefBind,
    ssl_DefListen,
    ssl_SecureShutdown,
    ssl_SecureClose,
    ssl_SecureRecv,
    ssl_SecureSend,
    ssl_SecureRead,
    ssl_SecureWrite,
    ssl_DefGetpeername,
    ssl_DefGetsockname
};

static const sslSocketOps ssl_secure_socks_ops = { /* Both SSL and Socks. */
    ssl_SecureSocksConnect,
    ssl_SecureSocksAccept,
    ssl_SocksBind,
    ssl_SocksListen,
    ssl_SecureShutdown,
    ssl_SecureClose,
    ssl_SecureRecv,
    ssl_SecureSend,
    ssl_SecureRead,
    ssl_SecureWrite,
    ssl_DefGetpeername,
    ssl_SocksGetsockname
};

/*
** default settings for socket enables
*/
static sslOptions ssl_defaults = {
    PR_TRUE, 	/* useSecurity        */
    PR_FALSE,	/* useSocks           */
    PR_FALSE,	/* requestCertificate */
    2,	        /* requireCertificate */
    PR_FALSE,	/* handshakeAsClient  */
    PR_FALSE,	/* handshakeAsServer  */
    PR_TRUE,	/* enableSSL2         */
    PR_TRUE,	/* enableSSL3         */
    PR_TRUE, 	/* enableTLS          */ /* now defaults to on in NSS 3.0 */
    PR_FALSE,	/* noCache            */
    PR_FALSE,	/* fdx                */
    PR_TRUE,	/* v2CompatibleHello  */
    PR_TRUE,	/* detectRollBack     */
};

sslSessionIDLookupFunc  ssl_sid_lookup;
sslSessionIDCacheFunc   ssl_sid_cache;
sslSessionIDUncacheFunc ssl_sid_uncache;

static PRBool ssl_inited = PR_FALSE;
static PRDescIdentity ssl_layer_id;

int                     ssl_lock_readers	= 1;	/* default true. */
char                    ssl_debug;
char                    ssl_trace;


/* forward declarations. */
static sslSocket *ssl_NewSocket(void);
static PRStatus   ssl_PushIOLayer(sslSocket *ns, PRFileDesc *stack, 
                                  PRDescIdentity id);

/************************************************************************/

/*
** Lookup a socket structure from a file descriptor.
*/
static sslSocket *
ssl_GetPrivate(PRFileDesc *fd)
{
    sslSocket *ss;

    PORT_Assert(fd != NULL);
    PORT_Assert(fd->methods->file_type == PR_DESC_LAYERED);
    PORT_Assert(fd->identity == ssl_layer_id);

    ss = (sslSocket *)fd->secret;
    ss->fd = fd;
    return ss;
}

sslSocket *
ssl_FindSocket(PRFileDesc *fd)
{
    PRFileDesc *layer;
    sslSocket *ss;

    PORT_Assert(fd != NULL);
    PORT_Assert(ssl_layer_id != 0);

    layer = PR_GetIdentitiesLayer(fd, ssl_layer_id);
    if (layer == NULL)
	return NULL;

    ss = (sslSocket *)layer->secret;
    ss->fd = layer;
    return ss;
}

sslSocket *
ssl_DupSocket(sslSocket *os)
{
    sslSocket *ss;
    SECStatus rv;

    ss = ssl_NewSocket();
    if (ss) {
	ss->useSocks           = os->useSocks;
	ss->useSecurity        = os->useSecurity;
	ss->requestCertificate = os->requestCertificate;
	ss->requireCertificate = os->requireCertificate;
	ss->handshakeAsClient  = os->handshakeAsClient;
	ss->handshakeAsServer  = os->handshakeAsServer;
	ss->enableSSL2         = os->enableSSL2;
	ss->enableSSL3         = os->enableSSL3;
	ss->enableTLS          = os->enableTLS;
	ss->noCache            = os->noCache;
	ss->fdx                = os->fdx;
	ss->v2CompatibleHello  = os->v2CompatibleHello;
	ss->detectRollBack     = os->detectRollBack;
	ss->peerID             = !os->peerID ? NULL : PORT_Strdup(os->peerID);
	ss->url                = !os->url    ? NULL : PORT_Strdup(os->url);

	ss->ops      = os->ops;
	ss->peer     = os->peer;
	ss->port     = os->port;
	ss->rTimeout = os->rTimeout;
	ss->wTimeout = os->wTimeout;
	ss->cTimeout = os->cTimeout;
	ss->dbHandle = os->dbHandle;

	/* copy ssl2&3 policy & prefs, even if it's not selected (yet) */
	ss->allowedByPolicy	= os->allowedByPolicy;
	ss->maybeAllowedByPolicy= os->maybeAllowedByPolicy;
	ss->chosenPreference 	= os->chosenPreference;
	PORT_Memcpy(ss->cipherSuites, os->cipherSuites, sizeof os->cipherSuites);

	if (os->cipherSpecs) {
	    ss->cipherSpecs  = (unsigned char*)PORT_Alloc(os->sizeCipherSpecs);
	    if (ss->cipherSpecs) 
	    	PORT_Memcpy(ss->cipherSpecs, os->cipherSpecs, 
		            os->sizeCipherSpecs);
	    ss->sizeCipherSpecs    = os->sizeCipherSpecs;
	    ss->preferredCipher    = os->preferredCipher;
	} else {
	    ss->cipherSpecs        = NULL;  /* produced lazily */
	    ss->sizeCipherSpecs    = 0;
	    ss->preferredCipher    = NULL;
	}
	if (ss->useSecurity) {
	    /* This int should be SSLKEAType, but CC on Irix complains,
	     * during the for loop.
	     */
	    int i;

	    for (i=kt_null; i < kt_kea_size; i++) {
		if (os->serverCert[i] && os->serverCertChain[i]) {
		    ss->serverCert[i] = CERT_DupCertificate(os->serverCert[i]);
		    ss->serverCertChain[i] = CERT_DupCertList(
		                                       os->serverCertChain[i]);
		} else {
		    ss->serverCert[i]      = NULL;
		    ss->serverCertChain[i] = NULL;
		}
		ss->serverKey[i] = os->serverKey[i] ?
				SECKEY_CopyPrivateKey(os->serverKey[i]) : NULL;
	    }
	    ss->stepDownKeyPair = !os->stepDownKeyPair ? NULL :
		                  ssl3_GetKeyPairRef(os->stepDownKeyPair);
/*
 * XXX the preceeding CERT_ and SECKEY_ functions can fail and return NULL.
 * XXX We should detect this, and not just march on with NULL pointers.
 */
	    ss->authCertificate       = os->authCertificate;
	    ss->authCertificateArg    = os->authCertificateArg;
	    ss->getClientAuthData     = os->getClientAuthData;
	    ss->getClientAuthDataArg  = os->getClientAuthDataArg;
	    ss->handleBadCert         = os->handleBadCert;
	    ss->badCertArg            = os->badCertArg;
	    ss->handshakeCallback     = os->handshakeCallback;
	    ss->handshakeCallbackData = os->handshakeCallbackData;
	    ss->pkcs11PinArg          = os->pkcs11PinArg;
    
	    /* Create security data */
	    rv = ssl_CopySecurityInfo(ss, os);
	    if (rv != SECSuccess) {
		goto losage;
	    }
	}
	if (ss->useSocks) {
	    /* Create security data */
	    rv = ssl_CopySocksInfo(ss, os);
	    if (rv != SECSuccess) {
		goto losage;
	    }
	}
    }
    return ss;

  losage:
    return NULL;
}

/*
 * free an sslSocket struct, and all the stuff that hangs off of it
 */
void
ssl_FreeSocket(sslSocket *ss)
{
    /* "i" should be of type SSLKEAType, but CC on IRIX complains during
     * the for loop.
     */
    int        i;

    sslSocket *fs;
    sslSocket  lSock;

/* Get every lock you can imagine!
** Caller already holds these:
**  SSL_LOCK_READER(ss);
**  SSL_LOCK_WRITER(ss);
*/
    ssl_Get1stHandshakeLock(ss);
    ssl_GetRecvBufLock(ss);
    ssl_GetSSL3HandshakeLock(ss);
    ssl_GetXmitBufLock(ss);
    ssl_GetSpecWriteLock(ss);

#ifdef DEBUG
    fs = &lSock;
    *fs = *ss;				/* Copy the old socket structure, */
    PORT_Memset(ss, 0x1f, sizeof *ss);  /* then blast the old struct ASAP. */
#else
    fs = ss;
#endif

    /* Free up socket */
    ssl_DestroySocksInfo(fs->socks);
    ssl_DestroySecurityInfo(fs->sec);
    ssl3_DestroySSL3Info(fs->ssl3);
    PORT_Free(fs->saveBuf.buf);
    PORT_Free(fs->pendingBuf.buf);
    if (fs->gather) {
	ssl_DestroyGather(fs->gather);
    }
    if (fs->peerID != NULL)
	PORT_Free(fs->peerID);
    if (fs->url != NULL)
	PORT_Free((void *)fs->url);	/* CONST */

    /* Clean up server configuration */
    for (i=kt_null; i < kt_kea_size; i++) {
	if (fs->serverCert[i] != NULL)
	    CERT_DestroyCertificate(fs->serverCert[i]);
	if (fs->serverCertChain[i] != NULL)
	    CERT_DestroyCertificateList(fs->serverCertChain[i]);
	if (fs->serverKey[i] != NULL)
	    SECKEY_DestroyPrivateKey(fs->serverKey[i]);
    }
    if (fs->stepDownKeyPair) {
	ssl3_FreeKeyPair(fs->stepDownKeyPair);
	fs->stepDownKeyPair = NULL;
    }


    /* Release all the locks acquired above.  */
    SSL_UNLOCK_READER(fs);
    SSL_UNLOCK_WRITER(fs);
    ssl_Release1stHandshakeLock(fs);
    ssl_ReleaseRecvBufLock(fs);
    ssl_ReleaseSSL3HandshakeLock(fs);
    ssl_ReleaseXmitBufLock(fs);
    ssl_ReleaseSpecWriteLock(fs);

    /* Destroy locks. */
    if (fs->firstHandshakeLock) {
    	PZ_DestroyMonitor(fs->firstHandshakeLock);
	fs->firstHandshakeLock = NULL;
    }
    if (fs->ssl3HandshakeLock) {
    	PZ_DestroyMonitor(fs->ssl3HandshakeLock);
	fs->ssl3HandshakeLock = NULL;
    }
    if (fs->specLock) {
    	NSSRWLock_Destroy(fs->specLock);
	fs->specLock = NULL;
    }

    if (fs->recvLock) {
    	PZ_DestroyLock(fs->recvLock);
	fs->recvLock = NULL;
    }
    if (fs->sendLock) {
    	PZ_DestroyLock(fs->sendLock);
	fs->sendLock = NULL;
    }
    if (fs->xmitBufLock) {
    	PZ_DestroyMonitor(fs->xmitBufLock);
	fs->xmitBufLock = NULL;
    }
    if (fs->recvBufLock) {
    	PZ_DestroyMonitor(fs->recvBufLock);
	fs->recvBufLock = NULL;
    }
    if (fs->cipherSpecs) {
	PORT_Free(fs->cipherSpecs);
	fs->cipherSpecs     = NULL;
	fs->sizeCipherSpecs = 0;
    }

    PORT_Free(ss);	/* free the caller's copy, not ours. */
    return;
}

/************************************************************************/

static void
ssl_ChooseOps(sslSocket *ss)
{
    if (ss->useSocks)  {
    	ss->ops = ss->useSecurity ? &ssl_secure_socks_ops : &ssl_socks_ops ;
    } else {
    	ss->ops = ss->useSecurity ? &ssl_secure_ops       : &ssl_default_ops;
    }
}

/* Called from SSL_Enable (immediately below) */
static SECStatus
PrepareSocket(sslSocket *ss)
{
    SECStatus     rv = SECSuccess;

    if (ss->useSocks) {
	rv = ssl_CreateSocksInfo(ss);
	if (rv != SECSuccess) {
	    return rv;
	}
    }
    if (ss->useSecurity) {
	rv = ssl_CreateSecurityInfo(ss);
	if (rv != SECSuccess) {
	    return rv;
	}
    }

    ssl_ChooseOps(ss);
    return rv;
}

SECStatus
SSL_Enable(PRFileDesc *fd, int which, PRBool on)
{
    return SSL_OptionSet(fd, which, on);
}

SECStatus
SSL_OptionSet(PRFileDesc *fd, PRInt32 which, PRBool on)
{
    sslSocket *ss = ssl_FindSocket(fd);
    SECStatus  rv = SECSuccess;

    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in Enable", SSL_GETPID(), fd));
	PORT_SetError(PR_BAD_DESCRIPTOR_ERROR);
	return SECFailure;
    }

    ssl_Get1stHandshakeLock(ss);
    ssl_GetSSL3HandshakeLock(ss);

    switch (which) {
      case SSL_SOCKS:
	ss->useSocks = on;
	rv = PrepareSocket(ss);
	break;

      case SSL_SECURITY:
	ss->useSecurity = on;
	rv = PrepareSocket(ss);
	break;

      case SSL_REQUEST_CERTIFICATE:
	ss->requestCertificate = on;
	break;

      case SSL_REQUIRE_CERTIFICATE:
	ss->requireCertificate = on;
	break;

      case SSL_HANDSHAKE_AS_CLIENT:
	if ( ss->handshakeAsServer && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    rv = SECFailure;
	    break;
	}
	ss->handshakeAsClient = on;
	break;

      case SSL_HANDSHAKE_AS_SERVER:
	if ( ss->handshakeAsClient && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    rv = SECFailure;
	    break;
	}
	ss->handshakeAsServer = on;
	break;

      case SSL_ENABLE_TLS:
	ss->enableTLS           = on;
	ss->preferredCipher     = NULL;
	if (ss->cipherSpecs) {
	    PORT_Free(ss->cipherSpecs);
	    ss->cipherSpecs     = NULL;
	    ss->sizeCipherSpecs = 0;
	}
	break;

      case SSL_ENABLE_SSL3:
	ss->enableSSL3          = on;
	ss->preferredCipher     = NULL;
	if (ss->cipherSpecs) {
	    PORT_Free(ss->cipherSpecs);
	    ss->cipherSpecs     = NULL;
	    ss->sizeCipherSpecs = 0;
	}
	break;

      case SSL_ENABLE_SSL2:
	ss->enableSSL2          = on;
	if (on) {
	    ss->v2CompatibleHello = on;
	}
	ss->preferredCipher     = NULL;
	if (ss->cipherSpecs) {
	    PORT_Free(ss->cipherSpecs);
	    ss->cipherSpecs     = NULL;
	    ss->sizeCipherSpecs = 0;
	}
	break;

      case SSL_NO_CACHE:
	ss->noCache = on;
	break;

      case SSL_ENABLE_FDX:
      	ss->fdx = on;
	break;

      case SSL_V2_COMPATIBLE_HELLO:
      	ss->v2CompatibleHello = on;
	if (!on) {
	    ss->enableSSL2    = on;
	}
	break;

      case SSL_ROLLBACK_DETECTION:  
	ss->detectRollBack = on;
        break;

      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	rv = SECFailure;
    }

    ssl_ReleaseSSL3HandshakeLock(ss);
    ssl_Release1stHandshakeLock(ss);

    return rv;
}

SECStatus
SSL_OptionGet(PRFileDesc *fd, PRInt32 which, PRBool *pOn)
{
    sslSocket *ss = ssl_FindSocket(fd);
    SECStatus  rv = SECSuccess;
    PRBool     on = PR_FALSE;

    if (!pOn) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in Enable", SSL_GETPID(), fd));
	PORT_SetError(PR_BAD_DESCRIPTOR_ERROR);
	*pOn = PR_FALSE;
	return SECFailure;
    }

    ssl_Get1stHandshakeLock(ss);
    ssl_GetSSL3HandshakeLock(ss);

    switch (which) {
    case SSL_SOCKS:               on = ss->useSocks;           break;
    case SSL_SECURITY:            on = ss->useSecurity;        break;
    case SSL_REQUEST_CERTIFICATE: on = ss->requestCertificate; break;
    case SSL_REQUIRE_CERTIFICATE: on = ss->requireCertificate; break;
    case SSL_HANDSHAKE_AS_CLIENT: on = ss->handshakeAsClient;  break;
    case SSL_HANDSHAKE_AS_SERVER: on = ss->handshakeAsServer;  break;
    case SSL_ENABLE_TLS:          on = ss->enableTLS;          break;
    case SSL_ENABLE_SSL3:         on = ss->enableSSL3;         break;
    case SSL_ENABLE_SSL2:         on = ss->enableSSL2;         break;
    case SSL_NO_CACHE:            on = ss->noCache;            break;
    case SSL_ENABLE_FDX:          on = ss->fdx;                break;
    case SSL_V2_COMPATIBLE_HELLO: on = ss->v2CompatibleHello;  break;
    case SSL_ROLLBACK_DETECTION:  on = ss->detectRollBack;     break;

    default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	rv = SECFailure;
    }

    ssl_ReleaseSSL3HandshakeLock(ss);
    ssl_Release1stHandshakeLock(ss);

    *pOn = on;
    return rv;
}

SECStatus
SSL_OptionGetDefault(PRInt32 which, PRBool *pOn)
{
    SECStatus  rv = SECSuccess;
    PRBool     on = PR_FALSE;

    if (!pOn) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    switch (which) {
    case SSL_SOCKS:               on = ssl_defaults.useSocks;           break;
    case SSL_SECURITY:            on = ssl_defaults.useSecurity;        break;
    case SSL_REQUEST_CERTIFICATE: on = ssl_defaults.requestCertificate; break;
    case SSL_REQUIRE_CERTIFICATE: on = ssl_defaults.requireCertificate; break;
    case SSL_HANDSHAKE_AS_CLIENT: on = ssl_defaults.handshakeAsClient;  break;
    case SSL_HANDSHAKE_AS_SERVER: on = ssl_defaults.handshakeAsServer;  break;
    case SSL_ENABLE_TLS:          on = ssl_defaults.enableTLS;          break;
    case SSL_ENABLE_SSL3:         on = ssl_defaults.enableSSL3;         break;
    case SSL_ENABLE_SSL2:         on = ssl_defaults.enableSSL2;         break;
    case SSL_NO_CACHE:            on = ssl_defaults.noCache;		break;
    case SSL_ENABLE_FDX:          on = ssl_defaults.fdx;                break;
    case SSL_V2_COMPATIBLE_HELLO: on = ssl_defaults.v2CompatibleHello;  break;
    case SSL_ROLLBACK_DETECTION:  on = ssl_defaults.detectRollBack;     break;

    default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	rv = SECFailure;
    }

    *pOn = on;
    return rv;
}

/* XXX Use Global Lock to protect this stuff. */
SECStatus
SSL_EnableDefault(int which, PRBool on)
{
    return SSL_OptionSetDefault(which, on);
}

SECStatus
SSL_OptionSetDefault(PRInt32 which, PRBool on)
{
    switch (which) {
      case SSL_SOCKS:
	ssl_defaults.useSocks = on;
	break;

      case SSL_SECURITY:
	ssl_defaults.useSecurity = on;
	break;

      case SSL_REQUEST_CERTIFICATE:
	ssl_defaults.requestCertificate = on;
	break;

      case SSL_REQUIRE_CERTIFICATE:
	ssl_defaults.requireCertificate = on;
	break;

      case SSL_HANDSHAKE_AS_CLIENT:
	if ( ssl_defaults.handshakeAsServer && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	ssl_defaults.handshakeAsClient = on;
	break;

      case SSL_HANDSHAKE_AS_SERVER:
	if ( ssl_defaults.handshakeAsClient && on ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	ssl_defaults.handshakeAsServer = on;
	break;

      case SSL_ENABLE_TLS:
	ssl_defaults.enableTLS = on;
	break;

      case SSL_ENABLE_SSL3:
	ssl_defaults.enableSSL3 = on;
	break;

      case SSL_ENABLE_SSL2:
	ssl_defaults.enableSSL2 = on;
	if (on) {
	    ssl_defaults.v2CompatibleHello = on;
	}
	break;

      case SSL_NO_CACHE:
	ssl_defaults.noCache = on;
	break;

      case SSL_ENABLE_FDX:
      	ssl_defaults.fdx = on;

      case SSL_V2_COMPATIBLE_HELLO:
      	ssl_defaults.v2CompatibleHello = on;
	if (!on) {
	    ssl_defaults.enableSSL2    = on;
	}
	break;

      case SSL_ROLLBACK_DETECTION:  
	ssl_defaults.detectRollBack = on;
	break;

      default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    return SECSuccess;
}

/* Part of the public NSS API.
 * Since this is a global (not per-socket) setting, we cannot use the
 * HandshakeLock to protect this.  Probably want a global lock.
 */
SECStatus
SSL_SetPolicy(long which, int policy)
{
    if ((which & 0xfffe) == SSL_RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA) {
    	/* one of the two old FIPS ciphers */
	if (which == SSL_RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA) 
	    which = SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA;
	else if (which == SSL_RSA_OLDFIPS_WITH_DES_CBC_SHA)
	    which = SSL_RSA_FIPS_WITH_DES_CBC_SHA;
    }
    return SSL_CipherPolicySet(which, policy);
}

SECStatus
SSL_CipherPolicySet(PRInt32 which, PRInt32 policy)
{
    SECStatus rv;

    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_SetPolicy(which, policy);
    } else {
	rv = ssl3_SetPolicy((ssl3CipherSuite)which, policy);
    }
    return rv;
}

SECStatus
SSL_CipherPolicyGet(PRInt32 which, PRInt32 *oPolicy)
{
    SECStatus rv;

    if (!oPolicy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_GetPolicy(which, oPolicy);
    } else {
	rv = ssl3_GetPolicy((ssl3CipherSuite)which, oPolicy);
    }
    return rv;
}

/* Part of the public NSS API.
 * Since this is a global (not per-socket) setting, we cannot use the
 * HandshakeLock to protect this.  Probably want a global lock.
 * These changes have no effect on any sslSockets already created. 
 */
SECStatus
SSL_EnableCipher(long which, PRBool enabled)
{
    if ((which & 0xfffe) == SSL_RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA) {
    	/* one of the two old FIPS ciphers */
	if (which == SSL_RSA_OLDFIPS_WITH_3DES_EDE_CBC_SHA) 
	    which = SSL_RSA_FIPS_WITH_3DES_EDE_CBC_SHA;
	else if (which == SSL_RSA_OLDFIPS_WITH_DES_CBC_SHA)
	    which = SSL_RSA_FIPS_WITH_DES_CBC_SHA;
    }
    return SSL_CipherPrefSetDefault(which, enabled);
}

SECStatus
SSL_CipherPrefSetDefault(PRInt32 which, PRBool enabled)
{
    SECStatus rv;
    
    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_CipherPrefSetDefault(which, enabled);
    } else {
	rv = ssl3_CipherPrefSetDefault((ssl3CipherSuite)which, enabled);
    }
    return rv;
}

SECStatus 
SSL_CipherPrefGetDefault(PRInt32 which, PRBool *enabled)
{
    SECStatus  rv;
    
    if (!enabled) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_CipherPrefGetDefault(which, enabled);
    } else {
	rv = ssl3_CipherPrefGetDefault((ssl3CipherSuite)which, enabled);
    }
    return rv;
}

SECStatus
SSL_CipherPrefSet(PRFileDesc *fd, PRInt32 which, PRBool enabled)
{
    SECStatus rv;
    sslSocket *ss = ssl_FindSocket(fd);
    
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in CipherPrefSet", SSL_GETPID(), fd));
	PORT_SetError(PR_BAD_DESCRIPTOR_ERROR);
	return SECFailure;
    }
    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_CipherPrefSet(ss, which, enabled);
    } else {
	rv = ssl3_CipherPrefSet(ss, (ssl3CipherSuite)which, enabled);
    }
    return rv;
}

SECStatus 
SSL_CipherPrefGet(PRFileDesc *fd, PRInt32 which, PRBool *enabled)
{
    SECStatus  rv;
    sslSocket *ss = ssl_FindSocket(fd);
    
    if (!enabled) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in CipherPrefGet", SSL_GETPID(), fd));
	PORT_SetError(PR_BAD_DESCRIPTOR_ERROR);
	*enabled = PR_FALSE;
	return SECFailure;
    }
    if (SSL_IS_SSL2_CIPHER(which)) {
	rv = ssl2_CipherPrefGet(ss, which, enabled);
    } else {
	rv = ssl3_CipherPrefGet(ss, (ssl3CipherSuite)which, enabled);
    }
    return rv;
}

SECStatus
NSS_SetDomesticPolicy(void)
{
#ifndef EXPORT_VERSION
    SECStatus      status = SECSuccess;
    cipherPolicy * policy;

    for (policy = ssl_ciphers; policy->cipher != 0; ++policy) {
	status = SSL_SetPolicy(policy->cipher, SSL_ALLOWED);
	if (status != SECSuccess)
	    break;
    }
    return status;
#else
    return NSS_SetExportPolicy();
#endif
}

SECStatus
NSS_SetExportPolicy(void)
{
    SECStatus      status = SECSuccess;
    cipherPolicy * policy;

    for (policy = ssl_ciphers; policy->cipher != 0; ++policy) {
	status = SSL_SetPolicy(policy->cipher, policy->export);
	if (status != SECSuccess)
	    break;
    }
    return status;
}

SECStatus
NSS_SetFrancePolicy(void)
{
    SECStatus      status = SECSuccess;
    cipherPolicy * policy;

    for (policy = ssl_ciphers; policy->cipher != 0; ++policy) {
	status = SSL_SetPolicy(policy->cipher, policy->france);
	if (status != SECSuccess)
	    break;
    }
    return status;
}



/* LOCKS ??? XXX */
PRFileDesc *
SSL_ImportFD(PRFileDesc *model, PRFileDesc *fd)
{
    sslSocket * ns = NULL;
    PRStatus    rv;

    if (model == NULL) {
	/* Just create a default socket if we're given NULL for the model */
	ns = ssl_NewSocket();
    } else {
	sslSocket * ss = ssl_FindSocket(model);
	if (ss == NULL) {
	    SSL_DBG(("%d: SSL[%d]: bad model socket in ssl_ImportFD", 
	    	      SSL_GETPID(), model));
	    SET_ERROR_CODE
	    return NULL;
	}
	ns = ssl_DupSocket(ss);
    }
    if (ns == NULL)
    	return NULL;

    rv = ssl_PushIOLayer(ns, fd, PR_TOP_IO_LAYER);
    if (rv != PR_SUCCESS) {
	ssl_FreeSocket(ns);
	SET_ERROR_CODE
	return NULL;
    }
#ifdef _WIN32
    PR_Sleep(PR_INTERVAL_NO_WAIT);     /* workaround NT winsock connect bug. */
#endif
    return fd;
}

/************************************************************************/
/* The following functions are the TOP LEVEL SSL functions.
** They all get called through the NSPRIOMethods table below.
*/

static PRFileDesc * PR_CALLBACK
ssl_Accept(PRFileDesc *fd, PRNetAddr *sockaddr, PRIntervalTime timeout)
{
    sslSocket  *ss;
    sslSocket  *ns 	= NULL;
    PRFileDesc *newfd 	= NULL;
    PRFileDesc *osfd;
    PRStatus    status;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in accept", SSL_GETPID(), fd));
	return NULL;
    }

    /* IF this is a listen socket, there shouldn't be any I/O going on */
    SSL_LOCK_READER(ss);
    SSL_LOCK_WRITER(ss);
    ssl_Get1stHandshakeLock(ss);
    ssl_GetSSL3HandshakeLock(ss);

    ss->cTimeout = timeout;

    osfd = ss->fd->lower;

    /* First accept connection */
    newfd = osfd->methods->accept(osfd, sockaddr, timeout);
    if (newfd == NULL) {
	SSL_DBG(("%d: SSL[%d]: accept failed, errno=%d",
		 SSL_GETPID(), ss->fd, PORT_GetError()));
    } else {
	/* Create ssl module */
	ns = ssl_DupSocket(ss);
    }

    ssl_ReleaseSSL3HandshakeLock(ss);
    ssl_Release1stHandshakeLock(ss);
    SSL_UNLOCK_WRITER(ss);
    SSL_UNLOCK_READER(ss);			/* ss isn't used below here. */

    if (ns == NULL)
	goto loser;

    /* push ssl module onto the new socket */
    status = ssl_PushIOLayer(ns, newfd, PR_TOP_IO_LAYER);
    if (status != PR_SUCCESS)
	goto loser;

    /* Now start server connection handshake with client.
    ** Don't need locks here because nobody else has a reference to ns yet.
    */
    if ( ns->useSecurity ) {
	if ( ns->handshakeAsClient ) {
	    ns->handshake = ssl2_BeginClientHandshake;
	} else {
	    ns->handshake = ssl2_BeginServerHandshake;
	}
    }
    return newfd;

loser:
    if (ns != NULL)
	ssl_FreeSocket(ns);
    if (newfd != NULL)
	PR_Close(newfd);
    return NULL;
}

static PRStatus PR_CALLBACK
ssl_Connect(PRFileDesc *fd, const PRNetAddr *sockaddr, PRIntervalTime timeout)
{
    sslSocket *ss;
    PRStatus   rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in connect", SSL_GETPID(), fd));
	return PR_FAILURE;
    }

    /* IF this is a listen socket, there shouldn't be any I/O going on */
    SSL_LOCK_READER(ss);
    SSL_LOCK_WRITER(ss);

    ss->cTimeout = timeout;
    rv = (PRStatus)(*ss->ops->connect)(ss, sockaddr);
#ifdef _WIN32
    PR_Sleep(PR_INTERVAL_NO_WAIT);     /* workaround NT winsock connect bug. */
#endif

    SSL_UNLOCK_WRITER(ss);
    SSL_UNLOCK_READER(ss);

    return rv;
}

static PRStatus PR_CALLBACK
ssl_Bind(PRFileDesc *fd, const PRNetAddr *addr)
{
    sslSocket * ss = ssl_GetPrivate(fd);
    PRStatus    rv;

    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in bind", SSL_GETPID(), fd));
	return PR_FAILURE;
    }
    SSL_LOCK_READER(ss);
    SSL_LOCK_WRITER(ss);

    rv = (PRStatus)(*ss->ops->bind)(ss, addr);

    SSL_UNLOCK_WRITER(ss);
    SSL_UNLOCK_READER(ss);
    return rv;
}

static PRStatus PR_CALLBACK
ssl_Listen(PRFileDesc *fd, PRIntn backlog)
{
    sslSocket * ss = ssl_GetPrivate(fd);
    PRStatus    rv;

    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in listen", SSL_GETPID(), fd));
	return PR_FAILURE;
    }
    SSL_LOCK_READER(ss);
    SSL_LOCK_WRITER(ss);

    rv = (PRStatus)(*ss->ops->listen)(ss, backlog);

    SSL_UNLOCK_WRITER(ss);
    SSL_UNLOCK_READER(ss);
    return rv;
}

static PRStatus PR_CALLBACK
ssl_Shutdown(PRFileDesc *fd, PRIntn how)
{
    sslSocket * ss = ssl_GetPrivate(fd);
    PRStatus    rv;

    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in shutdown", SSL_GETPID(), fd));
	return PR_FAILURE;
    }
    if (how == PR_SHUTDOWN_RCV || how == PR_SHUTDOWN_BOTH) {
    	SSL_LOCK_READER(ss);
    }
    if (how == PR_SHUTDOWN_SEND || how == PR_SHUTDOWN_BOTH) {
    	SSL_LOCK_WRITER(ss);
    }

    rv = (PRStatus)(*ss->ops->shutdown)(ss, how);

    if (how == PR_SHUTDOWN_SEND || how == PR_SHUTDOWN_BOTH) {
    	SSL_UNLOCK_WRITER(ss);
    }
    if (how == PR_SHUTDOWN_RCV || how == PR_SHUTDOWN_BOTH) {
    	SSL_UNLOCK_READER(ss);
    }
    return rv;
}

static PRStatus PR_CALLBACK
ssl_Close(PRFileDesc *fd)
{
    sslSocket *ss;
    PRStatus   rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in close", SSL_GETPID(), fd));
	return PR_FAILURE;
    }

    /* There must not be any I/O going on */
    SSL_LOCK_READER(ss);
    SSL_LOCK_WRITER(ss);

    /* By the time this function returns, 
    ** ss is an invalid pointer, and the locks to which it points have 
    ** been unlocked and freed.  So, this is the ONE PLACE in all of SSL
    ** where the LOCK calls and the corresponding UNLOCK calls are not in
    ** the same function scope.  The unlock calls are in ssl_FreeSocket().
    */
    rv = (PRStatus)(*ss->ops->close)(ss);

    return rv;
}

static int PR_CALLBACK
ssl_Recv(PRFileDesc *fd, void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
    sslSocket *ss;
    int        rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in recv", SSL_GETPID(), fd));
	return SECFailure;
    }
    SSL_LOCK_READER(ss);
    ss->rTimeout = timeout;
    rv = (*ss->ops->recv)(ss, (unsigned char*)buf, len, flags);
    SSL_UNLOCK_READER(ss);
    return rv;
}

static int PR_CALLBACK
ssl_Send(PRFileDesc *fd, const void *buf, PRInt32 len, PRIntn flags,
	 PRIntervalTime timeout)
{
    sslSocket *ss;
    int        rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in send", SSL_GETPID(), fd));
	return SECFailure;
    }
    SSL_LOCK_WRITER(ss);
    ss->wTimeout = timeout;
    rv = (*ss->ops->send)(ss, (const unsigned char*)buf, len, flags);
    SSL_UNLOCK_WRITER(ss);
    return rv;
}

static int PR_CALLBACK
ssl_Read(PRFileDesc *fd, void *buf, PRInt32 len)
{
    sslSocket *ss;
    int        rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in read", SSL_GETPID(), fd));
	return SECFailure;
    }
    SSL_LOCK_READER(ss);
    ss->rTimeout = PR_INTERVAL_NO_TIMEOUT;
    rv = (*ss->ops->read)(ss, (unsigned char*)buf, len);
    SSL_UNLOCK_READER(ss);
    return rv;
}

static int PR_CALLBACK
ssl_Write(PRFileDesc *fd, const void *buf, PRInt32 len)
{
    sslSocket *ss;
    int        rv;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in write", SSL_GETPID(), fd));
	return SECFailure;
    }
    SSL_LOCK_WRITER(ss);
    ss->wTimeout = PR_INTERVAL_NO_TIMEOUT;
    rv = (*ss->ops->write)(ss, (const unsigned char*)buf, len);
    SSL_UNLOCK_WRITER(ss);
    return rv;
}

static PRStatus PR_CALLBACK
ssl_GetPeerName(PRFileDesc *fd, PRNetAddr *addr)
{
    sslSocket *ss;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in getpeername", SSL_GETPID(), fd));
	return PR_FAILURE;
    }
    return (PRStatus)(*ss->ops->getpeername)(ss, addr);
}

/*
** XXX this code doesn't work properly inside a Socks server.
*/
SECStatus
ssl_GetPeerInfo(sslSocket *ss)
{
    sslConnectInfo *  ci;
    PRNetAddr         sin;
    int               rv;
    PRFileDesc *      osfd;

    PORT_Assert((ss->sec != 0));

    osfd = ss->fd->lower;
    ci   = &ss->sec->ci;

    /* If ssl_SocksConnect() has previously recorded the peer's IP & port,
     * use that.
     */
    if ((ss->port != 0) &&
	((ss->peer.pr_s6_addr32[0] != 0) || (ss->peer.pr_s6_addr32[1] != 0) ||
	 (ss->peer.pr_s6_addr32[2] != 0) || (ss->peer.pr_s6_addr32[3] != 0))) {
	/* SOCKS code has already recorded the peer's IP addr and port.
	 * (NOT the proxy's addr and port) in ss->peer & port.
	 */
	ci->peer = ss->peer;
	ci->port = ss->port;
	return SECSuccess;
    }

    PORT_Memset(&sin, 0, sizeof(sin));
    rv = osfd->methods->getpeername(osfd, &sin);
    if (rv < 0) {
	return SECFailure;
    }
    /* we have to mask off the high byte because AIX is lame */
    if ((sin.inet.family & 0xff) == PR_AF_INET) {
        PR_ConvertIPv4AddrToIPv6(sin.inet.ip, &ci->peer);
	ci->port = sin.inet.port;
    } else {
        PORT_Assert(sin.ipv6.family == PR_AF_INET6);
	ci->peer = sin.ipv6.ip;
	ci->port = sin.ipv6.port;
    }
    return SECSuccess;
}

static PRStatus PR_CALLBACK
ssl_GetSockName(PRFileDesc *fd, PRNetAddr *name)
{
    sslSocket *ss;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in getsockname", SSL_GETPID(), fd));
	return PR_FAILURE;
    }
    return (PRStatus)(*ss->ops->getsockname)(ss, name);
}

int PR_CALLBACK
SSL_SetSockPeerID(PRFileDesc *fd, char *peerID)
{
    sslSocket *ss;

    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SSL_SetCacheIndex",
		 SSL_GETPID(), fd));
	return SECFailure;
    }

    ss->peerID = PORT_Strdup(peerID);
    return 0;
}

static PRInt16 PR_CALLBACK
ssl_Poll(PRFileDesc *fd, PRInt16 how_flags, PRInt16 *out_flags)
{
    sslSocket *ss;
    PRInt16    ret_flags = how_flags;	/* should select on these flags. */

    *out_flags = 0;
    ss = ssl_GetPrivate(fd);
    if (!ss) {
	SSL_DBG(("%d: SSL[%d]: bad socket in SSL_Poll",
		 SSL_GETPID(), fd));
	return 0;	/* don't poll on this socket */
    }

    if ((ret_flags & PR_POLL_WRITE) && 
        ( (ss->useSocks && ss->handshake) || 
	  (ss->useSecurity && !ss->connected && 
	   /* XXX There needs to be a better test than the following. */
	   /* Don't check ss->securityHandshake. */
	   (ss->handshake || ss->nextHandshake)))) {
    	/* The user is trying to write, but the handshake is blocked waiting
	 * to read, so tell NSPR NOT to poll on write.
	 */
	ret_flags ^=  PR_POLL_WRITE;	/* don't select on write. */
	ret_flags |=  PR_POLL_READ;	/* do    select on read. */
    }

    if ((ret_flags & PR_POLL_READ) && (SSL_DataPending(fd) > 0)) {
	*out_flags = PR_POLL_READ;	/* it's ready already. */

    } else if (ret_flags && (fd->lower->methods->poll != NULL)) {
        ret_flags = fd->lower->methods->poll(fd->lower, ret_flags, out_flags);
    }

    return ret_flags;
}


PRBool
ssl_FdIsBlocking(PRFileDesc *fd)
{
    PRSocketOptionData opt;
    PRStatus           status;

    opt.option             = PR_SockOpt_Nonblocking;
    opt.value.non_blocking = PR_FALSE;
    status = PR_GetSocketOption(fd, &opt);
    if (status != PR_SUCCESS)
	return PR_FALSE;
    return (PRBool)!opt.value.non_blocking;
}

PRBool
ssl_SocketIsBlocking(sslSocket *ss)
{
    return ssl_FdIsBlocking(ss->fd);
}

PRInt32  sslFirstBufSize = 8 * 1024;
PRInt32  sslCopyLimit    = 1024;

static PRInt32 PR_CALLBACK
ssl_WriteV(PRFileDesc *fd, const PRIOVec *iov, PRInt32 vectors, 
           PRIntervalTime timeout)
{
    PRInt32            bufLen;
    PRInt32            left;
    PRInt32            rv;
    PRInt32            sent      =  0;
    const PRInt32      first_len = sslFirstBufSize;
    const PRInt32      limit     = sslCopyLimit;
    PRBool             blocking;
    PRIOVec            myIov	 = { 0, 0 };
    char               buf[MAX_FRAGMENT_LENGTH];

    if (vectors > PR_MAX_IOVECTOR_SIZE) {
    	PORT_SetError(PR_BUFFER_OVERFLOW_ERROR);
	return -1;
    }
    blocking = ssl_FdIsBlocking(fd);

#define K16 sizeof(buf)
#define KILL_VECTORS while (vectors && !iov->iov_len) { ++iov; --vectors; }
#define GET_VECTOR   do { myIov = *iov++; --vectors; KILL_VECTORS } while (0)
#define HANDLE_ERR(rv, len) \
    if (rv != len) { \
	if (rv < 0) { \
	    if (blocking \
		&& (PR_GetError() == PR_WOULD_BLOCK_ERROR) \
		&& (sent > 0)) { \
		return sent; \
	    } else { \
		return -1; \
	    } \
	} \
	/* Only a nonblocking socket can have partial sends */ \
	PR_ASSERT(blocking); \
	return sent; \
    } 
#define SEND(bfr, len) \
    do { \
	rv = ssl_Send(fd, bfr, len, 0, timeout); \
	HANDLE_ERR(rv, len) \
	sent += len; \
    } while (0)

    /* Make sure the first write is at least 8 KB, if possible. */
    KILL_VECTORS
    if (!vectors)
	return 0;
    GET_VECTOR;
    if (!vectors) {
	return ssl_Send(fd, myIov.iov_base, myIov.iov_len, 0, timeout);
    }
    if (myIov.iov_len < first_len) {
	PORT_Memcpy(buf, myIov.iov_base, myIov.iov_len);
	bufLen = myIov.iov_len;
	left = first_len - bufLen;
	while (vectors && left) {
	    int toCopy;
	    GET_VECTOR;
	    toCopy = PR_MIN(left, myIov.iov_len);
	    PORT_Memcpy(buf + bufLen, myIov.iov_base, toCopy);
	    bufLen         += toCopy;
	    left           -= toCopy;
	    myIov.iov_base += toCopy;
	    myIov.iov_len  -= toCopy;
	}
	SEND( buf, bufLen );
    }

    while (vectors || myIov.iov_len) {
	PRInt32   addLen;
	if (!myIov.iov_len) {
	    GET_VECTOR;
	}
	while (myIov.iov_len >= K16) {
	    SEND(myIov.iov_base, K16);
	    myIov.iov_base += K16;
	    myIov.iov_len  -= K16;
	}
	if (!myIov.iov_len)
	    continue;

	if (!vectors || myIov.iov_len > limit) {
	    addLen = 0;
	} else if ((addLen = iov->iov_len % K16) + myIov.iov_len <= limit) {
	    /* Addlen is already computed. */;
	} else if (vectors > 1 && 
	     iov[1].iov_len % K16 + addLen + myIov.iov_len <= 2 * limit) {
	     addLen = limit - myIov.iov_len;
	} else 
	    addLen = 0;

	if (!addLen) {
	    SEND( myIov.iov_base, myIov.iov_len );
	    myIov.iov_len = 0;
	    continue;
	}
	PORT_Memcpy(buf, myIov.iov_base, myIov.iov_len);
	bufLen = myIov.iov_len;
	do {
	    GET_VECTOR;
	    PORT_Memcpy(buf + bufLen, myIov.iov_base, addLen);
	    myIov.iov_base += addLen;
	    myIov.iov_len  -= addLen;
	    bufLen         += addLen;

	    left = PR_MIN( limit, K16 - bufLen);
	    if (!vectors 		/* no more left */
	    ||  myIov.iov_len > 0	/* we didn't use that one all up */
	    ||  bufLen >= K16		/* it's full. */
	    ) {
		addLen = 0;
	    } else if ((addLen = iov->iov_len % K16) <= left) {
		/* Addlen is already computed. */;
	    } else if (vectors > 1 && 
		 iov[1].iov_len % K16 + addLen <= left + limit) {
		 addLen = left;
	    } else 
		addLen = 0;

	} while (addLen);
	SEND( buf, bufLen );
    } 
    return sent;
}

/*
 * These functions aren't implemented.
 */

static PRInt32 PR_CALLBACK
ssl_Available(PRFileDesc *fd)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt64 PR_CALLBACK
ssl_Available64(PRFileDesc *fd)
{
    PRInt64 res;

    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    LL_I2L(res, -1L);
    return res;
}

static PRStatus PR_CALLBACK
ssl_FSync(PRFileDesc *fd)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

static PRInt32 PR_CALLBACK
ssl_Seek(PRFileDesc *fd, PRInt32 offset, PRSeekWhence how) {
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt64 PR_CALLBACK
ssl_Seek64(PRFileDesc *fd, PRInt64 offset, PRSeekWhence how) {
    PRInt64 res;

    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    LL_I2L(res, -1L);
    return res;
}

static PRStatus PR_CALLBACK
ssl_FileInfo(PRFileDesc *fd, PRFileInfo *info)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

static PRStatus PR_CALLBACK
ssl_FileInfo64(PRFileDesc *fd, PRFileInfo64 *info)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

static PRInt32 PR_CALLBACK
ssl_RecvFrom(PRFileDesc *fd, void *buf, PRInt32 amount, PRIntn flags,
	     PRNetAddr *addr, PRIntervalTime timeout)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static PRInt32 PR_CALLBACK
ssl_SendTo(PRFileDesc *fd, const void *buf, PRInt32 amount, PRIntn flags,
	   const PRNetAddr *addr, PRIntervalTime timeout)
{
    PORT_Assert(0);
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return SECFailure;
}

static const PRIOMethods ssl_methods = {
    PR_DESC_LAYERED,
    ssl_Close,           	/* close        */
    ssl_Read,            	/* read         */
    ssl_Write,           	/* write        */
    ssl_Available,       	/* available    */
    ssl_Available64,     	/* available64  */
    ssl_FSync,           	/* fsync        */
    ssl_Seek,            	/* seek         */
    ssl_Seek64,          	/* seek64       */
    ssl_FileInfo,        	/* fileInfo     */
    ssl_FileInfo64,      	/* fileInfo64   */
    ssl_WriteV,          	/* writev       */
    ssl_Connect,         	/* connect      */
    ssl_Accept,          	/* accept       */
    ssl_Bind,            	/* bind         */
    ssl_Listen,          	/* listen       */
    ssl_Shutdown,        	/* shutdown     */
    ssl_Recv,            	/* recv         */
    ssl_Send,            	/* send         */
    ssl_RecvFrom,        	/* recvfrom     */
    ssl_SendTo,          	/* sendto       */
    ssl_Poll,            	/* poll         */
    ssl_EmulateAcceptRead,      /* acceptread   */
    ssl_EmulateTransmitFile,    /* transmitfile */
    ssl_GetSockName,     	/* getsockname  */
    ssl_GetPeerName,     	/* getpeername  */
    NULL,                	/* getsockopt   OBSOLETE */
    NULL,                	/* setsockopt   OBSOLETE */
    NULL,                	/* getsocketoption   */
    NULL,                	/* setsocketoption   */
    ssl_EmulateSendFile, 	/* Send a (partial) file with header/trailer*/
    NULL,                	/* reserved for future use */
    NULL,                	/* reserved for future use */
    NULL,                	/* reserved for future use */
    NULL,                	/* reserved for future use */
    NULL                 	/* reserved for future use */
};


static PRIOMethods combined_methods;

static void
ssl_SetupIOMethods(void)
{
          PRIOMethods *new_methods  = &combined_methods;
    const PRIOMethods *nspr_methods = PR_GetDefaultIOMethods();
    const PRIOMethods *my_methods   = &ssl_methods;

    *new_methods = *nspr_methods;

    new_methods->file_type         = my_methods->file_type;
    new_methods->close             = my_methods->close;
    new_methods->read              = my_methods->read;
    new_methods->write             = my_methods->write;
    new_methods->available         = my_methods->available;
    new_methods->available64       = my_methods->available64;
    new_methods->fsync             = my_methods->fsync;
    new_methods->seek              = my_methods->seek;
    new_methods->seek64            = my_methods->seek64;
    new_methods->fileInfo          = my_methods->fileInfo;
    new_methods->fileInfo64        = my_methods->fileInfo64;
    new_methods->writev            = my_methods->writev;
    new_methods->connect           = my_methods->connect;
    new_methods->accept            = my_methods->accept;
    new_methods->bind              = my_methods->bind;
    new_methods->listen            = my_methods->listen;
    new_methods->shutdown          = my_methods->shutdown;
    new_methods->recv              = my_methods->recv;
    new_methods->send              = my_methods->send;
    new_methods->recvfrom          = my_methods->recvfrom;
    new_methods->sendto            = my_methods->sendto;
    new_methods->poll              = my_methods->poll;
    new_methods->acceptread        = my_methods->acceptread;
    new_methods->transmitfile      = my_methods->transmitfile;
    new_methods->getsockname       = my_methods->getsockname;
    new_methods->getpeername       = my_methods->getpeername;
/*  new_methods->getsocketoption   = my_methods->getsocketoption;	*/
/*  new_methods->setsocketoption   = my_methods->setsocketoption;	*/
    new_methods->sendfile          = my_methods->sendfile;

}

static PRCallOnceType initIoLayerOnce;

static PRStatus  
ssl_InitIOLayer(void)
{
    ssl_layer_id = PR_GetUniqueIdentity("SSL");
    ssl_SetupIOMethods();
    ssl_inited = PR_TRUE;
    return PR_SUCCESS;
}

static PRStatus
ssl_PushIOLayer(sslSocket *ns, PRFileDesc *stack, PRDescIdentity id)
{
    PRFileDesc *layer	= NULL;
    PRStatus    status;

    if (!ssl_inited) {
	PR_CallOnce(&initIoLayerOnce, &ssl_InitIOLayer);
    }

    if (ns == NULL)
	goto loser;

    layer = PR_CreateIOLayerStub(ssl_layer_id, &combined_methods);
    if (layer == NULL)
	goto loser;
    layer->secret = (PRFilePrivate *)ns;

    /* Here, "stack" points to the PRFileDesc on the top of the stack.
    ** "layer" points to a new FD that is to be inserted into the stack.
    ** If layer is being pushed onto the top of the stack, then 
    ** PR_PushIOLayer switches the contents of stack and layer, and then
    ** puts stack on top of layer, so that after it is done, the top of 
    ** stack is the same "stack" as it was before, and layer is now the 
    ** FD for the former top of stack.
    ** After this call, stack always points to the top PRFD on the stack.
    ** If this function fails, the contents of stack and layer are as 
    ** they were before the call.
    */
    status = PR_PushIOLayer(stack, id, layer);
    if (status != PR_SUCCESS)
	goto loser;

    ns->fd = (id == PR_TOP_IO_LAYER) ? stack : layer;
    return PR_SUCCESS;

loser:
    if (layer) {
	layer->dtor(layer); /* free layer */
    }
    return PR_FAILURE;
}

/*
** Create a newsocket structure for a file descriptor.
*/
static sslSocket *
ssl_NewSocket(void)
{
    sslSocket *ss;
#ifdef DEBUG
    static int firsttime = 1;
#endif

#ifdef DEBUG
#if defined(XP_UNIX) || defined(XP_WIN32)
    if (firsttime) {
	firsttime = 0;

	{
	    char *ev = getenv("SSLDEBUG");
	    if (ev && ev[0]) {
		ssl_debug = atoi(ev);
		SSL_TRACE(("SSL: debugging set to %d", ssl_debug));
	    }
	}
#ifdef TRACE
	{
	    char *ev = getenv("SSLTRACE");
	    if (ev && ev[0]) {
		ssl_trace = atoi(ev);
		SSL_TRACE(("SSL: tracing set to %d", ssl_trace));
	    }
	}
#endif /* TRACE */
    }
#endif /* XP_UNIX || XP_WIN32 */
#endif /* DEBUG */

    /* Make a new socket and get it ready */
    ss = (sslSocket*) PORT_ZAlloc(sizeof(sslSocket));
    if (ss) {
        /* This should be of type SSLKEAType, but CC on IRIX
	 * complains during the for loop.
	 */
	int i;
 
	ss->useSecurity        = ssl_defaults.useSecurity;
	ss->useSocks           = ssl_defaults.useSocks;
	ss->requestCertificate = ssl_defaults.requestCertificate;
	ss->requireCertificate = ssl_defaults.requireCertificate;
	ss->handshakeAsClient  = ssl_defaults.handshakeAsClient;
	ss->handshakeAsServer  = ssl_defaults.handshakeAsServer;
	ss->enableSSL2         = ssl_defaults.enableSSL2;
	ss->enableSSL3         = ssl_defaults.enableSSL3;
	ss->enableTLS          = ssl_defaults.enableTLS ;
	ss->fdx                = ssl_defaults.fdx;
	ss->v2CompatibleHello  = ssl_defaults.v2CompatibleHello;
	ss->detectRollBack     = ssl_defaults.detectRollBack;
	memset(&ss->peer, 0, sizeof(ss->peer));
	ss->port               = 0;
	ss->noCache            = ssl_defaults.noCache;
	ss->peerID             = NULL;
	ss->rTimeout	       = PR_INTERVAL_NO_TIMEOUT;
	ss->wTimeout	       = PR_INTERVAL_NO_TIMEOUT;
	ss->cTimeout	       = PR_INTERVAL_NO_TIMEOUT;
	ss->cipherSpecs        = NULL;
        ss->sizeCipherSpecs    = 0;  /* produced lazily */
        ss->preferredCipher    = NULL;
        ss->url                = NULL;

	for (i=kt_null; i < kt_kea_size; i++) {
	    ss->serverCert[i]      = NULL;
	    ss->serverCertChain[i] = NULL;
	    ss->serverKey[i]       = NULL;
	}
	ss->stepDownKeyPair    = NULL;
	ss->dbHandle           = CERT_GetDefaultCertDB();

	/* Provide default implementation of hooks */
	ss->authCertificate    = SSL_AuthCertificate;
	ss->authCertificateArg = (void *)ss->dbHandle;
	ss->getClientAuthData  = NULL;
	ss->handleBadCert      = NULL;
	ss->badCertArg         = NULL;
	ss->pkcs11PinArg       = NULL;

	ssl_ChooseOps(ss);
	ssl2_InitSocketPolicy(ss);
	ssl3_InitSocketPolicy(ss);

	ss->firstHandshakeLock = PZ_NewMonitor(nssILockSSL);
	ss->ssl3HandshakeLock  = PZ_NewMonitor(nssILockSSL);
	ss->specLock           = NSSRWLock_New(SSL_LOCK_RANK_SPEC, NULL);
	ss->recvBufLock        = PZ_NewMonitor(nssILockSSL);
	ss->xmitBufLock        = PZ_NewMonitor(nssILockSSL);
	if (ssl_lock_readers) {
	    ss->recvLock       = PZ_NewLock(nssILockSSL);
	    ss->sendLock       = PZ_NewLock(nssILockSSL);
	}
    }
    return ss;
}

