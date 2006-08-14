/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
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
/*
 * This file implements PKCS 11 on top of our existing security modules
 *
 * For more information about PKCS 11 See PKCS 11 Token Inteface Standard.
 *   This implementation has two slots:
 *	slot 1 is our generic crypto support. It does not require login
 *   (unless you've enabled FIPS). It supports Public Key ops, and all they
 *   bulk ciphers and hashes. It can also support Private Key ops for imported
 *   Private keys. It does not have any token storage.
 *	slot 2 is our private key support. It requires a login before use. It
 *   can store Private Keys and Certs as token objects. Currently only private
 *   keys and their associated Certificates are saved on the token.
 *
 *   In this implementation, session objects are only visible to the session
 *   that created or generated them.
 */
#include "seccomon.h"
#include "softoken.h"
#include "lowkeyi.h"
#include "pcert.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "prenv.h"
#include "prprf.h"

#include <ctype.h>

#ifdef XP_UNIX
#define NSS_AUDIT_WITH_SYSLOG 1
#include <syslog.h>
#include <unistd.h>
#endif

#ifdef SOLARIS
#include <bsm/libbsm.h>
#define AUE_FIPS_AUDIT 34444
#endif

#ifdef LINUX
#include <pthread.h>
#include <dlfcn.h>
#define LIBAUDIT_NAME "libaudit.so.0"
#ifndef AUDIT_USER
#define AUDIT_USER 1005  /* message type: message from userspace */
#endif
static void *libaudit_handle;
static int (*audit_open_func)(void);
static void (*audit_close_func)(int fd);
static int (*audit_log_user_message_func)(int audit_fd, int type,
    const char *message, const char *hostname, const char *addr,
    const char *tty, int result);

static pthread_once_t libaudit_once_control = PTHREAD_ONCE_INIT;

static void
libaudit_init(void)
{
    libaudit_handle = dlopen(LIBAUDIT_NAME, RTLD_LAZY);
    if (!libaudit_handle) {
	return;
    }
    audit_open_func = dlsym(libaudit_handle, "audit_open");
    audit_close_func = dlsym(libaudit_handle, "audit_close");
    audit_log_user_message_func = dlsym(libaudit_handle,
					"audit_log_user_message");
}
#endif /* LINUX */


/*
 * ******************** Password Utilities *******************************
 */
static PRBool isLoggedIn = PR_FALSE;
PRBool sftk_fatalError = PR_FALSE;

/*
 * This function returns
 *   - CKR_PIN_INVALID if the password/PIN is not a legal UTF8 string
 *   - CKR_PIN_LEN_RANGE if the password/PIN is too short or does not
 *     consist of characters from three or more character classes.
 *   - CKR_OK otherwise
 *
 * The minimum password/PIN length is FIPS_MIN_PIN Unicode characters.
 * We define five character classes: digits (0-9), ASCII lowercase letters,
 * ASCII uppercase letters, ASCII non-alphanumeric characters (such as
 * space and punctuation marks), and non-ASCII characters.  If an ASCII
 * uppercase letter is the first character of the password/PIN, the
 * uppercase letter is not counted toward its character class.  Similarly,
 * if a digit is the last character of the password/PIN, the digit is not
 * counted toward its character class.
 *
 * Although NSC_SetPIN and NSC_InitPIN already do the maximum and minimum
 * password/PIN length checks, they check the length in bytes as opposed
 * to characters.  To meet the minimum password/PIN guessing probability
 * requirements in FIPS 140-2, we need to check the length in characters.
 */
static CK_RV sftk_newPinCheck(CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    unsigned int i;
    int nchar = 0;      /* number of characters */
    int ntrail = 0;     /* number of trailing bytes to follow */
    int ndigit = 0;     /* number of decimal digits */
    int nlower = 0;     /* number of ASCII lowercase letters */
    int nupper = 0;     /* number of ASCII uppercase letters */
    int nnonalnum = 0;  /* number of ASCII non-alphanumeric characters */
    int nnonascii = 0;  /* number of non-ASCII characters */
    int nclass;         /* number of character classes */

    for (i = 0; i < ulPinLen; i++) {
	unsigned int byte = pPin[i];

	if (ntrail) {
	    if ((byte & 0xc0) != 0x80) {
		/* illegal */
		nchar = -1;
		break;
	    }
	    if (--ntrail == 0) {
		nchar++;
		nnonascii++;
	    }
	    continue;
	}
	if ((byte & 0x80) == 0x00) {
	    /* single-byte (ASCII) character */
	    nchar++;
	    if (isdigit(byte)) {
		if (i < ulPinLen - 1) {
		    ndigit++;
		}
	    } else if (islower(byte)) {
		nlower++;
	    } else if (isupper(byte)) {
		if (i > 0) {
		    nupper++;
		}
	    } else {
		nnonalnum++;
	    }
	} else if ((byte & 0xe0) == 0xc0) {
	    /* leading byte of two-byte character */
	    ntrail = 1;
	} else if ((byte & 0xf0) == 0xe0) {
	    /* leading byte of three-byte character */
	    ntrail = 2;
	} else if ((byte & 0xf8) == 0xf0) {
	    /* leading byte of four-byte character */
	    ntrail = 3;
	} else {
	    /* illegal */
	    nchar = -1;
	    break;
	}
    }
    if (nchar == -1) {
	/* illegal UTF8 string */
	return CKR_PIN_INVALID;
    }
    if (nchar < FIPS_MIN_PIN) {
	return CKR_PIN_LEN_RANGE;
    }
    nclass = (ndigit != 0) + (nlower != 0) + (nupper != 0) +
	     (nnonalnum != 0) + (nnonascii != 0);
    if (nclass < 3) {
	return CKR_PIN_LEN_RANGE;
    }
    return CKR_OK;
}


/* FIPS required checks before any useful cryptographic services */
static CK_RV sftk_fipsCheck(void) {
    if (sftk_fatalError) 
	return CKR_DEVICE_ERROR;
    if (!isLoggedIn) 
	return CKR_USER_NOT_LOGGED_IN;
    return CKR_OK;
}


#define SFTK_FIPSCHECK() \
    CK_RV rv; \
    if ((rv = sftk_fipsCheck()) != CKR_OK) return rv;

#define SFTK_FIPSFATALCHECK() \
    if (sftk_fatalError) return CKR_DEVICE_ERROR;


/* grab an attribute out of a raw template */
void *
fc_getAttribute(CK_ATTRIBUTE_PTR pTemplate, 
				CK_ULONG ulCount, CK_ATTRIBUTE_TYPE type)
{
    int i;

    for (i=0; i < (int) ulCount; i++) {
	if (pTemplate[i].type == type) {
	    return pTemplate[i].pValue;
	}
    }
    return NULL;
}


#define __PASTE(x,y)	x##y

/* ------------- forward declare all the NSC_ functions ------------- */
#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) CK_RV __PASTE(NS,name)
#define CK_NEED_ARG_LIST 1

#include "pkcs11f.h"

/* ------------- forward declare all the FIPS functions ------------- */
#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) CK_RV __PASTE(F,name)
#define CK_NEED_ARG_LIST 1

#include "pkcs11f.h"

/* ------------- build the CK_CRYPTO_TABLE ------------------------- */
static CK_FUNCTION_LIST sftk_fipsTable = {
    { 1, 10 },

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO

#define CK_PKCS11_FUNCTION_INFO(name) __PASTE(F,name),


#include "pkcs11f.h"

};

#undef CK_NEED_ARG_LIST
#undef CK_PKCS11_FUNCTION_INFO


#undef __PASTE

static CK_RV
fips_login_if_key_object(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject)
{
    CK_RV rv;
    CK_OBJECT_CLASS objClass;
    CK_ATTRIBUTE class; 
    class.type = CKA_CLASS;
    class.pValue = &objClass;
    class.ulValueLen = sizeof(objClass);
    rv = NSC_GetAttributeValue(hSession, hObject, &class, 1);
    if (rv == CKR_OK) {
	if ((objClass == CKO_PRIVATE_KEY) || (objClass == CKO_SECRET_KEY)) {
	    rv = sftk_fipsCheck();
	}
    }
    return rv;
}

/**********************************************************************
 *
 *     FIPS 140 auditable event logging
 *
 **********************************************************************/

PRBool sftk_audit_enabled = PR_FALSE;

/*
 * Each audit record must have the following information:
 * - Date and time of the event
 * - Type of event
 * - user (subject) identity
 * - outcome (success or failure) of the event
 * - process ID
 * - name (ID) of the object
 * - for changes to data (except for authentication data and CSPs), the new
 *   and old values of the data
 * - for authentication attempts, the origin of the attempt (e.g., terminal
 *   identifier)
 * - for assuming a role, the type of role, and the location of the request
 */
void
sftk_LogAuditMessage(NSSAuditSeverity severity, const char *msg)
{
#ifdef NSS_AUDIT_WITH_SYSLOG
    int level;

    switch (severity) {
    case NSS_AUDIT_ERROR:
	level = LOG_ERR;
	break;
    case NSS_AUDIT_WARNING:
	level = LOG_WARNING;
	break;
    default:
	level = LOG_INFO;
	break;
    }
    /* timestamp is provided by syslog in the message header */
    syslog(level | LOG_USER /* facility */,
	   "NSS " SOFTOKEN_LIB_NAME "[pid=%d uid=%d]: %s",
	   (int)getpid(), (int)getuid(), msg);
#ifdef LINUX
    if (pthread_once(&libaudit_once_control, libaudit_init) != 0) {
	return;
    }
    if (libaudit_handle) {
	int audit_fd;
	int result = (severity != NSS_AUDIT_ERROR); /* 1=success; 0=failed */
	char *message = PR_smprintf("NSS " SOFTOKEN_LIB_NAME ": %s", msg);
	if (!message) {
	    return;
	}
	audit_fd = audit_open_func();
	if (audit_fd < 0) {
	    PR_smprintf_free(message);
	    return;
	}
	audit_log_user_message_func(audit_fd, AUDIT_USER, message,
				    NULL, NULL, NULL, result);
	audit_close_func(audit_fd);
	PR_smprintf_free(message);
    }
#endif /* LINUX */
#ifdef SOLARIS
    {
        int rd;
        char *message = PR_smprintf("NSS " SOFTOKEN_LIB_NAME ": %s", msg);

        if (!message) {
            return;
        }

        /* open the record descriptor */
        if ((rd = au_open()) == -1) {
            PR_smprintf_free(message);
            return;
        }

        /* write the audit tokens to the audit record */
        if (au_write(rd, au_to_text(message))) {
            (void)au_close(rd, AU_TO_NO_WRITE, AUE_FIPS_AUDIT);
            PR_smprintf_free(message);
            return;
        }

        /* close the record and send it to the audit trail */
        (void)au_close(rd, AU_TO_WRITE, AUE_FIPS_AUDIT);

        PR_smprintf_free(message);
    }
#endif /* SOLARIS */
#else
    /* do nothing */
#endif
}


/**********************************************************************
 *
 *     Start of PKCS 11 functions 
 *
 **********************************************************************/
/* return the function list */
CK_RV FC_GetFunctionList(CK_FUNCTION_LIST_PTR *pFunctionList) {
    *pFunctionList = &sftk_fipsTable;
    return CKR_OK;
}

/* sigh global so pkcs11 can read it */
PRBool nsf_init = PR_FALSE;

/* FC_Initialize initializes the PKCS #11 library. */
CK_RV FC_Initialize(CK_VOID_PTR pReserved) {
    const char *envp;
    CK_RV crv;

    if (nsf_init) {
	return CKR_CRYPTOKI_ALREADY_INITIALIZED;
    }

    if ((envp = PR_GetEnv("NSS_ENABLE_AUDIT")) != NULL) {
	sftk_audit_enabled = (atoi(envp) == 1);
    }

    crv = nsc_CommonInitialize(pReserved, PR_TRUE);

    /* not an 'else' rv can be set by either SFTK_LowInit or SFTK_SlotInit*/
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
	return crv;
    }

    sftk_fatalError = PR_FALSE; /* any error has been reset */

    crv = sftk_fipsPowerUpSelfTest();
    if (crv != CKR_OK) {
        nsc_CommonFinalize(NULL, PR_TRUE);
	sftk_fatalError = PR_TRUE;
	if (sftk_audit_enabled) {
	    char msg[128];
	    PR_snprintf(msg,sizeof msg,
			"C_Initialize()=0x%08lX "
			"power-up self-tests failed",
			(PRUint32)crv);
	    sftk_LogAuditMessage(NSS_AUDIT_ERROR, msg);
	}
	return crv;
    }
    nsf_init = PR_TRUE;

    return CKR_OK;
}

/*FC_Finalize indicates that an application is done with the PKCS #11 library.*/
CK_RV FC_Finalize (CK_VOID_PTR pReserved) {
   CK_RV crv;
   if (!nsf_init) {
      return CKR_OK;
   }
   crv = nsc_CommonFinalize (pReserved, PR_TRUE);
   nsf_init = (PRBool) !(crv == CKR_OK);
   return crv;
}


/* FC_GetInfo returns general information about PKCS #11. */
CK_RV  FC_GetInfo(CK_INFO_PTR pInfo) {
    return NSC_GetInfo(pInfo);
}

/* FC_GetSlotList obtains a list of slots in the system. */
CK_RV FC_GetSlotList(CK_BBOOL tokenPresent,
	 		CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pulCount) {
    return nsc_CommonGetSlotList(tokenPresent,pSlotList,pulCount,
							 NSC_FIPS_MODULE);
}
	
/* FC_GetSlotInfo obtains information about a particular slot in the system. */
CK_RV FC_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo) {
    return NSC_GetSlotInfo(slotID,pInfo);
}


/*FC_GetTokenInfo obtains information about a particular token in the system.*/
 CK_RV FC_GetTokenInfo(CK_SLOT_ID slotID,CK_TOKEN_INFO_PTR pInfo) {
    CK_RV crv;

    crv = NSC_GetTokenInfo(slotID,pInfo);
    pInfo->flags |= CKF_LOGIN_REQUIRED;
    return crv;

}



/*FC_GetMechanismList obtains a list of mechanism types supported by a token.*/
 CK_RV FC_GetMechanismList(CK_SLOT_ID slotID,
	CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pusCount) {
    SFTK_FIPSFATALCHECK();
    if (slotID == FIPS_SLOT_ID) slotID = NETSCAPE_SLOT_ID;
    /* FIPS Slot supports all functions */
    return NSC_GetMechanismList(slotID,pMechanismList,pusCount);
}


/* FC_GetMechanismInfo obtains information about a particular mechanism 
 * possibly supported by a token. */
 CK_RV FC_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type,
    					CK_MECHANISM_INFO_PTR pInfo) {
    SFTK_FIPSFATALCHECK();
    if (slotID == FIPS_SLOT_ID) slotID = NETSCAPE_SLOT_ID;
    /* FIPS Slot supports all functions */
    return NSC_GetMechanismInfo(slotID,type,pInfo);
}


/* FC_InitToken initializes a token. */
 CK_RV FC_InitToken(CK_SLOT_ID slotID,CK_CHAR_PTR pPin,
 				CK_ULONG usPinLen,CK_CHAR_PTR pLabel) {
    CK_RV crv;

    crv = NSC_InitToken(slotID,pPin,usPinLen,pLabel);
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (crv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	/* pLabel points to a 32-byte label, which is not null-terminated */
	PR_snprintf(msg,sizeof msg,
		"C_InitToken(slotID=%lu, pLabel=\"%.32s\")=0x%08lX",
		(PRUint32)slotID,pLabel,(PRUint32)crv);
	sftk_LogAuditMessage(severity, msg);
    }
    return crv;
}


/* FC_InitPIN initializes the normal user's PIN. */
 CK_RV FC_InitPIN(CK_SESSION_HANDLE hSession,
    					CK_CHAR_PTR pPin, CK_ULONG ulPinLen) {
    CK_RV rv;
    if (sftk_fatalError) return CKR_DEVICE_ERROR;
    if ((rv = sftk_newPinCheck(pPin,ulPinLen)) == CKR_OK) {
	rv = NSC_InitPIN(hSession,pPin,ulPinLen);
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		"C_InitPIN(hSession=%lu)=0x%08lX",
		(PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, msg);
    }
    return rv;
}


/* FC_SetPIN modifies the PIN of user that is currently logged in. */
/* NOTE: This is only valid for the PRIVATE_KEY_SLOT */
 CK_RV FC_SetPIN(CK_SESSION_HANDLE hSession, CK_CHAR_PTR pOldPin,
    CK_ULONG usOldLen, CK_CHAR_PTR pNewPin, CK_ULONG usNewLen) {
    CK_RV rv;
    if ((rv = sftk_fipsCheck()) == CKR_OK &&
	(rv = sftk_newPinCheck(pNewPin,usNewLen)) == CKR_OK) {
	rv = NSC_SetPIN(hSession,pOldPin,usOldLen,pNewPin,usNewLen);
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		"C_SetPIN(hSession=%lu)=0x%08lX",
		(PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, msg);
    }
    return rv;
}

/* FC_OpenSession opens a session between an application and a token. */
 CK_RV FC_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags,
   CK_VOID_PTR pApplication,CK_NOTIFY Notify,CK_SESSION_HANDLE_PTR phSession) {
    SFTK_FIPSFATALCHECK();
    return NSC_OpenSession(slotID,flags,pApplication,Notify,phSession);
}


/* FC_CloseSession closes a session between an application and a token. */
 CK_RV FC_CloseSession(CK_SESSION_HANDLE hSession) {
    return NSC_CloseSession(hSession);
}


/* FC_CloseAllSessions closes all sessions with a token. */
 CK_RV FC_CloseAllSessions (CK_SLOT_ID slotID) {
    return NSC_CloseAllSessions (slotID);
}


/* FC_GetSessionInfo obtains information about the session. */
 CK_RV FC_GetSessionInfo(CK_SESSION_HANDLE hSession,
						CK_SESSION_INFO_PTR pInfo) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();

    rv = NSC_GetSessionInfo(hSession,pInfo);
    if (rv == CKR_OK) {
	if ((isLoggedIn) && (pInfo->state == CKS_RO_PUBLIC_SESSION)) {
		pInfo->state = CKS_RO_USER_FUNCTIONS;
	}
	if ((isLoggedIn) && (pInfo->state == CKS_RW_PUBLIC_SESSION)) {
		pInfo->state = CKS_RW_USER_FUNCTIONS;
	}
    }
    return rv;
}

/* FC_Login logs a user into a token. */
 CK_RV FC_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType,
				    CK_CHAR_PTR pPin, CK_ULONG usPinLen) {
    CK_RV rv;
    PRBool successful;
    if (sftk_fatalError) return CKR_DEVICE_ERROR;
    rv = NSC_Login(hSession,userType,pPin,usPinLen);
    successful = (rv == CKR_OK) || (rv == CKR_USER_ALREADY_LOGGED_IN);
    if (successful)
	isLoggedIn = PR_TRUE;
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity;
	severity = successful ? NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		    "C_Login(hSession=%lu, userType=%lu)=0x%08lX",
		    (PRUint32)hSession,(PRUint32)userType,(PRUint32)rv);
	sftk_LogAuditMessage(severity, msg);
    }
    return rv;
}

/* FC_Logout logs a user out from a token. */
 CK_RV FC_Logout(CK_SESSION_HANDLE hSession) {
    CK_RV rv;
    if ((rv = sftk_fipsCheck()) == CKR_OK) {
	rv = NSC_Logout(hSession);
	isLoggedIn = PR_FALSE;
    }
    if (sftk_audit_enabled) {
	char msg[128];
	NSSAuditSeverity severity = (rv == CKR_OK) ?
		NSS_AUDIT_INFO : NSS_AUDIT_ERROR;
	PR_snprintf(msg,sizeof msg,
		    "C_Logout(hSession=%lu)=0x%08lX",
		    (PRUint32)hSession,(PRUint32)rv);
	sftk_LogAuditMessage(severity, msg);
    }
    return rv;
}


/* FC_CreateObject creates a new object. */
 CK_RV FC_CreateObject(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, 
					CK_OBJECT_HANDLE_PTR phObject) {
    CK_OBJECT_CLASS * classptr;
    SFTK_FIPSCHECK();
    classptr = (CK_OBJECT_CLASS *)fc_getAttribute(pTemplate,ulCount,CKA_CLASS);
    if (classptr == NULL) return CKR_TEMPLATE_INCOMPLETE;

    /* FIPS can't create keys from raw key material */
    if ((*classptr == CKO_SECRET_KEY) || (*classptr == CKO_PRIVATE_KEY)) {
	return CKR_ATTRIBUTE_VALUE_INVALID;
    }
    return NSC_CreateObject(hSession,pTemplate,ulCount,phObject);
}





/* FC_CopyObject copies an object, creating a new object for the copy. */
 CK_RV FC_CopyObject(CK_SESSION_HANDLE hSession,
       CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usCount,
					CK_OBJECT_HANDLE_PTR phNewObject) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();
    rv = fips_login_if_key_object(hSession, hObject);
    if (rv != CKR_OK) {
	return rv;
    }
    return NSC_CopyObject(hSession,hObject,pTemplate,usCount,phNewObject);
}


/* FC_DestroyObject destroys an object. */
 CK_RV FC_DestroyObject(CK_SESSION_HANDLE hSession,
		 				CK_OBJECT_HANDLE hObject) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();
    rv = fips_login_if_key_object(hSession, hObject);
    if (rv != CKR_OK) {
	return rv;
    }
    return NSC_DestroyObject(hSession,hObject);
}


/* FC_GetObjectSize gets the size of an object in bytes. */
 CK_RV FC_GetObjectSize(CK_SESSION_HANDLE hSession,
    			CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pusSize) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();
    rv = fips_login_if_key_object(hSession, hObject);
    if (rv != CKR_OK) {
	return rv;
    }
    return NSC_GetObjectSize(hSession, hObject, pusSize);
}


/* FC_GetAttributeValue obtains the value of one or more object attributes. */
 CK_RV FC_GetAttributeValue(CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();
    rv = fips_login_if_key_object(hSession, hObject);
    if (rv != CKR_OK) {
	return rv;
    }
    return NSC_GetAttributeValue(hSession,hObject,pTemplate,usCount);
}


/* FC_SetAttributeValue modifies the value of one or more object attributes */
 CK_RV FC_SetAttributeValue (CK_SESSION_HANDLE hSession,
 CK_OBJECT_HANDLE hObject,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    CK_RV rv;
    SFTK_FIPSFATALCHECK();
    rv = fips_login_if_key_object(hSession, hObject);
    if (rv != CKR_OK) {
	return rv;
    }
    return NSC_SetAttributeValue(hSession,hObject,pTemplate,usCount);
}



/* FC_FindObjectsInit initializes a search for token and session objects 
 * that match a template. */
 CK_RV FC_FindObjectsInit(CK_SESSION_HANDLE hSession,
    			CK_ATTRIBUTE_PTR pTemplate,CK_ULONG usCount) {
    /* let publically readable object be found */
    unsigned int i;
    CK_RV rv;
    PRBool needLogin = PR_FALSE;

    SFTK_FIPSFATALCHECK();

    for (i=0; i < usCount; i++) {
	CK_OBJECT_CLASS class;
	if (pTemplate[i].type != CKA_CLASS) {
	    continue;
	}
	if (pTemplate[i].ulValueLen != sizeof(CK_OBJECT_CLASS)) {
	    continue;
	}
	if (pTemplate[i].pValue == NULL) {
	    continue;
	}
	class = *(CK_OBJECT_CLASS *)pTemplate[i].pValue;
	if ((class == CKO_PRIVATE_KEY) || (class == CKO_SECRET_KEY)) {
	    needLogin = PR_TRUE;
	    break;
	}
    }
    if (needLogin) {
	if ((rv = sftk_fipsCheck()) != CKR_OK) return rv;
    }
    return NSC_FindObjectsInit(hSession,pTemplate,usCount);
}


/* FC_FindObjects continues a search for token and session objects 
 * that match a template, obtaining additional object handles. */
 CK_RV FC_FindObjects(CK_SESSION_HANDLE hSession,
    CK_OBJECT_HANDLE_PTR phObject,CK_ULONG usMaxObjectCount,
    					CK_ULONG_PTR pusObjectCount) {
    /* let publically readable object be found */
    SFTK_FIPSFATALCHECK();
    return NSC_FindObjects(hSession,phObject,usMaxObjectCount,
    							pusObjectCount);
}


/*
 ************** Crypto Functions:     Encrypt ************************
 */

/* FC_EncryptInit initializes an encryption operation. */
 CK_RV FC_EncryptInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_EncryptInit(hSession,pMechanism,hKey);
}

/* FC_Encrypt encrypts single-part data. */
 CK_RV FC_Encrypt (CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    		CK_ULONG usDataLen, CK_BYTE_PTR pEncryptedData,
					 CK_ULONG_PTR pusEncryptedDataLen) {
    SFTK_FIPSCHECK();
    return NSC_Encrypt(hSession,pData,usDataLen,pEncryptedData,
							pusEncryptedDataLen);
}


/* FC_EncryptUpdate continues a multiple-part encryption operation. */
 CK_RV FC_EncryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart, CK_ULONG usPartLen, CK_BYTE_PTR pEncryptedPart,	
					CK_ULONG_PTR pusEncryptedPartLen) {
    SFTK_FIPSCHECK();
    return NSC_EncryptUpdate(hSession,pPart,usPartLen,pEncryptedPart,
						pusEncryptedPartLen);
}


/* FC_EncryptFinal finishes a multiple-part encryption operation. */
 CK_RV FC_EncryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pusLastEncryptedPartLen) {

    SFTK_FIPSCHECK();
    return NSC_EncryptFinal(hSession,pLastEncryptedPart,
						pusLastEncryptedPartLen);
}

/*
 ************** Crypto Functions:     Decrypt ************************
 */


/* FC_DecryptInit initializes a decryption operation. */
 CK_RV FC_DecryptInit( CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_DecryptInit(hSession,pMechanism,hKey);
}

/* FC_Decrypt decrypts encrypted data in a single part. */
 CK_RV FC_Decrypt(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,CK_ULONG usEncryptedDataLen,CK_BYTE_PTR pData,
    						CK_ULONG_PTR pusDataLen) {
    SFTK_FIPSCHECK();
    return NSC_Decrypt(hSession,pEncryptedData,usEncryptedDataLen,pData,
    								pusDataLen);
}


/* FC_DecryptUpdate continues a multiple-part decryption operation. */
 CK_RV FC_DecryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart, CK_ULONG usEncryptedPartLen,
    				CK_BYTE_PTR pPart, CK_ULONG_PTR pusPartLen) {
    SFTK_FIPSCHECK();
    return NSC_DecryptUpdate(hSession,pEncryptedPart,usEncryptedPartLen,
    							pPart,pusPartLen);
}


/* FC_DecryptFinal finishes a multiple-part decryption operation. */
 CK_RV FC_DecryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart, CK_ULONG_PTR pusLastPartLen) {
    SFTK_FIPSCHECK();
    return NSC_DecryptFinal(hSession,pLastPart,pusLastPartLen);
}


/*
 ************** Crypto Functions:     Digest (HASH)  ************************
 */

/* FC_DigestInit initializes a message-digesting operation. */
 CK_RV FC_DigestInit(CK_SESSION_HANDLE hSession,
    					CK_MECHANISM_PTR pMechanism) {
    SFTK_FIPSFATALCHECK();
    return NSC_DigestInit(hSession, pMechanism);
}


/* FC_Digest digests data in a single part. */
 CK_RV FC_Digest(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData, CK_ULONG usDataLen, CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    SFTK_FIPSFATALCHECK();
    return NSC_Digest(hSession,pData,usDataLen,pDigest,pusDigestLen);
}


/* FC_DigestUpdate continues a multiple-part message-digesting operation. */
 CK_RV FC_DigestUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
					    CK_ULONG usPartLen) {
    SFTK_FIPSFATALCHECK();
    return NSC_DigestUpdate(hSession,pPart,usPartLen);
}


/* FC_DigestFinal finishes a multiple-part message-digesting operation. */
 CK_RV FC_DigestFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pusDigestLen) {
    SFTK_FIPSFATALCHECK();
    return NSC_DigestFinal(hSession,pDigest,pusDigestLen);
}


/*
 ************** Crypto Functions:     Sign  ************************
 */

/* FC_SignInit initializes a signature (private key encryption) operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_SignInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_SignInit(hSession,pMechanism,hKey);
}


/* FC_Sign signs (encrypts with private key) data in a single part,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_Sign(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,CK_ULONG usDataLen,CK_BYTE_PTR pSignature,
    					CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    return NSC_Sign(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}


/* FC_SignUpdate continues a multiple-part signature operation,
 * where the signature is (will be) an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_SignUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    							CK_ULONG usPartLen) {
    SFTK_FIPSCHECK();
    return NSC_SignUpdate(hSession,pPart,usPartLen);
}


/* FC_SignFinal finishes a multiple-part signature operation, 
 * returning the signature. */
 CK_RV FC_SignFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pSignature,
					    CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    return NSC_SignFinal(hSession,pSignature,pusSignatureLen);
}

/*
 ************** Crypto Functions:     Sign Recover  ************************
 */
/* FC_SignRecoverInit initializes a signature operation,
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
 CK_RV FC_SignRecoverInit(CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_SignRecoverInit(hSession,pMechanism,hKey);
}


/* FC_SignRecover signs data in a single operation
 * where the (digest) data can be recovered from the signature. 
 * E.g. encryption with the user's private key */
 CK_RV FC_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
  CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pusSignatureLen) {
    SFTK_FIPSCHECK();
    return NSC_SignRecover(hSession,pData,usDataLen,pSignature,pusSignatureLen);
}

/*
 ************** Crypto Functions:     verify  ************************
 */

/* FC_VerifyInit initializes a verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature (e.g. DSA) */
 CK_RV FC_VerifyInit(CK_SESSION_HANDLE hSession,
			   CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_VerifyInit(hSession,pMechanism,hKey);
}


/* FC_Verify verifies a signature in a single-part operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    CK_ULONG usDataLen, CK_BYTE_PTR pSignature, CK_ULONG usSignatureLen) {
    /* make sure we're legal */
    SFTK_FIPSCHECK();
    return NSC_Verify(hSession,pData,usDataLen,pSignature,usSignatureLen);
}


/* FC_VerifyUpdate continues a multiple-part verification operation, 
 * where the signature is an appendix to the data, 
 * and plaintext cannot be recovered from the signature */
 CK_RV FC_VerifyUpdate( CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart,
						CK_ULONG usPartLen) {
    SFTK_FIPSCHECK();
    return NSC_VerifyUpdate(hSession,pPart,usPartLen);
}


/* FC_VerifyFinal finishes a multiple-part verification operation, 
 * checking the signature. */
 CK_RV FC_VerifyFinal(CK_SESSION_HANDLE hSession,
			CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen) {
    SFTK_FIPSCHECK();
    return NSC_VerifyFinal(hSession,pSignature,usSignatureLen);
}

/*
 ************** Crypto Functions:     Verify  Recover ************************
 */

/* FC_VerifyRecoverInit initializes a signature verification operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
 CK_RV FC_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_VerifyRecoverInit(hSession,pMechanism,hKey);
}


/* FC_VerifyRecover verifies a signature in a single-part operation, 
 * where the data is recovered from the signature. 
 * E.g. Decryption with the user's public key */
 CK_RV FC_VerifyRecover(CK_SESSION_HANDLE hSession,
		 CK_BYTE_PTR pSignature,CK_ULONG usSignatureLen,
    				CK_BYTE_PTR pData,CK_ULONG_PTR pusDataLen) {
    SFTK_FIPSCHECK();
    return NSC_VerifyRecover(hSession,pSignature,usSignatureLen,pData,
								pusDataLen);
}

/*
 **************************** Key Functions:  ************************
 */

/* FC_GenerateKey generates a secret key, creating a new key object. */
 CK_RV FC_GenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount,
    						CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();

    /* all secret keys must be sensitive, if the upper level code tries to say
     * otherwise, reject it. */
    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, ulCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }

    return NSC_GenerateKey(hSession,pMechanism,pTemplate,ulCount,phKey);
}


/* FC_GenerateKeyPair generates a public-key/private-key pair, 
 * creating new key objects. */
 CK_RV FC_GenerateKeyPair (CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG usPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG usPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPublicKey,
					CK_OBJECT_HANDLE_PTR phPrivateKey) {
    CK_BBOOL *boolptr;
    CK_RV crv;

    SFTK_FIPSCHECK();

    /* all private keys must be sensitive, if the upper level code tries to say
     * otherwise, reject it. */
    boolptr = (CK_BBOOL *) fc_getAttribute(pPrivateKeyTemplate, 
				usPrivateKeyAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    crv = NSC_GenerateKeyPair (hSession,pMechanism,pPublicKeyTemplate,
    		usPublicKeyAttributeCount,pPrivateKeyTemplate,
		usPrivateKeyAttributeCount,phPublicKey,phPrivateKey);
    if (crv == CKR_GENERAL_ERROR) {
	/* pairwise consistency check failed. */
	sftk_fatalError = PR_TRUE;
    }
    return crv;
}


/* FC_WrapKey wraps (i.e., encrypts) a key. */
 CK_RV FC_WrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
					 CK_ULONG_PTR pusWrappedKeyLen) {
    SFTK_FIPSCHECK();
    return NSC_WrapKey(hSession,pMechanism,hWrappingKey,hKey,pWrappedKey,
							pusWrappedKeyLen);
}


/* FC_UnwrapKey unwraps (decrypts) a wrapped key, creating a new key object. */
 CK_RV FC_UnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG usWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usAttributeCount,
						 CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();

    /* all secret keys must be sensitive, if the upper level code tries to say
     * otherwise, reject it. */
    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, 
					usAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    return NSC_UnwrapKey(hSession,pMechanism,hUnwrappingKey,pWrappedKey,
			usWrappedKeyLen,pTemplate,usAttributeCount,phKey);
}


/* FC_DeriveKey derives a key from a base key, creating a new key object. */
 CK_RV FC_DeriveKey( CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
	 CK_ATTRIBUTE_PTR pTemplate, CK_ULONG usAttributeCount, 
						CK_OBJECT_HANDLE_PTR phKey) {
    CK_BBOOL *boolptr;

    SFTK_FIPSCHECK();

    /* all secret keys must be sensitive, if the upper level code tries to say
     * otherwise, reject it. */
    boolptr = (CK_BBOOL *) fc_getAttribute(pTemplate, 
					usAttributeCount, CKA_SENSITIVE);
    if (boolptr != NULL) {
	if (!(*boolptr)) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }
    return NSC_DeriveKey(hSession,pMechanism,hBaseKey,pTemplate,
			usAttributeCount, phKey);
}

/*
 **************************** Radom Functions:  ************************
 */

/* FC_SeedRandom mixes additional seed material into the token's random number 
 * generator. */
 CK_RV FC_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed,
    CK_ULONG usSeedLen) {
    CK_RV crv;

    SFTK_FIPSFATALCHECK();
    crv = NSC_SeedRandom(hSession,pSeed,usSeedLen);
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
    }
    return crv;
}


/* FC_GenerateRandom generates random data. */
 CK_RV FC_GenerateRandom(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR	pRandomData, CK_ULONG ulRandomLen) {
    CK_RV crv;

    SFTK_FIPSFATALCHECK();
    crv = NSC_GenerateRandom(hSession,pRandomData,ulRandomLen);
    if (crv != CKR_OK) {
	sftk_fatalError = PR_TRUE;
	if (sftk_audit_enabled) {
	    char msg[128];
	    PR_snprintf(msg,sizeof msg,
			"C_GenerateRandom(hSession=%lu, pRandomData=%p, "
			"ulRandomLen=%lu)=0x%08lX "
			"self-test: continuous RNG test failed",
			(PRUint32)hSession,pRandomData,
			(PRUint32)ulRandomLen,(PRUint32)crv);
	    sftk_LogAuditMessage(NSS_AUDIT_ERROR, msg);
	}
    }
    return crv;
}


/* FC_GetFunctionStatus obtains an updated status of a function running 
 * in parallel with an application. */
 CK_RV FC_GetFunctionStatus(CK_SESSION_HANDLE hSession) {
    SFTK_FIPSCHECK();
    return NSC_GetFunctionStatus(hSession);
}


/* FC_CancelFunction cancels a function running in parallel */
 CK_RV FC_CancelFunction(CK_SESSION_HANDLE hSession) {
    SFTK_FIPSCHECK();
    return NSC_CancelFunction(hSession);
}

/*
 ****************************  Version 1.1 Functions:  ************************
 */

/* FC_GetOperationState saves the state of the cryptographic 
 *operation in a session. */
CK_RV FC_GetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG_PTR pulOperationStateLen) {
    SFTK_FIPSFATALCHECK();
    return NSC_GetOperationState(hSession,pOperationState,pulOperationStateLen);
}


/* FC_SetOperationState restores the state of the cryptographic operation 
 * in a session. */
CK_RV FC_SetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG  ulOperationStateLen,
        CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey) {
    SFTK_FIPSFATALCHECK();
    return NSC_SetOperationState(hSession,pOperationState,ulOperationStateLen,
        				hEncryptionKey,hAuthenticationKey);
}

/* FC_FindObjectsFinal finishes a search for token and session objects. */
CK_RV FC_FindObjectsFinal(CK_SESSION_HANDLE hSession) {
    /* let publically readable object be found */
    SFTK_FIPSFATALCHECK();
    return NSC_FindObjectsFinal(hSession);
}


/* Dual-function cryptographic operations */

/* FC_DigestEncryptUpdate continues a multiple-part digesting and encryption 
 * operation. */
CK_RV FC_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
    CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {
    SFTK_FIPSCHECK();
    return NSC_DigestEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}


/* FC_DecryptDigestUpdate continues a multiple-part decryption and digesting 
 * operation. */
CK_RV FC_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
     CK_BYTE_PTR  pEncryptedPart, CK_ULONG  ulEncryptedPartLen,
    				CK_BYTE_PTR  pPart, CK_ULONG_PTR pulPartLen) {

    SFTK_FIPSCHECK();
    return NSC_DecryptDigestUpdate(hSession, pEncryptedPart,ulEncryptedPartLen,
    				pPart,pulPartLen);
}

/* FC_SignEncryptUpdate continues a multiple-part signing and encryption 
 * operation. */
CK_RV FC_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
	 CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen) {

    SFTK_FIPSCHECK();
    return NSC_SignEncryptUpdate(hSession,pPart,ulPartLen,pEncryptedPart,
					 pulEncryptedPartLen);
}

/* FC_DecryptVerifyUpdate continues a multiple-part decryption and verify 
 * operation. */
CK_RV FC_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pEncryptedData, CK_ULONG  ulEncryptedDataLen, 
				CK_BYTE_PTR  pData, CK_ULONG_PTR pulDataLen) {

    SFTK_FIPSCHECK();
    return NSC_DecryptVerifyUpdate(hSession,pEncryptedData,ulEncryptedDataLen, 
				pData,pulDataLen);
}


/* FC_DigestKey continues a multi-part message-digesting operation,
 * by digesting the value of a secret key as part of the data already digested.
 */
CK_RV FC_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey) {
    SFTK_FIPSCHECK();
    return NSC_DigestKey(hSession,hKey);
}


CK_RV FC_WaitForSlotEvent(CK_FLAGS flags, CK_SLOT_ID_PTR pSlot,
							 CK_VOID_PTR pReserved)
{
    return NSC_WaitForSlotEvent(flags, pSlot, pReserved);
}
