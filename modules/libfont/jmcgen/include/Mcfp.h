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
 * Source date: 9 Apr 1997 21:45:13 GMT
 * netscape/fonts/cfp module C header file
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#ifndef _Mcfp_H_
#define _Mcfp_H_

#include "jmc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * cfp
 ******************************************************************************/

/* The type of the cfp interface. */
struct cfpInterface;

/* The public type of a cfp instance. */
typedef struct cfp {
	const struct cfpInterface*	vtable;
} cfp;

/* The inteface ID of the cfp interface. */
#ifndef JMC_INIT_cfp_ID
extern EXTERN_C_WITHOUT_EXTERN const JMCInterfaceID cfp_ID;
#else
EXTERN_C const JMCInterfaceID cfp_ID = { 0x1827292b, 0x1915644a, 0x282f2339, 0x77347420 };
#endif /* JMC_INIT_cfp_ID */
/*******************************************************************************
 * cfp Operations
 ******************************************************************************/

#define cfp_getInterface(self, a, exception)	\
	(((self)->vtable->getInterface)(self, cfp_getInterface_op, a, exception))

#define cfp_addRef(self, exception)	\
	(((self)->vtable->addRef)(self, cfp_addRef_op, exception))

#define cfp_release(self, exception)	\
	(((self)->vtable->release)(self, cfp_release_op, exception))

#define cfp_hashCode(self, exception)	\
	(((self)->vtable->hashCode)(self, cfp_hashCode_op, exception))

#define cfp_equals(self, a, exception)	\
	(((self)->vtable->equals)(self, cfp_equals_op, a, exception))

#define cfp_clone(self, exception)	\
	(((self)->vtable->clone)(self, cfp_clone_op, exception))

#define cfp_toString(self, exception)	\
	(((self)->vtable->toString)(self, cfp_toString_op, exception))

#define cfp_finalize(self, exception)	\
	(((self)->vtable->finalize)(self, cfp_finalize_op, exception))

#define cfp_LookupFont(self, a, b, c, exception)	\
	(((self)->vtable->LookupFont)(self, cfp_LookupFont_op, a, b, c, exception))

#define cfp_CreateFontFromFile(self, a, b, c, d, exception)	\
	(((self)->vtable->CreateFontFromFile)(self, cfp_CreateFontFromFile_op, a, b, c, d, exception))

#define cfp_CreateFontStreamHandler(self, a, b, exception)	\
	(((self)->vtable->CreateFontStreamHandler)(self, cfp_CreateFontStreamHandler_op, a, b, exception))

#define cfp_EnumerateSizes(self, a, b, exception)	\
	(((self)->vtable->EnumerateSizes)(self, cfp_EnumerateSizes_op, a, b, exception))

#define cfp_ReleaseFontHandle(self, a, exception)	\
	(((self)->vtable->ReleaseFontHandle)(self, cfp_ReleaseFontHandle_op, a, exception))

#define cfp_GetMatchInfo(self, a, exception)	\
	(((self)->vtable->GetMatchInfo)(self, cfp_GetMatchInfo_op, a, exception))

#define cfp_GetRenderableFont(self, a, b, c, exception)	\
	(((self)->vtable->GetRenderableFont)(self, cfp_GetRenderableFont_op, a, b, c, exception))

#define cfp_Name(self, exception)	\
	(((self)->vtable->Name)(self, cfp_Name_op, exception))

#define cfp_Description(self, exception)	\
	(((self)->vtable->Description)(self, cfp_Description_op, exception))

#define cfp_EnumerateMimeTypes(self, exception)	\
	(((self)->vtable->EnumerateMimeTypes)(self, cfp_EnumerateMimeTypes_op, exception))

#define cfp_ListFonts(self, a, b, exception)	\
	(((self)->vtable->ListFonts)(self, cfp_ListFonts_op, a, b, exception))

#define cfp_ListSizes(self, a, b, exception)	\
	(((self)->vtable->ListSizes)(self, cfp_ListSizes_op, a, b, exception))

#define cfp_CacheFontInfo(self, exception)	\
	(((self)->vtable->CacheFontInfo)(self, cfp_CacheFontInfo_op, exception))

/*******************************************************************************
 * cfp Interface
 ******************************************************************************/

struct netscape_jmc_JMCInterfaceID;
struct java_lang_Object;
struct java_lang_String;
struct netscape_fonts_nfrc;
struct netscape_fonts_nffmi;
struct netscape_jmc_ConstCString;
struct netscape_fonts_nfstrm;
struct netscape_fonts_nfrf;

struct cfpInterface {
	void*	(*getInterface)(struct cfp* self, jint op, const JMCInterfaceID* a, JMCException* *exception);
	void	(*addRef)(struct cfp* self, jint op, JMCException* *exception);
	void	(*release)(struct cfp* self, jint op, JMCException* *exception);
	jint	(*hashCode)(struct cfp* self, jint op, JMCException* *exception);
	jbool	(*equals)(struct cfp* self, jint op, void* a, JMCException* *exception);
	void*	(*clone)(struct cfp* self, jint op, JMCException* *exception);
	const char*	(*toString)(struct cfp* self, jint op, JMCException* *exception);
	void	(*finalize)(struct cfp* self, jint op, JMCException* *exception);
	void*	(*LookupFont)(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, const char* c, JMCException* *exception);
	void*	(*CreateFontFromFile)(struct cfp* self, jint op, struct nfrc* a, const char* b, const char* c, const char* d, JMCException* *exception);
	struct nfstrm*	(*CreateFontStreamHandler)(struct cfp* self, jint op, struct nfrc* a, const char* b, JMCException* *exception);
	void*	(*EnumerateSizes)(struct cfp* self, jint op, struct nfrc* a, void* b, JMCException* *exception);
	jint	(*ReleaseFontHandle)(struct cfp* self, jint op, void* a, JMCException* *exception);
	struct nffmi*	(*GetMatchInfo)(struct cfp* self, jint op, void* a, JMCException* *exception);
	struct nfrf*	(*GetRenderableFont)(struct cfp* self, jint op, struct nfrc* a, void* b, jdouble c, JMCException* *exception);
	const char*	(*Name)(struct cfp* self, jint op, JMCException* *exception);
	const char*	(*Description)(struct cfp* self, jint op, JMCException* *exception);
	const char*	(*EnumerateMimeTypes)(struct cfp* self, jint op, JMCException* *exception);
	void*	(*ListFonts)(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, JMCException* *exception);
	void*	(*ListSizes)(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, JMCException* *exception);
	jint	(*CacheFontInfo)(struct cfp* self, jint op, JMCException* *exception);
};

/*******************************************************************************
 * cfp Operation IDs
 ******************************************************************************/

typedef enum cfpOperations {
	cfp_getInterface_op,
	cfp_addRef_op,
	cfp_release_op,
	cfp_hashCode_op,
	cfp_equals_op,
	cfp_clone_op,
	cfp_toString_op,
	cfp_finalize_op,
	cfp_LookupFont_op,
	cfp_CreateFontFromFile_op,
	cfp_CreateFontStreamHandler_op,
	cfp_EnumerateSizes_op,
	cfp_ReleaseFontHandle_op,
	cfp_GetMatchInfo_op,
	cfp_GetRenderableFont_op,
	cfp_Name_op,
	cfp_Description_op,
	cfp_EnumerateMimeTypes_op,
	cfp_ListFonts_op,
	cfp_ListSizes_op,
	cfp_CacheFontInfo_op
} cfpOperations;

/*******************************************************************************
 * Writing your C implementation: "cfp.h"
 * *****************************************************************************
 * You must create a header file named "cfp.h" that implements
 * the struct cfpImpl, including the struct cfpImplHeader
 * as it's first field:
 * 
 * 		#include "Mcfp.h" // generated header
 * 
 * 		struct cfpImpl {
 * 			cfpImplHeader	header;
 * 			<your instance data>
 * 		};
 * 
 * This header file will get included by the generated module implementation.
 ******************************************************************************/

/* Forward reference to the user-defined instance struct: */
typedef struct cfpImpl	cfpImpl;


/* This struct must be included as the first field of your instance struct: */
typedef struct cfpImplHeader {
	const struct cfpInterface*	vtablecfp;
	jint		refcount;
} cfpImplHeader;

/*******************************************************************************
 * Instance Casting Macros
 * These macros get your back to the top of your instance, cfp,
 * given a pointer to one of its interfaces.
 ******************************************************************************/

#undef  cfpImpl2nffp
#define cfpImpl2nffp(cfpImplPtr) \
	((nffp*)((char*)(cfpImplPtr) + offsetof(cfpImplHeader, vtablecfp)))

#undef  nffp2cfpImpl
#define nffp2cfpImpl(nffpPtr) \
	((cfpImpl*)((char*)(nffpPtr) - offsetof(cfpImplHeader, vtablecfp)))

#undef  cfpImpl2cfp
#define cfpImpl2cfp(cfpImplPtr) \
	((cfp*)((char*)(cfpImplPtr) + offsetof(cfpImplHeader, vtablecfp)))

#undef  cfp2cfpImpl
#define cfp2cfpImpl(cfpPtr) \
	((cfpImpl*)((char*)(cfpPtr) - offsetof(cfpImplHeader, vtablecfp)))

/*******************************************************************************
 * Operations you must implement
 ******************************************************************************/


extern JMC_PUBLIC_API(void*)
_cfp_getBackwardCompatibleInterface(struct cfp* self, const JMCInterfaceID* iid,
	JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cfp_init(struct cfp* self, JMCException* *exception, struct nffbp* a);

extern JMC_PUBLIC_API(void*)
_cfp_getInterface(struct cfp* self, jint op, const JMCInterfaceID* a, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cfp_addRef(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cfp_release(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cfp_hashCode(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(jbool)
_cfp_equals(struct cfp* self, jint op, void* a, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_clone(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_cfp_toString(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void)
_cfp_finalize(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_LookupFont(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, const char* c, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_CreateFontFromFile(struct cfp* self, jint op, struct nfrc* a, const char* b, const char* c, const char* d, JMCException* *exception);

extern JMC_PUBLIC_API(struct nfstrm*)
_cfp_CreateFontStreamHandler(struct cfp* self, jint op, struct nfrc* a, const char* b, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_EnumerateSizes(struct cfp* self, jint op, struct nfrc* a, void* b, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cfp_ReleaseFontHandle(struct cfp* self, jint op, void* a, JMCException* *exception);

extern JMC_PUBLIC_API(struct nffmi*)
_cfp_GetMatchInfo(struct cfp* self, jint op, void* a, JMCException* *exception);

extern JMC_PUBLIC_API(struct nfrf*)
_cfp_GetRenderableFont(struct cfp* self, jint op, struct nfrc* a, void* b, jdouble c, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_cfp_Name(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_cfp_Description(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(const char*)
_cfp_EnumerateMimeTypes(struct cfp* self, jint op, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_ListFonts(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, JMCException* *exception);

extern JMC_PUBLIC_API(void*)
_cfp_ListSizes(struct cfp* self, jint op, struct nfrc* a, struct nffmi* b, JMCException* *exception);

extern JMC_PUBLIC_API(jint)
_cfp_CacheFontInfo(struct cfp* self, jint op, JMCException* *exception);

/*******************************************************************************
 * Factory Operations
 ******************************************************************************/

JMC_PUBLIC_API(cfp*)
cfpFactory_Create(JMCException* *exception, struct nffbp* a);

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _Mcfp_H_ */
