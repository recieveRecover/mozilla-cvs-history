/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
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
/*******************************************************************************
 * Source date: 9 Apr 1997 21:45:11 GMT
 * netscape/fonts/nffbp public interface
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mnffbp_H_
#define _Mnffbp_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * nffbp
 ******************************************************************************/

/* The type of the nffbp interface. */
struct nffbpInterface;

/* The public type of a nffbp instance. */
typedef struct nffbp {
	const struct nffbpInterface*	vtable;
} nffbp;

/* The inteface ID of the nffbp interface. */
#ifndef JMC_INIT_nffbp_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID nffbp_ID;
#else
EXTERN_C const JMCInterfaceID nffbp_ID = { 0x20344d64, 0x37395725, 0x253b0b28, 0x673d761a };
#endif /* JMC_INIT_nffbp_ID */
/*******************************************************************************
 * nffbp Operations
 ******************************************************************************/

#define nffbp_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, nffbp_getInterface_op, a, exception))

#define nffbp_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, nffbp_addRef_op, exception))

#define nffbp_release(self, exception)	\
	(((self)->vtable->release)(self, nffbp_release_op, exception))

#define nffbp_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, nffbp_hashCode_op, exception))

#define nffbp_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, nffbp_equals_op, a, exception))

#define nffbp_clone(self, exception)	\
	(((self)->vtable->clone)(self, nffbp_clone_op, exception))

#define nffbp_toString(self, exception)	\
	(((self)->vtable->toString)(self, nffbp_toString_op, exception))

#define nffbp_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, nffbp_finalize_op, exception))

#define nffbp_RegisterFontDisplayer(self, a, exception)	\
	(((self)->vtable->RegisterFontDisplayer)(self, nffbp_RegisterFontDisplayer_op, a, exception))

#define nffbp_CreateFontDisplayerFromDLM(self, a, exception)	\
	(((self)->vtable->CreateFontDisplayerFromDLM)(self, nffbp_CreateFontDisplayerFromDLM_op, a, exception))

#define nffbp_ScanForFontDisplayers(self, a, exception)	\
	(((self)->vtable->ScanForFontDisplayers)(self, nffbp_ScanForFontDisplayers_op, a, exception))

#define nffbp_RfDone(self, a, exception)	\
	(((self)->vtable->RfDone)(self, nffbp_RfDone_op, a, exception))

/*******************************************************************************
 * nffbp Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_nffp;
struct netscape_jmc_ConstCString;
struct netscape_fonts_nfrf;

struct nffbpInterface {
	void*	(*getInterface)(struct nffbp* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct nffbp* self, jint op, JMCException* *exception);
	void	(*release)(struct nffbp* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct nffbp* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct nffbp* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct nffbp* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct nffbp* self, jint op, JMCException* *exception);
	void	(*finalize)(struct nffbp* self, jint op, JMCException* *exception);
	jint	(*RegisterFontDisplayer)(struct nffbp* self, jint op, struct nffp* a, JMCException* *exception);
	jint	(*CreateFontDisplayerFromDLM)(struct nffbp* self, jint op, const char* a, JMCException* *exception);
	jint	(*ScanForFontDisplayers)(struct nffbp* self, jint op, const char* a, JMCException* *exception);
	void	(*RfDone)(struct nffbp* self, jint op, struct nfrf* a, JMCException* *exception);
};

/*******************************************************************************
 * nffbp Operation IDs
 ******************************************************************************/

typedef enum nffbpOperations {
	nffbp_getInterface_op,
	nffbp_addRef_op,
	nffbp_release_op,
	nffbp_hashCode_op,
	nffbp_equals_op,
	nffbp_clone_op,
	nffbp_toString_op,
	nffbp_finalize_op,
	nffbp_RegisterFontDisplayer_op,
	nffbp_CreateFontDisplayerFromDLM_op,
	nffbp_ScanForFontDisplayers_op,
	nffbp_RfDone_op
} nffbpOperations;

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mnffbp_H_ */
