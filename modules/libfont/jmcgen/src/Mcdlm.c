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
 * netscape/fonts/cdlm module C stub file
 * Generated by jmc version 1.8 -- DO NOT EDIT
 ******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "xp_mem.h"

/* Include the implementation-specific header: */
#include "Pcdlm.h"

/* Include other interface headers: */

/*******************************************************************************
 * cdlm Methods
 ******************************************************************************/

#ifndef OVERRIDE_cdlm_getInterface
JMC_PUBLIC_API(void*)
_cdlm_getInterface(struct cdlm* self, jint op, const JMCInterfaceID* iid, JMCException* *exc)
{
	if (memcmp(iid, &cdlm_ID, sizeof(JMCInterfaceID)) == 0)
		return cdlmImpl2cdlm(cdlm2cdlmImpl(self));
	return _cdlm_getBackwardCompatibleInterface(self, iid, exc);
}
#endif

#ifndef OVERRIDE_cdlm_addRef
JMC_PUBLIC_API(void)
_cdlm_addRef(struct cdlm* self, jint op, JMCException* *exc)
{
	cdlmImplHeader* impl = (cdlmImplHeader*)cdlm2cdlmImpl(self);
	impl->refcount++;
}
#endif

#ifndef OVERRIDE_cdlm_release
JMC_PUBLIC_API(void)
_cdlm_release(struct cdlm* self, jint op, JMCException* *exc)
{
	cdlmImplHeader* impl = (cdlmImplHeader*)cdlm2cdlmImpl(self);
	if (--impl->refcount == 0) {
		cdlm_finalize(self, exc);
	}
}
#endif

#ifndef OVERRIDE_cdlm_hashCode
JMC_PUBLIC_API(jint)
_cdlm_hashCode(struct cdlm* self, jint op, JMCException* *exc)
{
	return (jint)self;
}
#endif

#ifndef OVERRIDE_cdlm_equals
JMC_PUBLIC_API(jbool)
_cdlm_equals(struct cdlm* self, jint op, void* obj, JMCException* *exc)
{
	return self == obj;
}
#endif

#ifndef OVERRIDE_cdlm_clone
JMC_PUBLIC_API(void*)
_cdlm_clone(struct cdlm* self, jint op, JMCException* *exc)
{
	cdlmImpl* impl = cdlm2cdlmImpl(self);
	cdlmImpl* newImpl = (cdlmImpl*)malloc(sizeof(cdlmImpl));
	if (newImpl == NULL) return NULL;
	memcpy(newImpl, impl, sizeof(cdlmImpl));
	((cdlmImplHeader*)newImpl)->refcount = 1;
	return newImpl;
}
#endif

#ifndef OVERRIDE_cdlm_toString
JMC_PUBLIC_API(const char*)
_cdlm_toString(struct cdlm* self, jint op, JMCException* *exc)
{
	return NULL;
}
#endif

#ifndef OVERRIDE_cdlm_finalize
JMC_PUBLIC_API(void)
_cdlm_finalize(struct cdlm* self, jint op, JMCException* *exc)
{
	/* Override this method and add your own finalization here. */
	XP_FREEIF(self);
}
#endif

/*******************************************************************************
 * Jump Tables
 ******************************************************************************/

const struct cdlmInterface cdlmVtable = {
	_cdlm_getInterface,
	_cdlm_addRef,
	_cdlm_release,
	_cdlm_hashCode,
	_cdlm_equals,
	_cdlm_clone,
	_cdlm_toString,
	_cdlm_finalize,
	_cdlm_SupportsInterface,
	_cdlm_CreateObject,
	_cdlm_OnUnload
};

/*******************************************************************************
 * Factory Operations
 ******************************************************************************/

JMC_PUBLIC_API(cdlm*)
cdlmFactory_Create(JMCException* *exception)
{
	cdlmImplHeader* impl = (cdlmImplHeader*)XP_NEW_ZAP(cdlmImpl);
	cdlm* self;
	if (impl == NULL) {
		JMC_EXCEPTION(exception, JMCEXCEPTION_OUT_OF_MEMORY);
		return NULL;
	}
	self = cdlmImpl2cdlm(impl);
	impl->vtablecdlm = &cdlmVtable;
	impl->refcount = 1;
	_cdlm_init(self, exception);
	if (JMC_EXCEPTION_RETURNED(exception)) {
		XP_FREE(impl);
		return NULL;
	}
	return self;
}

