/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
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
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsDeviceContextSpecFactoryP.h"
#include "nsDeviceContextSpecPh.h"
#include "nsGfxCIID.h"
#include "plstr.h"
#include "nsIServiceManager.h"

#include "nsIPrintOptions.h"

#include <Pt.h>
#include "nsPhGfxLog.h"

nsDeviceContextSpecFactoryPh :: nsDeviceContextSpecFactoryPh()
{
  NS_INIT_REFCNT();
}

nsDeviceContextSpecFactoryPh :: ~nsDeviceContextSpecFactoryPh()
{
}

static NS_DEFINE_IID(kIDeviceContextSpecIID, NS_IDEVICE_CONTEXT_SPEC_IID);
static NS_DEFINE_IID(kDeviceContextSpecCID, NS_DEVICE_CONTEXT_SPEC_CID);
static NS_DEFINE_CID(kPrintOptionsCID, NS_PRINTOPTIONS_CID);

NS_IMPL_ISUPPORTS1(nsDeviceContextSpecFactoryPh, nsIDeviceContextSpecFactory)

NS_IMETHODIMP nsDeviceContextSpecFactoryPh :: Init(void)
{
  return NS_OK;
}

//XXX this method needs to do what the API says...

NS_IMETHODIMP nsDeviceContextSpecFactoryPh :: CreateDeviceContextSpec(nsIWidget *aWidget,
                                  nsIPrintSettings* aPrintSettings,
																	nsIDeviceContextSpec *&aNewSpec,
																	PRBool aQuiet)
{
	NS_ENSURE_ARG_POINTER(aWidget);
	PpPrintContext_t *pc = NULL;

	nsresult  rv = NS_ERROR_FAILURE;
	nsIDeviceContextSpec  *devSpec = nsnull;

	nsComponentManager::CreateInstance(kDeviceContextSpecCID, nsnull, kIDeviceContextSpecIID, (void **)&devSpec);

	if (devSpec != nsnull)
	{
		PtWidget_t *widget = (PtWidget_t*) aWidget->GetNativeData( NS_NATIVE_WIDGET );
		PtWidget_t *disjoint = PtFindDisjoint( widget );
		if( !PtWidgetIsClass( disjoint, PtWindow ) ) 
		{
			PRInt32	range = 0;

			aQuiet = 1; /* for the embedding stuff, the PrintSelection dialog is displayed by the client */
			/* check for the page range if we are called by an embedded app, the pc is there */
			aPrintSettings->GetEndPageRange(&range);
			if (range)
				pc = (PpPrintContext_t *) range;
		}

		nsDeviceContextSpecPh* specPh = NS_STATIC_CAST(nsDeviceContextSpecPh*, devSpec);
		if (pc)
		{
			specPh->SetPrintContext(pc);
			printf("Set print context: %X\n", pc);
		}
		rv = specPh->Init(aWidget, aPrintSettings, aQuiet);
		if (NS_SUCCEEDED(rv)) {
  			aNewSpec = devSpec;
  		} else {
    		NS_RELEASE(devSpec);
		}
	}

	return rv;
}
