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

#ifndef nsMessageBaseCID_h__
#define nsMessageBaseCID_h__

#include "nsISupports.h"
#include "nsIFactory.h"
#include "nsIComponentManager.h"

#define NS_MSGFOLDEREVENT_CID				              \
{ /* FBFEBE7A-C1DD-11d2-8A40-0060B0FC04D2 */      \
 0xfbfebe7a, 0xc1dd, 0x11d2,                      \
 {0x8a, 0x40, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}

#define NS_MSGGROUPRECORD_CID                     \
{ /* a8f54ee0-d292-11d2-b7f6-00805f05ffa5 */      \
 0xa8f54ee0, 0xd292, 0x11d2,                      \
 {0xb7, 0xf6, 0x00, 0x80, 0x5f, 0x05, 0xff, 0xa5}}

//
// nsMsgFolderDataSource
//
#define NS_MAILNEWSFOLDERDATASOURCE_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "mailnewsfolders"

#define NS_MAILNEWSFOLDERDATASOURCE_CID                    \
{ /* 2B8ED4A4-F684-11d2-8A5D-0060B0FC04D2 */         \
    0x2b8ed4a4,                                      \
    0xf684,                                          \
    0x11d2,                                          \
    {0x8a, 0x5d, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2} \
}

//
// nsMsgMessageDataSource
//
#define NS_MAILNEWSMESSAGEDATASOURCE_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "mailnewsmessages"

#define NS_MAILNEWSMESSAGEDATASOURCE_CID                    \
{ /* 2B8ED4A5-F684-11d2-8A5D-0060B0FC04D2 */         \
    0x2b8ed4a5,                                      \
    0xf684,                                          \
    0x11d2,                                          \
    {0x8a, 0x5d, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2} \
}


//
// nsMessageViewDataSource
//
#define NS_MESSAGEVIEWDATASOURCE_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "mail-messageview"

#define NS_MESSAGEVIEWDATASOURCE_CID				\
{ /* 14495573-E945-11d2-8A52-0060B0FC04D2 */		\
0x14495573, 0xe945, 0x11d2,							\
{0x8a, 0x52, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}


//
// nsMsgAccountManager
// 
#define NS_MSGACCOUNTMANAGER_PROGID \
  "component://netscape/messenger/account-manager"

#define NS_MSGACCOUNTMANAGER_CID									\
{ /* D2876E50-E62C-11d2-B7FC-00805F05FFA5 */			\
 0xd2876e50, 0xe62c, 0x11d2,											\
 {0xb7, 0xfc, 0x0, 0x80, 0x5f, 0x5, 0xff, 0xa5 }}

// 
// nsMessengerMigrator
//
#define NS_MESSENGERMIGRATOR_PROGID \
	"component://netscape/messenger/migrator"

#define NS_MESSENGERMIGRATOR_CID	\
{ /* 54818d98-1dd2-11b2-82aa-a9197f997503 */	\
	0x54818d98, 0x1dd2, 0x11b2,	\
	{ 0x82, 0xaa, 0xa9, 0x19, 0x7f, 0x99, 0x75, 0x03}}



//
// nsMsgIdentity
//
#define NS_MSGIDENTITY_PROGID \
  "component://netscape/messenger/identity"

#define NS_MSGIDENTITY_CID												\
{ /* 8fbf6ac0-ebcc-11d2-b7fc-00805f05ffa5 */			\
 0x8fbf6ac0, 0xebcc, 0x11d2,											\
 {0xb7, 0xfc, 0x0, 0x80, 0x5f, 0x5, 0xff, 0xa5 }}

//
// nsMsgIncomingServer
#define NS_MSGINCOMINGSERVER_PROGID_PREFIX \
  "component://netscape/messenger/server&type="

#define NS_MSGINCOMINGSERVER_PROGID \
  NS_MSGINCOMINGSERVER_PROGID_PREFIX "generic"

/* {66e5ff08-5126-11d3-9711-006008948010} */
#define NS_MSGINCOMINGSERVER_CID \
  {0x66e5ff08, 0x5126, 0x11d3, \
    {0x97, 0x11, 0x00, 0x60, 0x08, 0x94, 0x80, 0x10}}


//
// nsMsgAccount
//
#define NS_MSGACCOUNT_PROGID \
  "component://netscape/messenger/account"

#define NS_MSGACCOUNT_CID													\
{ /* 68b25510-e641-11d2-b7fc-00805f05ffa5 */			\
 0x68b25510, 0xe641, 0x11d2,											\
 {0xb7, 0xfc, 0x0, 0x80, 0x5f, 0x5, 0xff, 0xa5 }}

//
// nsMsgFilterService
//
#define NS_MSGFILTERSERVICE_PROGID \
  "component://netscape/messenger/services/filters"

#define NS_MSGFILTERSERVICE_CID                         \
{ 0x5cbb0700, 0x04bc, 0x11d3,                 \
    { 0xa5, 0x0a, 0x0, 0x60, 0xb0, 0xfc, 0x04, 0xb7 } }


//
// nsMsgSearchSession
//
/* e9a7cd70-0303-11d3-a50a-0060b0fc04b7 */
#define NS_MSGSEARCHSESSION_CID						  \
{ 0xe9a7cd70, 0x0303, 0x11d3,                 \
    { 0xa5, 0x0a, 0x0, 0x60, 0xb0, 0xfc, 0x04, 0xb7 } }

//
// nsMsgMailSession
//
#define NS_MSGMAILSESSION_PROGID \
  "component://netscape/messenger/services/session"

/* D5124441-D59E-11d2-806A-006008128C4E */
#define NS_MSGMAILSESSION_CID							\
{ 0xd5124441, 0xd59e, 0x11d2,							\
    { 0x80, 0x6a, 0x0, 0x60, 0x8, 0x12, 0x8c, 0x4e } }

//
// nsMsgBiffManager
//
#define NS_MSGBIFFMANAGER_PROGID \
  "component://netscape/messenger/biffManager"

/* 4A374E7E-190F-11d3-8A88-0060B0FC04D2 */
#define NS_MSGBIFFMANAGER_CID							\
{ 0x4a374e7e, 0x190f, 0x11d3,							\
    { 0x8a, 0x88, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2 } }

//
// nsMsgNotificationManager
//
#define NS_MSGNOTIFICATIONMANAGER_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "msgnotifications"

/* 7C601F60-1EF3-11d3-9574-006097222B83 */
#define NS_MSGNOTIFICATIONMANAGER_CID							\
{ 0x7c601f60, 0x1ef3, 0x11d3,							\
    { 0x95, 0x74, 0x0, 0x60, 0x97, 0x22, 0x2b, 0x83 } }

//
// nsCopyMessageStreamListener
//
#define NS_COPYMESSAGESTREAMLISTENER_PROGID \
  "component://netscape/messenger/copymessagestreamlistener"

#define NS_COPYMESSAGESTREAMLISTENER_CID							\
{ 0x7741daed, 0x2125, 0x11d3,							\
    { 0x8a, 0x90, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2 } }

//
// nsMsgCopyService
//
#define NS_MSGCOPYSERVICE_PROGID \
  "component://netscape/messenger/messagecopyservice"

/* c766e666-29bd-11d3-afb3-001083002da8 */
#define NS_MSGCOPYSERVICE_CID \
{ 0xc766e666, 0x29bd, 0x11d3, \
    { 0xaf, 0xb3, 0x00, 0x10, 0x83, 0x00, 0x2d, 0xa8 } }

#define NS_MSGFOLDERCACHE_PROGID \
	"component://netscape/messenger/msgFolderCache"

/* bcdca970-3b22-11d3-8d76-00805f8a6617 */
#define NS_MSGFOLDERCACHE_CID \
{ 0xbcdca970, 0x3b22, 0x11d3,  \
	{ 0x8d, 0x76, 0x00, 0x80, 0xf5, 0x8a, 0x66, 0x17 } }

//
// nsUrlListenerManager
//
#define NS_URLLISTENERMANAGER_PROGID \
  "component://netscape/messenger/urlListenerManager"

/* B1AA0820-D04B-11d2-8069-006008128C4E */
#define NS_URLLISTENERMANAGER_CID \
{ 0xb1aa0820, 0xd04b, 0x11d2, \
  {0x80, 0x69, 0x0, 0x60, 0x8, 0x12, 0x8c, 0x4e} }

//
// nsMessengerBootstrap
//
#define NS_MESSENGERBOOTSTRAP_PROGID \
  "component://netscape/appshell/component/messenger"
#define NS_MAILSTARTUPHANDLER_PROGID \
  "component://netscape/commandlinehandler/general-startup-mail"

//
// nsMessenger
//
#define NS_MESSENGER_PROGID	\
  "component://netscape/messenger"

//
// nsMsgStatusFeedback
//
#define NS_MSGSTATUSFEEDBACK_PROGID \
  "component://netscape/messenger/statusfeedback"

/* B1AA0820-D04B-11d2-8069-006008128C4E */
#define NS_MSGSTATUSFEEDBACK_CID \
{ 0xbd85a417, 0x5433, 0x11d3, \
  {0x8a, 0xc5, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2} }

//
//nsMessageView
//
#define NS_MESSAGEVIEW_PROGID \
	"component://netscape/messenger/messageview"

/* 4E03B3A6-624A-11d3-8AD4-0060B0FC04D2*/
#define NS_MESSAGEVIEW_CID \
{ 0x4e03b3a6, 0x624a, 0x11d3, \
  { 0x8a, 0xd4, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}

//
//nsMsgWindow
//
#define NS_MSGWINDOW_PROGID \
	"component://netscape/messenger/msgwindow"

/* BB460DFF-8BF0-11d3-8AFE-0060B0FC04D2*/
#define NS_MSGWINDOW_CID \
{ 0xbb460dff, 0x8bf0, 0x11d3, \
  { 0x8a, 0xfe, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}

//
//nsMsgViewNavigationService
//
#define NS_MSGVIEWNAVIGATIONSERVICE_PROGID \
	"component://netscape/messenger/msgviewnavigationservice"

/* 60D34FB4-D031-11d3-8B2E-0060B0FC04D2*/
#define NS_MSGVIEWNAVIGATIONSERVICE_CID \
{ 0x60d34fb4, 0xd031, 0x11d3, \
  { 0x8b, 0x2e, 0x0, 0x60, 0xb0, 0xfc, 0x4, 0xd2}}

//
// Print Engine...
//
#define NS_MSGPRINTENGINE_PROGID \
  "component://netscape/messenger/msgPrintEngine"

#define NS_MSG_PRINTENGINE_CID                  \
  { /* 91FD6B19-E0BC-11d3-8F97-000064657374 */  \
    0x91fd6b19, 0xe0bc, 0x11d3,			\
  { 0x8f, 0x97, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } };

//
// nsMsgServiceProviderService
//
#define NS_MSGSERVICEPROVIDERSERVICE_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "ispdefaults"
  
/* 10998cef-d7f2-4772-b7db-bd097454984c */
#define NS_MSGSERVICEPROVIDERSERVICE_CID \
{ 0x10998cef, 0xd7f2, 0x4772, \
  { 0xb7, 0xdb, 0xbd, 0x09, 0x74, 0x54, 0x98, 0x4c}}

#define NS_MSGLOGONREDIRECTORSERVICE_PROGID \
	"component://netscape/messenger/msglogonredirector"

#define NS_MSGLOGONREDIRECTORSERVICE_CID \
{0x0d7456ae, 0xe28a, 0x11d3, \
  {0xa5, 0x60, 0x00, 0x60, 0xb0, 0xfc, 0x04, 0xb7}}

//
// nsSubscribeDataSource
//
#define NS_SUBSCRIBEDATASOURCE_PROGID \
  NS_RDF_DATASOURCE_PROGID_PREFIX "subscribe"

/* 00e89c82-1dd2-11b2-9a1c-e75995d7d595 */
#define NS_SUBSCRIBEDATASOURCE_CID \
{ 0x00e89c82, 0x1dd2, 0x11b2, \
  { 0x9a, 0x1c, 0xe7, 0x59, 0x95, 0xd7, 0xd5, 0x95}} 

//
// delegate factory
//

/* c6584cee-8ee8-4b2c-8dbe-7dfcb55c9c61 */
#define NS_MSGFILTERDELEGATEFACTORY_CID \
  {0xc6584cee, 0x8ee8, 0x4b2c, \
    { 0x8d, 0xbe, 0x7d, 0xfc, 0xb5, 0x5c, 0x9c, 0x61 }}

#define NS_MSGFILTERDELEGATEFACTORY_PROGID_PREFIX \
  NS_RDF_DELEGATEFACTORY_PROGID_PREFIX "filter" "."   

// Note: the above CID should live in base, but each protocol
// should be creating the ProgID themselves. for now we'll
// do it for news/imap/local mail

#define NS_MSGFILTERDELEGATEFACTORY_MAILBOX_PROGID \
  NS_MSGFILTERDELEGATEFACTORY_PROGID_PREFIX "mailbox"

#define NS_MSGFILTERDELEGATEFACTORY_NEWS_PROGID \
  NS_MSGFILTERDELEGATEFACTORY_PROGID_PREFIX "news"

#define NS_MSGFILTERDELEGATEFACTORY_IMAP_PROGID \
  NS_MSGFILTERDELEGATEFACTORY_PROGID_PREFIX "imap"


#endif // nsMessageBaseCID_h__
