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
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-2001 Netscape Communications Corporation. All
 * Rights Reserved.
 * 
 * Contributors:
 *   Scott MacGreogr <mscott@netscape.com>
 */

var gSignedUIVisible = false;
var gEncryptionUIVisible = false;

var gSignedUINode = null;
var gEncryptedUINode = null;
var gSMIMEContainer = null;

var smimeHeaderSink = 
{ 
  signedStatus: function(aValidSignature)
  {
    if (aValidSignature)
    {
      gSignedUINode.removeAttribute('collapsed');
      gSMIMEContainer.removeAttribute('collapsed');
    }
    else
    {
      // show a broken signature icon....
    }

    gSignedUIVisible = true;
  },

  encryptionStatus: function(aValidEncryption)
  {
    if (aValidEncryption)
    {
      gEncryptedUINode.removeAttribute('collapsed');
      gSMIMEContainer.removeAttribute('collapsed');
    }
    else
    {
      // show a broken encryption icon....
    }

    gEncryptionUIVisible = true;
  },
  QueryInterface : function(iid)
  {
    if (iid.equals(Components.interfaces.nsIMsgSMIMEHeaderSink) || iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
};

function onSMIMEStartHeaders()
{
  gSMIMEContainer.setAttribute('collapsed', true);

  if (gEncryptionUIVisible)
  {
    gEncryptedUINode.setAttribute('collapsed', true);
    gEncryptionUIVisible = false;
  }

  if (gSignedUIVisible)
  {
    gSignedUINode.setAttribute('collapsed', true);
    gSignedUIVisible = false;
  }
}

function onSMIMEEndHeaders()
{}

function msgHdrViewSMIMEOnLoad(event)
{
  // we want to register our security header sink as an opaque nsISupports
  // on the msgHdrSink used by mail.....
  msgWindow.msgHeaderSink.securityInfo = smimeHeaderSink;

  gSignedUINode = document.getElementById('signedText');
  gEncryptedUINode = document.getElementById('encryptedText');
  gSMIMEContainer = document.getElementById('expandedAttachmentBox');

  // add ourself to the list of message display listeners so we get notified when we are about to display a
  // message.
  var listener = {};
  listener.onStartHeaders = onSMIMEStartHeaders;
  listener.onEndHeaders = onSMIMEEndHeaders;
  gMessageListeners.push(listener);
}

addEventListener('messagepane-loaded', msgHdrViewSMIMEOnLoad, true);

