/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
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
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributors(s):
 *   Jan Varga <varga@utcru.sk>
 *   Hakan Waara <hwaara@chello.se>
 */

//NOTE: gMessengerBundle must be defined and set or this Overlay won't work

function fillThreadPaneContextMenu()
{
  var numSelected = GetNumSelectedMessages();

  var isNewsgroup = false;
  var selectedMessage = null;

  if(numSelected >= 0) {
    selectedMessage = GetFirstSelectedMessage();
    isNewsgroup = IsNewsMessage(selectedMessage);
  }


  SetupNewMessageWindowMenuItem("threadPaneContext-openNewWindow", numSelected, false);
  SetupEditAsNewMenuItem("threadPaneContext-editAsNew", numSelected, false);

  ShowMenuItem("threadPaneContext-sep-open", (numSelected <= 1));

  SetupReplyToSenderMenuItem("threadPaneContext-replySender", numSelected, false);
  SetupReplyToNewsgroupMenuItem("threadPaneContext-replyNewsgroup", numSelected, isNewsgroup, false);
  SetupReplyAllMenuItem("threadPaneContext-replyAll", numSelected, false);
  SetupForwardMenuItem("threadPaneContext-forward", numSelected, false);
  SetupForwardAsAttachmentMenuItem("threadPaneContext-forwardAsAttachment", numSelected, false);

  ShowMenuItem("threadPaneContext-sep-reply", true);

  SetupCopyMessageUrlMenuItem("threadPaneContext-copyMessageUrl", numSelected, isNewsgroup, numSelected != 1); 
  SetupCopyMenuItem("threadPaneContext-copyMenu", numSelected, false);
  SetupMoveMenuItem("threadPaneContext-moveMenu", numSelected, isNewsgroup, false);
  SetupSaveAsMenuItem("threadPaneContext-saveAs", numSelected, false);
  SetupPrintMenuItem("threadPaneContext-print", numSelected, false);
  SetupDeleteMenuItem("threadPaneContext-delete", numSelected, isNewsgroup, false);
  SetupAddSenderToABMenuItem("threadPaneContext-addSenderToAddressBook", numSelected, false);
  SetupAddAllToABMenuItem("threadPaneContext-addAllToAddressBook", numSelected, false);


  ShowMenuItem("threadPaneContext-sep-edit", (numSelected <= 1));

  return(true);
}

function SetupNewMessageWindowMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, (numSelected == 1));
}

function SetupEditAsNewMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1)&& !forceHide);
  EnableMenuItem(menuID, (numSelected == 1));
}

function SetupReplyToSenderMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1)&& !forceHide);
  EnableMenuItem(menuID, (numSelected == 1));
}

function SetupReplyToNewsgroupMenuItem(menuID, numSelected, isNewsgroup, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && isNewsgroup && !forceHide);
  EnableMenuItem(menuID,  (numSelected == 1));
}

function SetupReplyAllMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, (numSelected == 1));
}

function SetupForwardMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID,  (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, (numSelected > 0));
}

function SetupForwardAsAttachmentMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID,  (numSelected > 1) && !forceHide);
  EnableMenuItem(menuID, (numSelected > 1));
}

function SetupMoveMenuItem(menuID, numSelected, isNewsgroup, forceHide)
{
  ShowMenuItem(menuID, !isNewsgroup && !forceHide);
  EnableMenuItem(menuID, (numSelected > 0));
}

function SetupCopyMessageUrlMenuItem(menuID, numSelected, isNewsgroup, forceHide)
{
  ShowMenuItem(menuID, isNewsgroup && !forceHide);
  EnableMenuItem(menuID, (numSelected > 0));
}

function SetupCopyMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, !forceHide);
  EnableMenuItem(menuID, (numSelected > 0));
}

function SetupSaveAsMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, (numSelected == 1));
}

function SetupPrintMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, !forceHide);
  EnableMenuItem(menuID, (numSelected > 0));
}

function SetupDeleteMenuItem(menuID, numSelected, isNewsgroup, forceHide)
{
  var showMenuItem = !forceHide;

  ShowMenuItem(menuID, showMenuItem);
  if(showMenuItem)
  {
    EnableMenuItem(menuID, (numSelected > 0));
    if(!isNewsgroup)
    {
      SetMenuItemLabel(menuID, gMessengerBundle.getString("delete"));
      SetMenuItemAccessKey(menuID, gMessengerBundle.getString("deleteAccessKey"));
    }
    else
    {
      SetMenuItemLabel(menuID, gMessengerBundle.getString("cancel"));
      SetMenuItemAccessKey(menuID, gMessengerBundle.getString("cancelAccessKey"));
    }
  }
}

function SetupAddSenderToABMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, false);
}

function SetupAddAllToABMenuItem(menuID, numSelected, forceHide)
{
  ShowMenuItem(menuID, (numSelected <= 1) && !forceHide);
  EnableMenuItem(menuID, false);
}

function fillFolderPaneContextMenu()
{
  var folderOutliner = GetFolderOutliner();
  var startIndex = {};
  var endIndex = {};
  folderOutliner.outlinerBoxObject.selection.getRangeAt(0, startIndex, endIndex);
  if (startIndex.value < 0)
    return false;
  var numSelected = endIndex.value - startIndex.value + 1;
  var folderResource = GetFolderResource(folderOutliner, startIndex.value);

  var isServer = GetFolderAttribute(folderOutliner, folderResource, "IsServer") == 'true';
  var serverType = GetFolderAttribute(folderOutliner, folderResource, "ServerType");
  var specialFolder = GetFolderAttribute(folderOutliner, folderResource, "SpecialFolder");
  var canSubscribeToFolder = (serverType == "nntp") || (serverType == "imap");
  var isNewsgroup = !isServer && serverType == 'nntp';
  var canGetMessages =  (isServer && (serverType != "nntp") && (serverType !="none")) || isNewsgroup;

  EnableMenuItem("folderPaneContext-properties", !isServer);
  ShowMenuItem("folderPaneContext-getMessages", (numSelected <= 1) && canGetMessages);
  EnableMenuItem("folderPaneContext-getMessages", true);

  ShowMenuItem("folderPaneContext-openNewWindow", (numSelected <= 1) && !isServer);
  EnableMenuItem("folderPaneContext-openNewWindow", (true));

  SetupRenameMenuItem(folderResource, numSelected, isServer, serverType, specialFolder);
  SetupRemoveMenuItem(folderResource, numSelected, isServer, serverType, specialFolder);
  SetupCompactMenuItem(folderResource, numSelected);

  ShowMenuItem("folderPaneContext-emptyTrash", (numSelected <= 1) && (specialFolder == 'Trash'));
  EnableMenuItem("folderPaneContext-emptyTrash", true);

  var showSendUnsentMessages = (numSelected <= 1) && (specialFolder == 'Unsent Messages');
  ShowMenuItem("folderPaneContext-sendUnsentMessages", showSendUnsentMessages);
  if (showSendUnsentMessages) {
    EnableMenuItem("folderPaneContext-sendUnsentMessages", IsSendUnsentMsgsEnabled(folderResource));
  }

  ShowMenuItem("folderPaneContext-sep-edit", (numSelected <= 1));

  SetupNewMenuItem(folderResource, numSelected, isServer, serverType, specialFolder);

  ShowMenuItem("folderPaneContext-subscribe", (numSelected <= 1) && canSubscribeToFolder);
  EnableMenuItem("folderPaneContext-subscribe", true);

// News folder context menu =============================================

  ShowMenuItem("folderPaneContext-newsUnsubscribe", (numSelected <= 1) && canSubscribeToFolder && isNewsgroup);
  EnableMenuItem("folderPaneContext-newsUnsubscribe", true);
  ShowMenuItem("folderPaneContext-markAllRead", (numSelected <= 1) && isNewsgroup);
  EnableMenuItem("folderPaneContext-markAllRead", true);

// End of News folder context menu =======================================

  ShowMenuItem("folderPaneContext-searchMessages", (numSelected<=1));
  EnableMenuItem("folderPaneContext-searchMessages", true);

  return(true);
}

function SetupRenameMenuItem(folderResource, numSelected, isServer, serverType, specialFolder)
{
  var isSpecialFolder = specialFolder != 'none';
  var isMail = serverType != 'nntp';
  var folderOutliner = GetFolderOutliner();
  var canRename = GetFolderAttribute(folderOutliner, folderResource, "CanRename") == "true";

  ShowMenuItem("folderPaneContext-rename", (numSelected <= 1) && !isServer && (specialFolder == "none") && canRename);
  var folder = GetMsgFolderFromResource(folderResource);
  EnableMenuItem("folderPaneContext-rename", !isServer && folder.isCommandEnabled("cmd_renameFolder"));

  if(canRename)
  {
    SetMenuItemLabel("folderPaneContext-rename", gMessengerBundle.getString("renameFolder"));
  }
}

function SetupRemoveMenuItem(folderResource, numSelected, isServer, serverType, specialFolder)
{
  var isMail = serverType != 'nntp';
  var isSpecialFolder = specialFolder != "none";
  //Can't currently delete Accounts or special folders.
  var showRemove = (numSelected <=1) && (isMail && !isSpecialFolder) && !isServer;


  ShowMenuItem("folderPaneContext-remove", showRemove);
  if(showRemove)
  {
    var folder = GetMsgFolderFromResource(folderResource);
    EnableMenuItem("folderPaneContext-remove", folder.isCommandEnabled("cmd_delete"));
  }
  if(isMail && !isSpecialFolder)
  {
    SetMenuItemLabel("folderPaneContext-remove", gMessengerBundle.getString("removeFolder"));
  }
}

function SetupCompactMenuItem(folderResource, numSelected)
{
  var folderOutliner = GetFolderOutliner();
  var canCompact = GetFolderAttribute(folderOutliner, folderResource, "CanCompact") == "true";
  ShowMenuItem("folderPaneContext-compact", (numSelected <=1) && canCompact);
  var folder = GetMsgFolderFromResource(folderResource);
  EnableMenuItem("folderPaneContext-compact", folder.isCommandEnabled("cmd_compactFolder"));

  if(canCompact)
  {
    SetMenuItemLabel("folderPaneContext-compact", gMessengerBundle.getString("compactFolder"));
  }
}

function SetupNewMenuItem(folderResource, numSelected, isServer, serverType, specialFolder)
{
  var folderOutliner = GetFolderOutliner();
  var canCreateNew = GetFolderAttribute(folderOutliner, folderResource, "CanCreateSubfolders") == "true";
  var isInbox = specialFolder == "Inbox";

  var showNew = ((numSelected <=1) && (serverType != 'nntp') && canCreateNew) || isInbox;
  ShowMenuItem("folderPaneContext-new", showNew);
  EnableMenuItem("folderPaneContext-new", true);
  if(showNew)
  {
    if(isServer || isInbox)
      SetMenuItemLabel("folderPaneContext-new", gMessengerBundle.getString("newFolder"));
    else
      SetMenuItemLabel("folderPaneContext-new", gMessengerBundle.getString("newSubfolder"));
  }
}

function ShowMenuItem(id, showItem)
{
  var item = document.getElementById(id);
  if(item)
  {
    var showing = (item.getAttribute('hidden') !='true');
    if(showItem != showing)
      item.setAttribute('hidden', showItem ? '' : 'true');
  }
}

function EnableMenuItem(id, enableItem)
{
  var item = document.getElementById(id);
  if(item)
  {
    var enabled = (item.getAttribute('disabled') !='true');
    if(enableItem != enabled)
    {
      item.setAttribute('disabled', enableItem ? '' : 'true');
    }
  }
}

function SetMenuItemLabel(id, label)
{
  var item = document.getElementById(id);
  if(item)
    item.setAttribute('label', label);

}

function SetMenuItemAccessKey(id, accessKey)
{
  var item = document.getElementById(id);
  if(item)
    item.setAttribute('accesskey', accessKey);

}

function fillMessagePaneContextMenu()
{
  var message = GetLoadedMessage();
  var numSelected = (message) ? 1 : 0;

  var isNewsgroup = false;

  if (numSelected == 1) {
    isNewsgroup = IsNewsMessage(message);
    }

  var hideMailItems = AreBrowserItemsShowing();

  SetupEditAsNewMenuItem("messagePaneContext-editAsNew", numSelected, (numSelected == 0 || hideMailItems));
  SetupReplyToSenderMenuItem("messagePaneContext-replySender", numSelected, (numSelected == 0 || hideMailItems));
  SetupReplyToNewsgroupMenuItem("messagePaneContext-replyNewsgroup", numSelected, isNewsgroup, (numSelected == 0 || hideMailItems));
  SetupReplyAllMenuItem("messagePaneContext-replyAll" , numSelected, (numSelected == 0 || hideMailItems));
  SetupForwardMenuItem("messagePaneContext-forward", numSelected, (numSelected == 0 || hideMailItems));
  SetupCopyMessageUrlMenuItem("messagePaneContext-copyMessageUrl", numSelected, isNewsgroup, (numSelected == 0 || hideMailItems)); 
  SetupCopyMenuItem("messagePaneContext-copyMenu", numSelected, (numSelected == 0 || hideMailItems));
  SetupMoveMenuItem("messagePaneContext-moveMenu", numSelected, isNewsgroup, (numSelected == 0 || hideMailItems));
  SetupSaveAsMenuItem("messagePaneContext-saveAs", numSelected, (numSelected == 0 || hideMailItems));
  SetupPrintMenuItem("messagePaneContext-print", numSelected, (numSelected == 0 || hideMailItems));
  SetupDeleteMenuItem("messagePaneContext-delete", numSelected, isNewsgroup, (numSelected == 0 || hideMailItems));
  SetupAddSenderToABMenuItem("messagePaneContext-addSenderToAddressBook", numSelected, (numSelected == 0 || hideMailItems));
  SetupAddAllToABMenuItem("messagePaneContext-addAllToAddressBook", numSelected, (numSelected == 0 || hideMailItems));

  //Figure out separators
  ShowMenuItem("messagePaneContext-sep-open", ShowMessagePaneOpenSeparator());
  ShowMenuItem("messagePaneContext-sep-reply", ShowMessagePaneReplySeparator());
  ShowMenuItem("messagePaneContext-sep-edit", ShowMessagePaneEditSeparator());
  ShowMenuItem("messagePaneContext-sep-link", ShowMessagePaneLinkSeparator());
  ShowMenuItem("messagePaneContext-sep-image", ShowMessagePaneImageSeparator());
  ShowMenuItem("messagePaneContext-sep-copy", ShowMessagePaneCopySeparator());
}

function AreBrowserItemsShowing()
{
  return(IsMenuItemShowing("context-openlink") ||
    IsMenuItemShowing("context-editlink") ||
    IsMenuItemShowing("context-viewimage") ||
    IsMenuItemShowing("context-copylink") ||
    IsMenuItemShowing("context-copyimage") ||
    IsMenuItemShowing("context-savelink") ||
    IsMenuItemShowing("context-saveimage") ||
    IsMenuItemShowing("context-bookmarklink"));
}

function ShowMessagePaneOpenSeparator()
{
  return(IsMenuItemShowing("context-selectall") ||
    IsMenuItemShowing("context-copy"));
}

function ShowMessagePaneReplySeparator()
{
  return (IsMenuItemShowing("messagePaneContext-replySender") ||
    IsMenuItemShowing("messagePaneContext-replyNewsgroup") ||
    IsMenuItemShowing("messagePaneContext-replyAll") ||
    IsMenuItemShowing("messagePaneContext-forward") ||
    IsMenuItemShowing("messagePaneContext-editAsNew"));
}

function ShowMessagePaneEditSeparator()
{
  return (IsMenuItemShowing("messagePaneContext-moveMenu") ||
    IsMenuItemShowing("messagePaneContext-copyMenu") ||
    IsMenuItemShowing("messagePaneContext-copyMessageUrl") ||
    IsMenuItemShowing("messagePaneContext-saveAs") ||
    IsMenuItemShowing("messagePaneContext-print") ||
    IsMenuItemShowing("messagePaneContext-delete"));
}

function ShowMessagePaneLinkSeparator()
{
  return (IsMenuItemShowing("context-openlink") ||
    IsMenuItemShowing("context-editlink"));
}

function ShowMessagePaneImageSeparator()
{
  return (IsMenuItemShowing("context-viewimage"));
}

function ShowMessagePaneCopySeparator()
{
  return (IsMenuItemShowing("context-copylink") ||
    IsMenuItemShowing("context-copyimage"));
}

function IsMenuItemShowing(menuID)
{

  var item = document.getElementById(menuID);
  if(item)
  {
    return(item.getAttribute('hidden') !='true');
  }
  return false;
}

function CopyMessageUrl()
{
  try {
    var hdr = gDBView.hdrForFirstSelectedMessage;
    var server = hdr.folder.server;

    var url;
    if (server.isSecure) {
      url = "snews://";
    }
    else {
      url = "news://"
    }
    url += server.hostName;
    url += ":";
    url += server.port;
    url += "/";
    url += hdr.messageId;

    var contractid = "@mozilla.org/widget/clipboardhelper;1";
    var iid = Components.interfaces.nsIClipboardHelper;
    var clipboard = Components.classes[contractid].getService(iid);
    clipboard.copyString(url);
  }
  catch (ex) {
    dump("ex="+ex+"\n");
  }
}
