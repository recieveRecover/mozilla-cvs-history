/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#ifndef nsGfxCIID_h__
#define nsGfxCIID_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

#define NS_IMAGE_CID \
{ 0x6049b260, 0xc1e6, 0x11d1, \
{ 0xa8, 0x27, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_RENDERING_CONTEXT_CID \
{ 0x6049b261, 0xc1e6, 0x11d1, \
{ 0xa8, 0x27, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_DEVICE_CONTEXT_CID \
{ 0x6049b262, 0xc1e6, 0x11d1, \
{ 0xa8, 0x27, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_FONT_METRICS_CID \
{ 0x6049b263, 0xc1e6, 0x11d1, \
{ 0xa8, 0x27, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_FONT_ENUMERATOR_CID \
{ 0xa6cf9115, 0x15b3, 0x11d2, \
{ 0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } }

#define NS_REGION_CID \
{ 0xe12752f0, 0xee9a, 0x11d1, \
{ 0xa8, 0x2a, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_SCRIPTABLE_REGION_CID \
{ 0xda5b130a, 0x1dd1, 0x11b2, \
{ 0xad, 0x47, 0xf4, 0x55, 0xb1, 0x81, 0x4a, 0x78 } }

#define NS_BLENDER_CID \
{ 0x6049b264, 0xc1e6, 0x11d1, \
{ 0xa8, 0x27, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_DEVICE_CONTEXT_SPEC_CID \
{ 0xd7193600, 0x78e0, 0x11d2, \
{ 0xa8, 0x46, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_DEVICE_CONTEXT_SPEC_FACTORY_CID \
{ 0xec5bebb0, 0x7b51, 0x11d2, \
{ 0xa8, 0x48, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_DRAWING_SURFACE_CID \
{ 0x199c7040, 0xcab0, 0x11d2, \
{ 0xa8, 0x49, 0x00, 0x40, 0x95, 0x9a, 0x28, 0xc9 } }

#define NS_IMAGEMANAGER_CID    \
{ 0x140d2dd1, 0x96f4, 0x11d3,    \
{ 0x8a, 0xf3, 0x0, 0x10, 0x5a, 0x1b, 0x88, 0x60 } }

#define NS_SCREENMANAGER_CID \
{ 0xc401eb80, 0xf9ea, 0x11d3, \
{ 0xbb, 0x6f, 0xe7, 0x32, 0xb7, 0x3e, 0xbe, 0x7c } }

#define NS_PRINTOPTIONS_CID \
{ 0x30a3b080, 0x4867, 0x11d4, \
{ 0xa8, 0x56, 0x0, 0x10, 0x5a, 0x18, 0x34, 0x19 } };


#endif
