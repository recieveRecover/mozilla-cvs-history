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
 * Source date: 9 Apr 1997 21:45:12 GMT
 * netscape/fonts/nfrc public interface
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mnfrc_H_
#define _Mnfrc_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * nfrc
 ******************************************************************************/

/* The type of the nfrc interface. */
struct nfrcInterface;

/* The public type of a nfrc instance. */
typedef struct nfrc {
	const struct nfrcInterface*	vtable;
} nfrc;

/* The inteface ID of the nfrc interface. */
#ifndef JMC_INIT_nfrc_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID nfrc_ID;
#else
EXTERN_C const JMCInterfaceID nfrc_ID = { 0x241a4e5e, 0x465b4148, 0x5d5a481e, 0x695e4c7f };
#endif /* JMC_INIT_nfrc_ID */
/*******************************************************************************
 * nfrc Operations
 ******************************************************************************/

#define nfrc_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, nfrc_getInterface_op, a, exception))

#define nfrc_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, nfrc_addRef_op, exception))

#define nfrc_release(self, exception)	\
	(((self)->vtable->release)(self, nfrc_release_op, exception))

#define nfrc_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, nfrc_hashCode_op, exception))

#define nfrc_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, nfrc_equals_op, a, exception))

#define nfrc_clone(self, exception)	\
	(((self)->vtable->clone)(self, nfrc_clone_op, exception))

#define nfrc_toString(self, exception)	\
	(((self)->vtable->toString)(self, nfrc_toString_op, exception))

#define nfrc_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, nfrc_finalize_op, exception))

#define nfrc_GetMajorType(self, exception)	\
	(((self)->vtable->GetMajorType)(self, nfrc_GetMajorType_op, exception))

#define nfrc_GetMinorType(self, exception)	\
	(((self)->vtable->GetMinorType)(self, nfrc_GetMinorType_op, exception))

#define nfrc_IsEquivalent(self, a, b, exception)	\
	(((self)->vtable->IsEquivalent)(self, nfrc_IsEquivalent_op, a, b, exception))

#define nfrc_GetPlatformData(self, exception)	\
	(((self)->vtable->GetPlatformData)(self, nfrc_GetPlatformData_op, exception))

#define nfrc_SetPlatformData(self, a, exception)	\
	(((self)->vtable->SetPlatformData)(self, nfrc_SetPlatformData_op, a, exception))

/*******************************************************************************
 * nfrc Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_PlatformRCData;
struct netscape_fonts_PlatformRCDataStar;

struct nfrcInterface {
	void*	(*getInterface)(struct nfrc* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct nfrc* self, jint op, JMCException* *exception);
	void	(*release)(struct nfrc* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct nfrc* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct nfrc* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct nfrc* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct nfrc* self, jint op, JMCException* *exception);
	void	(*finalize)(struct nfrc* self, jint op, JMCException* *exception);
	jint	(*GetMajorType)(struct nfrc* self, jint op, JMCException* *exception);
	jint	(*GetMinorType)(struct nfrc* self, jint op, JMCException* *exception);
	jint	(*IsEquivalent)(struct nfrc* self, jint op, jint a, jint b, JMCException* *exception);
	struct rc_data	(*GetPlatformData)(struct nfrc* self, jint op, JMCException* *exception);
	jint	(*SetPlatformData)(struct nfrc* self, jint op, struct rc_data * a, JMCException* *exception);
};

/*******************************************************************************
 * nfrc Operation IDs
 ******************************************************************************/

typedef enum nfrcOperations {
	nfrc_getInterface_op,
	nfrc_addRef_op,
	nfrc_release_op,
	nfrc_hashCode_op,
	nfrc_equals_op,
	nfrc_clone_op,
	nfrc_toString_op,
	nfrc_finalize_op,
	nfrc_GetMajorType_op,
	nfrc_GetMinorType_op,
	nfrc_IsEquivalent_op,
	nfrc_GetPlatformData_op,
	nfrc_SetPlatformData_op
} nfrcOperations;

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mnfrc_H_ */
