/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

/*

  XPCOM Class IDs for RDF objects that can be constructed via the RDF
  factory.

 */

#ifndef nsRDFCID_h__
#define nsRDFCID_h__

// {0F78DA56-8321-11d2-8EAC-00805F29F370}
#define NS_RDFDEFAULTRESOURCE_CID \
{ 0xf78da56, 0x8321, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {BFD05264-834C-11d2-8EAC-00805F29F370}
#define NS_RDFSERVICE_CID \
{ 0xbfd05264, 0x834c, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {BFD05264-834C-11d2-8EAC-00805F29F371}
#define NS_XULSORTSERVICE_CID \
{ 0xbfd05264, 0x834c, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x71 } }

// {BFD0526D-834C-11d2-8EAC-00805F29F370}
#define NS_RDFINMEMORYDATASOURCE_CID \
{ 0xbfd0526d, 0x834c, 0x11d2, { 0x8e, 0xac, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {E638D760-8687-11d2-B530-000000000001}
#define NS_RDFFILESYSTEMDATASOURCE_CID \
{ 0xe638d760, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01 } }

// {6bd1d807-1c67-11d3-9820-ed1b357eb3c4}
#define NS_RDFSEARCHDATASOURCE_CID \
{ 0x6bd1d807, 0x1c67, 0x11d3, { 0x98, 0x20, 0xed, 0x1b, 0x35, 0x7e, 0xb3, 0xc4 } }

// {E638D760-8687-11d2-B530-000000000002}
#define NS_RDFFINDDATASOURCE_CID \
{ 0xe638d760, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02 } }

// {9ebd2e83-e2fc-11d2-9820-c7de17d21fbc}
#define NS_RDFFTPDATASOURCE_CID \
{ 0x9ebd2e83, 0xe2fc, 0x11d2, { 0x98, 0x20, 0xc7, 0xde, 0x17, 0xd2, 0x1f, 0xbc } }

// {dc2fb186-f7ff-11d2-9820-f65ea652ae3c}
#define NS_RDFRELATEDLINKSDATASOURCE_CID \
{ 0xdc2fb186, 0xf7ff, 0x11d2, { 0x98, 0x20, 0xf6, 0x5e, 0xa6, 0x52, 0xae, 0x3c } }

// {E638D761-8687-11d2-B530-000000000000}
#define NS_RDFCOMPOSITEDATASOURCE_CID \
{ 0xe638d761, 0x8687, 0x11d2, { 0xb5, 0x30, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }

// {954F0813-81DC-11d2-B52A-000000000000}
#define NS_RDFHTMLBUILDER_CID \
{ 0x954f0813, 0x81dc, 0x11d2, { 0xb5, 0x2a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }

// {3D262D00-8B5A-11d2-8EB0-00805F29F370}
#define NS_RDFTREEBUILDER_CID \
{ 0x3d262d00, 0x8b5a, 0x11d2, { 0x8e, 0xb0, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {CF6547E1-D427-11d2-96ED-00104B7B7DEB}
#define NS_RDFMENUBUILDER_CID \
{ 0xcf6547e1, 0xd427, 0x11d2, { 0x96, 0xed, 0x0, 0x10, 0x4b, 0x7b, 0x7d, 0xeb } }

// {FEA36A61-D48F-11d2-96ED-00104B7B7DEB}
#define NS_RDFTOOLBARBUILDER_CID \
{ 0xfea36a61, 0xd48f, 0x11d2, { 0x96, 0xed, 0x0, 0x10, 0x4b, 0x7b, 0x7d, 0xeb } }

// {7BAF62E0-8E61-11d2-8EB1-00805F29F370}
#define NS_RDFXMLDATASOURCE_CID \
{ 0x7baf62e0, 0x8e61, 0x11d2, { 0x8e, 0xb1, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {6791B601-B9FE-11d2-BF86-00105A1B0627}
#define NS_XULDATASOURCE_CID \
{ 0x6791b601, 0xb9fe, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {0958B101-9ADA-11d2-8EBC-00805F29F370}
#define NS_RDFCONTENTSINK_CID \
{ 0x958b101, 0x9ada, 0x11d2, { 0x8e, 0xbc, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {E45313F0-B59C-11d2-A68C-00104BDE6048}
#define NS_RDFXULBUILDER_CID \
{ 0xe45313f0, 0xb59c, 0x11d2, { 0xa6, 0x8c, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

// {CE058B21-BA9C-11d2-BF86-00105A1B0627}
#define NS_XULCONTENTSINK_CID \
{ 0xce058b21, 0xba9c, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {541AFCB2-A9A3-11d2-8EC5-00805F29F370}
#define NS_XULDOCUMENT_CID \
{ 0x541afcb2, 0xa9a3, 0x11d2, { 0x8e, 0xc5, 0x0, 0x80, 0x5f, 0x29, 0xf3, 0x70 } }

// {F53279E1-D7A4-11d2-BF86-00105A1B0627}
#define NS_XULDOCUMENTINFO_CID \
{ 0xf53279e1, 0xd7a4, 0x11d2, { 0xbf, 0x86, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {1EAAFD60-D596-11d2-80BE-006097B76B8E}
#define NS_RDFHISTORYDATASOURCE_CID \
{ 0x1eaafd60, 0xd596, 0x11d2, { 0x80, 0xbe, 0x0, 0x60, 0x97, 0xb7, 0x6b, 0x8e } }

// {DF71C6F0-EC53-11d2-BDCA-000064657374}
#define NS_LOCALSTORE_CID \
{ 0xdf71c6f0, 0xec53, 0x11d2, { 0xbd, 0xca, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

// {D4214E92-FB94-11d2-BDD8-00104BDE6048}
#define NS_RDFCONTAINERUTILS_CID \
{ 0xd4214e92, 0xfb94, 0x11d2, { 0xbd, 0xd8, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

// {D4214E93-FB94-11d2-BDD8-00104BDE6048}
#define NS_RDFCONTAINER_CID \
{ 0xd4214e93, 0xfb94, 0x11d2, { 0xbd, 0xd8, 0x0, 0x10, 0x4b, 0xde, 0x60, 0x48 } }

// {3986B301-097C-11d3-BF87-00105A1B0627}
#define NS_XULPOPUPLISTENER_CID \
{ 0x3986b301, 0x97c, 0x11d3, { 0xbf, 0x87, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {FE71D561-1501-11d3-BF87-00105A1B0627}
#define NS_XULFOCUSTRACKER_CID \
{ 0xfe71d561, 0x1501, 0x11d3, { 0xbf, 0x87, 0x0, 0x10, 0x5a, 0x1b, 0x6, 0x27 } }

// {9e246e80-2064-11d3-973e-00805f916fd3}
#define NS_XULKEYLISTENER_CID \
{ 0x9e246e80, 0x2064, 0x11d3, {0x97, 0x3e, 0x00, 0x80, 0x5f, 0x91, 0x6f, 0xd3} }

#endif // nsRDFCID_h__
