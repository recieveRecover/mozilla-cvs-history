/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 */

var stringBundle;

function onInit() {
    stringBundle = srGetStrBundle("chrome://messenger/locale/messenger.properties");
    initServerType();
}

function onPreInit(account, accountValues)
{
    var type = parent.getAccountValue(account, accountValues, "server", "type");
    
    hideShowControls(type);

    enableBiffUI();
}


function initServerType() {
  var serverType = document.getElementById("server.type").getAttribute("value");
  
  var verboseName;

  var propertyName = "serverType-" + serverType;

  verboseName = stringBundle.GetStringFromName(propertyName);

  setDivText("servertype.verbose", verboseName);
}

function hideShowControls(serverType)
{
    var controls = document.getElementsByAttribute("hidable", "true");
    var len = controls.length;
    for (var i=0; i<len; i++) {
        var control = controls[i];

        var hideFor = control.getAttribute("hidefor");
        if (!hideFor || hideFor == "") {
            dump("this should not happen, things that are hidable should have hidefor set\n");
            continue;
        }

        var box = getEnclosingContainer(control);

        if (!box) {
            dump("this should not happen, things that are hidable should be in a box\n");
            continue;
        }

        // hide unsupported server type
        // adding support for hiding multiple server types using hideFor="server1,server2"
        var hideForBool = false;
        var hideForTokens = hideFor.split(",");
        for (var j = 0; j < hideForTokens.length; j++) {
            if (hideForTokens[j] == serverType) {
                hideForBool = true;
                break;
            }
        }

        if (hideForBool) {
            box.setAttribute("hidden", "true");
        }
        else {
            box.removeAttribute("hidden");
        }
    }
}


function setDivText(divname, value) {
    var div = document.getElementById(divname);
    if (!div) return;
    div.setAttribute("value", value);
}


function openImapAdvanced()
{
    var imapServer = getImapServer();
    dump("Opening dialog..\n");
    window.openDialog("chrome://messenger/content/am-imap-advanced.xul",
                      "_blank",
                      "chrome,modal,titlebar", imapServer);

    saveServerLocally(imapServer);
}

function getImapServer() {
    var imapServer = new Array;

    imapServer.dualUseFolders = document.getElementById("imap.dualUseFolders").checked

    imapServer.usingSubscription = document.getElementById("imap.usingSubscription").checked;

    // string prefs
    imapServer.personalNamespace = document.getElementById("imap.personalNamespace").getAttribute("value");
    imapServer.publicNamespace = document.getElementById("imap.publicNamespace").getAttribute("value");
    imapServer.serverDirectory = document.getElementById("imap.serverDirectory").getAttribute("value");
    imapServer.otherUsersNamespace = document.getElementById("imap.otherUsersNamespace").getAttribute("value");

    imapServer.overrideNamespaces = document.getElementById("imap.overrideNamespaces").checked;
    return imapServer;
}

function saveServerLocally(imapServer)
{
    document.getElementById("imap.dualUseFolders").checked = imapServer.dualUseFolders;
    document.getElementById("imap.usingSubscription").checked = imapServer.usingSubscription;

    // string prefs
    document.getElementById("imap.personalNamespace").setAttribute("value", imapServer.personalNamespace);
    document.getElementById("imap.publicNamespace").setAttribute("value", imapServer.publicNamespace);
    document.getElementById("imap.serverDirectory").setAttribute("value", imapServer.serverDirectory);
    document.getElementById("imap.otherUsersNamespace").setAttribute("value", imapServer.otherUsersNamespace);

    document.getElementById("imap.overrideNamespaces").checked = imapServer.overrideNamespaces;

}

function getEnclosingContainer(startNode) {

    var parent = startNode;
    var box;
    
    while (parent && parent != document) {

        var isContainer =
            (parent.getAttribute("iscontrolcontainer") == "true");
          
        // remember the FIRST container we encounter, or the first
        // controlcontainer
        if (!box || isContainer)
            box=parent;
        
        // break out with a controlcontainer
        if (isContainer)
            break;
        parent = parent.parentNode;
    }
    
    return box;
}

function secureSelect() {
    var serverType   = document.getElementById("server.type").getAttribute("value");
    var protocolInfo = Components.classes["@mozilla.org/messenger/protocol/info;1?type=" + serverType].getService(Components.interfaces.nsIMsgProtocolInfo);

    // If the secure option is checked, protocolInfo returns a secure port value
	// for the corresponding protocol. Otherwise, a default value is returned.
    if (document.getElementById("server.isSecure").checked)
        document.getElementById("server.port").value = protocolInfo.getDefaultServerPort(true);
    else
        document.getElementById("server.port").value = protocolInfo.getDefaultServerPort(false);
}

function enableBiffUI()
{ 
    var parentCheckBox = document.getElementById('server.doBiff');
    var checkBox = document.getElementById('server.downloadOnBiff');
    var textField = document.getElementById('server.biffMinutes');

    if (parentCheckBox.getAttribute("checked")) {
      checkBox.setAttribute("disabled", "true");
      textField.setAttribute("disabled", "true");
    }
    else {
      checkBox.setAttribute("disabled", "false");
      textField.setAttribute("disabled", "false");
    } 
}
