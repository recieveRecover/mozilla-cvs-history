/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef nsLayoutCID_h__
#define nsLayoutCID_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

// {1691E1F4-EE41-11d4-9885-00C04FA0CF4B}
#define NS_FRAMETRAVERSAL_CID \
{ 0x1691e1f4, 0xee41, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

/* a6cf90fa-15b3-11d2-932e-00805f8add32 */
#define NS_LAYOUT_DOCUMENT_LOADER_FACTORY_CID \
 { 0xa6cf90fa, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/* a6cf90f9-15b3-11d2-932e-00805f8add32 */
#define NS_LAYOUT_DEBUGGER_CID \
 { 0xa6cf90f9, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

#define NS_HTMLDOCUMENT_CID                       \
{ /* 5d0fcdd0-4daa-11d2-b328-00805f8a3859 */      \
 0x5d0fcdd0, 0x4daa, 0x11d2,                      \
 {0xb3, 0x28, 0x00, 0x80, 0x5f, 0x8a, 0x38, 0x59}}

#define NS_XMLDOCUMENT_CID                        \
{ /* a6cf9063-15b3-11d2-932e-00805f8add32 */      \
 0xa6cf9063, 0x15b3, 0x11d2,                      \
 {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

#define NS_IMAGEDOCUMENT_CID                      \
{ /* e11a6080-4daa-11d2-b328-00805f8a3859 */      \
 0xe11a6080, 0x4daa, 0x11d2,                      \
 {0xb3, 0x28, 0x00, 0x80, 0x5f, 0x8a, 0x38, 0x59}}

#define NS_HTMLIMAGEELEMENT_CID                   \
{ /* d6008c40-4dad-11d2-b328-00805f8a3859 */      \
 0xd6008c40, 0x4dad, 0x11d2,                      \
 {0xb3, 0x28, 0x00, 0x80, 0x5f, 0x8a, 0x38, 0x59}}

#define NS_HTMLOPTIONELEMENT_CID                  \
{ /* a6cf90f5-15b3-11d2-932e-00805f8add32 */      \
 0xa6cf90f5, 0x15b3, 0x11d2,                      \
 {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

#define NS_NAMESPACEMANAGER_CID                   \
{ /* d9783472-8fe9-11d2-9d3c-0060088f9ff7 */      \
 0xd9783472, 0x8fe9, 0x11d2,                      \
 {0x9d, 0x3c, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7}}

/* a6cf90d7-15b3-11d2-932e-00805f8add32 */
#define NS_FRAME_UTIL_CID \
 { 0xa6cf90d5, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}


// XXX This should really be factored into a style-specific DLL so
// that all the HTML, generic layout, and style stuff isn't munged
// together.

// {2E363D60-872E-11d2-B531-000000000000}
#define NS_CSSPARSER_CID \
{ 0x2e363d60, 0x872e, 0x11d2, { 0xb5, 0x31, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }

// {E6FD9940-899D-11d2-8EAE-00805F29F370}
#define NS_PRESSHELL_CID \
{ 0xe6fd9940, 0x899d, 0x11d2, { 0x8e, 0xae, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {A1FDE861-E802-11d4-9885-00C04FA0CF4B}
#define NS_PRESSTATE_CID \
{ 0xa1fde861, 0xe802, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

// {A1FDE85E-E802-11d4-9885-00C04FA0CF4B}
#define NS_GALLEYCONTEXT_CID \
{ 0xa1fde85e, 0xe802, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

// {A1FDE85F-E802-11d4-9885-00C04FA0CF4B}
#define NS_PRINTCONTEXT_CID \
{ 0xa1fde85f, 0xe802, 0x11d4, { 0x98, 0x85, 0x0, 0xc0, 0x4f, 0xa0, 0xcf, 0x4b } }

// {95F46161-D177-11d2-BF86-00105A1B0627}
#define NS_HTML_CSS_STYLESHEET_CID \
{ 0x95f46161, 0xd177, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {eaca2576-0d4a-11d3-9d7e-0060088f9ff7}
#define NS_CSS_LOADER_CID \
{ 0xeaca2576, 0x0d4a, 0x11d3, { 0x9d, 0x7e, 0x00, 0x60, 0x08, 0x8f, 0x9f, 0xf7 } }

// {96882B71-8A27-11d2-8EAF-00805F29F370}
#define NS_TEXTNODE_CID \
{ 0x96882b71, 0x8a27, 0x11d2, { 0x8e, 0xaf, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {96882B72-8A27-11d2-8EAF-00805F29F370}
#define NS_SELECTION_CID \
{ 0x96882b72, 0x8a27, 0x11d2, { 0x8e, 0xaf, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

#define NS_FRAMESELECTION_CID \
{/* {905F80F1-8A7B-11d2-918C-0080C8E44DB5}*/ \
 0x905f80f1, 0x8a7b, 0x11d2, { 0x91, 0x8c, 0x0, 0x80, 0xc8, 0xe4, 0x4d, 0xb5 } }

#define NS_DOMSELECTION_CID \
{/* {C87A37FC-8109-4ce2-A322-8CDEC925379F}*/ \
 0xc87a37fc, 0x8109, 0x4ce2, { 0xa3, 0x22, 0x8c, 0xde, 0xc9, 0x25, 0x37, 0x9f } }

#define NS_RANGE_CID \
{/* {56AD2981-8A87-11d2-918C-0080C8E44DB5}*/ \
 0x56ad2981, 0x8a87, 0x11d2, { 0x91, 0x8c, 0x0, 0x80, 0xc8, 0xe4, 0x4d, 0xb5 } }

#define NS_CONTENTITERATOR_CID \
{/* {a6cf90e3-15b3-11d2-932e-00805f8add32}*/ \
 0xa6cf90e3, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } }

#define NS_GENERATEDSUBTREEITERATOR_CID \
{/* {9A45253B-EB8F-49f1-B925-E9EA90D3EB3A}*/ \
 0x9a45253b, 0xeb8f, 0x49f1, { 0xb9, 0x25, 0xe9, 0xea, 0x90, 0xd3, 0xeb, 0x3a } }

#define NS_GENERATEDCONTENTITERATOR_CID \
{/* {A364930F-E353-49f1-AC69-91637EB8B757}*/ \
 0xa364930f, 0xe353, 0x49f1, { 0xac, 0x69, 0x91, 0x63, 0x7e, 0xb8, 0xb7, 0x57 } }

#define NS_SUBTREEITERATOR_CID \
{/* {a6cf90e5-15b3-11d2-932e-00805f8add32}*/ \
 0xa6cf90e5, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32 } }

/* a6cf90f7-15b3-11d2-932e-00805f8add32 */
#define NS_PRINT_PREVIEW_CONTEXT_CID \
 { 0xa6cf90f7, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

// {64F300A1-C88C-11d3-97FB-00400553EEF0}
#define NS_XBLSERVICE_CID \
{ 0x64f300a1, 0xc88c, 0x11d3, { 0x97, 0xfb, 0x0, 0x40, 0x5, 0x53, 0xee, 0xf0 } }

// {15671AF5-39F9-4c70-8CE3-72C97111B52D}
#define NS_BINDINGMANAGER_CID \
{ 0x15671af5, 0x39f9, 0x4c70, { 0x8c, 0xe3, 0x72, 0xc9, 0x71, 0x11, 0xb5, 0x2d } }

// {D750A964-2D14-484c-B3AA-8ED7823B5C7B}
#define NS_BOXOBJECT_CID \
{ 0xd750a964, 0x2d14, 0x484c, { 0xb3, 0xaa, 0x8e, 0xd7, 0x82, 0x3b, 0x5c, 0x7b } }

// {C2710D40-6F4D-4b7f-9778-76AE5166648C}
#define NS_LISTBOXOBJECT_CID \
{ 0xc2710d40, 0x6f4d, 0x4b7f, { 0x97, 0x78, 0x76, 0xae, 0x51, 0x66, 0x64, 0x8c } }

// {56E2ADA8-4631-11d4-BA11-001083023C1E}
#define NS_SCROLLBOXOBJECT_CID \
{ 0x56e2ada8, 0x4631, 0x11d4, { 0xba, 0x11, 0x0, 0x10, 0x83, 0x2, 0x3c, 0x1e } }

// {AA40253B-4C42-4056-8132-37BCD07862FD}
#define NS_MENUBOXOBJECT_CID \
{ 0xaa40253b, 0x4c42, 0x4056, { 0x81, 0x32, 0x37, 0xbc, 0xd0, 0x78, 0x62, 0xfd } }

// {6C392C62-1AB1-4de7-BFC6-ED4F9FC7749A}
#define NS_POPUPBOXOBJECT_CID \
{ 0x6c392c62, 0x1ab1, 0x4de7, { 0xbf, 0xc6, 0xed, 0x4f, 0x9f, 0xc7, 0x74, 0x9a } }

// {31246420-A2F7-4933-AE71-79BBD5D94B04}
#define NS_BROWSERBOXOBJECT_CID \
{ 0x31246420, 0xa2f7, 0x4933, { 0xae, 0x71, 0x79, 0xbb, 0xd5, 0xd9, 0x4b, 0x4 } }

// {8852AB1F-0AA2-46ef-A147-A908896E0D0B}
#define NS_EDITORBOXOBJECT_CID \
{ 0x8852ab1f, 0xaa2, 0x46ef, { 0xa1, 0x47, 0xa9, 0x8, 0x89, 0x6e, 0xd, 0xb } }

// {9580E69B-8FD6-414e-80CD-3A1821017646}
#define NS_IFRAMEBOXOBJECT_CID \
{ 0x9580e69b, 0x8fd6, 0x414e, { 0x80, 0xcd, 0x3a, 0x18, 0x21, 0x1, 0x76, 0x46 } }

// {3B581FD4-3497-426c-8F61-3658B971CB80}
#define NS_TREEBOXOBJECT_CID \
{ 0x3b581fd4, 0x3497, 0x426c, { 0x8f, 0x61, 0x36, 0x58, 0xb9, 0x71, 0xcb, 0x80 } }

// {8775CA39-4072-4cc0-92D3-A7C2B820089C}
#define NS_AUTOCOPYSERVICE_CID \
{ 0x8775ca39, 0x4072, 0x4cc0, { 0x92, 0xd3, 0xa7, 0xc2, 0xb8, 0x20, 0x8, 0x9c } }

// 3a9cd622-264d-11d4-ba06-0060b0fc76dd
#define NS_DOM_IMPLEMENTATION_CID \
{ 0x3a9cd622, 0x264d, 0x11d4, {0xba, 0x06, 0x00, 0x60, 0xb0, 0xfc, 0x76, 0xdd } }

// {AE52FE52-683A-437D-B661-DE55F4E0A873}
#define NS_NODEINFOMANAGER_CID \
{ 0xae52fe52, 0x683a, 0x437d, { 0xb6, 0x61, 0xde, 0x55, 0xf4, 0xe0, 0xa8, 0x73 } }

// {ECEA1B28-AE54-4047-8BBE-C624235106B4}
#define NS_COMPUTEDDOMSTYLE_CID \
{ 0xecea1b28, 0xae54, 0x4047, { 0x8b, 0xbe, 0xc6, 0x24, 0x23, 0x51, 0x06, 0xb4 } }

// {4aef38b7-6364-4e23-a5e7-12f837fbbd9c}
#define NS_XMLCONTENTSERIALIZER_CID \
{ 0x4aef38b7, 0x6364, 0x4e23, { 0xa5, 0xe7, 0x12, 0xf8, 0x37, 0xfb, 0xbd, 0x9c } }

// {9d3f70da-86e9-11d4-95ec-00b0d03e37b7}
#define NS_HTMLCONTENTSERIALIZER_CID \
{ 0x9d3f70da, 0x86e9, 0x11d4, { 0x95, 0xec, 0x00, 0xb0, 0xd0, 0x3e, 0x37, 0xb7 } }

// {6030f7ef-32ed-46a7-9a63-6a5d3f90445f}
#define NS_PLAINTEXTSERIALIZER_CID \
{ 0x6030f7ef, 0x32ed, 0x46a7, { 0x9a, 0x63, 0x6a, 0x5d, 0x3f, 0x90, 0x44, 0x5f } }

// {5C5AF390-34BE-11d5-A03B-0010A4EF48C9}
#define NS_LAYOUT_HISTORY_STATE_CID \
{ 0x5c5af390, 0x34be, 0x11d5, { 0xa0, 0x3b, 0x00, 0x10, 0xa4, 0xef, 0x48, 0xc9 } }

// {9d1001b1-e59a-456b-99dc-cc3f1283236e}
#define NS_SELECTIONIMAGESERVICE_CID \
{ 0x9d1001b1, 0xe59a, 0x456b, { 0x99, 0xdc, 0xcc, 0x3f, 0x12, 0x83, 0x23, 0x6e } }

// {E14B66F6-BFC5-11d2-B57E-00105AA83B2F}
#define NS_CARET_CID \
{ 0xe14b66f6, 0xbfc5, 0x11d2, { 0xb5, 0x7e, 0x0, 0x10, 0x5a, 0xa8, 0x3b, 0x2f } }

// {f96f5ec9-755b-447e-b1f3-717d1a84bb41}
#define NS_PLUGINDOCUMENT_CID \
{ 0xf96f5ec9, 0x755b, 0x447e, { 0xb1, 0xf3, 0x71, 0x7d, 0x1a, 0x84, 0xbb, 0x41 } }



#endif /* nsLayoutCID_h__ */
