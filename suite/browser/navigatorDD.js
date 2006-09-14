/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 *  - Kevin Puetz (puetzk@iastate.edu)
 *  - Ben Goodger <ben@netscape.com>
 */

var gRDFService = nsJSComponentManager.getService("component://netscape/rdf/rdf-service",
                                                  "nsIRDFService"); 

function RDF(aType) 
  {
    return "http://www.w3.org/1999/02/22-rdf-syntax-ns#" + aType;
  }
function NC_RDF(aType)
  {
    return "http://home.netscape.com/NC-rdf#" + aType;  
  }

var RDFUtils = {
  getResource: function(aString)
    {
      return gRDFService.GetResource(aString, true);
    },
  
  getTarget: function(aDS, aSourceID, aPropertyID)
    {
      dump("*** aSourceID = " + aSourceID + "\n");
      dump("*** aPropertyID = " + aPropertyID + "\n");
      var source = this.getResource(aSourceID);
      var property = this.getResource(aPropertyID);
      return aDS.GetTarget(source, property, true);
    },
  
  getValueFromResource: function(aResource)
    {
      aResource = aResource.QueryInterface(Components.interfaces.nsIRDFResource);
      return aResource ? target.Value : null;
    },
};

function isBookmark(aURI)
  {
    var rv = true;
    var typeValue = RDFUtils.getTarget(childWithDatabase.database, uri, RDF("type"));
    typeValue = RDFUtils.getValueFromResource(typeValue);
    if (typeValue != NC_RDF("BookmarkSeparator") && 
        typeValue != NC_RDF("Bookmark") &&
        typeValue != NC_RDF("Folder")) 
      rv = false;
    return rv;
  }

function isPToolbarDNDEnabled()
  {
    var prefs = nsJSComponentManager.getService("component://netscape/preferences",
                                                "nsIPref");
    var dragAndDropEnabled = false;                                                  
    try {
      dragAndDropEnabled = prefs.GetBoolPref("browser.enable.tb_dnd");
    }
    catch(e) {
    }
    
    return dragAndDropEnabled;
  }
  
var personalToolbarObserver = {
  onDragStart: function (aEvent)
    {
      // temporary
      if (!isPToolbarDNDEnabled())
        return false;
        
      var personalToolbar = document.getElementById("PersonalToolbar");
      if (aEvent.target == personalToolbar)
        return;
        
      var childWithDatabase = document.getElementById("innermostBox");
      var uri = aEvent.target.id;
      //if (!isBookmark(uri)) 
      //  return;
      
      var title = aEvent.target.value;
      var htmlString = "<A HREF='" + uri + "'>" + title + "</A>";

      var flavourList = { };
      flavourList["moz/toolbaritem"] = { width: 2, data: uri };
      flavourList["text/unicode"] = { width: 2, data: uri };
      return flavourList;
    },
  
  onDrop: function (aEvent, aData) 
    {
      // temporary
      if (!isPToolbarDNDEnabled())
        return false;
        
      var element = aData.data.data;
      var elementRes = RDFUtils.getResource(element);
      var personalToolbarRes = RDFUtils.getResource("NC:PersonalToolbarFolder");
      
      var childDB = document.getElementById("innermostBox").database;
      var rdfContainer = nsJSComponentManager.createInstance("component://netscape/rdf/container",
                                                             "nsIRDFContainer");
      rdfContainer.Init(childDB, personalToolbarRes);
      
      var newIndex = 1; // XXX need .offset* to figure out where to drop element
      var dropIndex = rdfContainer.IndexOf(elementRes);
      if (dropIndex > 0) 
        rdfContainer.RemoveElement(elementRes, true);
      else if (dropIndex == -1)
        {
          dump("*** element = " + element + "\n");
          // look up this URL's title in global history
          var potentialTitle = null;
          var historyDS = gRDFService.GetDataSource("rdf:history");
          var historyEntry = gRDFService.GetResource(element);
          var historyTitleProperty = gRDFService.GetResource(NC_RDF("URL"));
          var titleFromHistory = historyDS.GetTarget(historyEntry, historyTitleProperty, true);
          if (titleFromHistory) 
//            titleFromHistory = titleFromHistory.QueryInterface(Components.interfaces.nsIRDFLiteral);
          if (titleFromHistory)
            potentialTitle = titleFromHistory.Value;
          linkTitle = potentialTitle ? potentialTitle : element;
          childDB.Assert(gRDFService.GetResource(element, true), 
                         gRDFService.GetResource(NC_RDF("Name"), true),
                         gRDFService.GetLiteral(linkTitle),
                         true);
        }
      rdfContainer.InsertElementAt(elementRes, newIndex, true);
    },
  
  onDragOver: function (aEvent, aFlavour)
    {
      // temporary
      if (!isPToolbarDNDEnabled())
        return false;
        
      var toolbar = document.getElementById("PersonalToolbar");
      toolbar.setAttribute("dd-triggerrepaint", 0);
    },

  getSupportedFlavours: function ()
    {
      // temporary
      if (!isPToolbarDNDEnabled())
        return false;
        
      var flavourList = { };
      flavourList["moz/toolbaritem"] = { width: 2, iid: "nsISupportsWString" };
      flavourList["text/unicode"] = { width: 2, iid: "nsISupportsWString" };
      return flavourList;
    },
}; 

var contentAreaDNDObserver = {
  onDragStart: function (aEvent)
    {  
      var htmlstring = null;
      var textstring = null;
      var domselection = window._content.getSelection();
      if (domselection && !domselection.isCollapsed && 
          domselection.containsNode(aEvent.target,false))
        {
          // the window has a selection so we should grab that rather than looking for specific elements
          htmlstring = domselection.toString("text/html", 128+256, 0);
          textstring = domselection.toString("text/plain", 0, 0);
        }
      else 
        {
          switch (aEvent.target.localName) 
            {
              case 'AREA':
              case 'IMG':
                var imgsrc = aEvent.target.getAttribute("src");
                var baseurl = window._content.location.href;
                // need to do some stuff with the window._content.location here (path?) 
                // to get base URL for image.
                textstring = imgsrc;
                htmlstring = "<img src=\"" + textstring + "\">";
                break;
              case 'A':
                if (aEvent.target.href)
                  {
                    textstring = aEvent.target.getAttribute("href");
                    htmlstring = "<a href=\"" + textstring + "\">" + textstring + "</a>";
                  }
                else if (aEvent.target.name)
                  {
                    textstring = aEvent.target.getAttribute("name");
                    htmlstring = "<a name=\"" + textstring + "\">" + textstring + "</a>"
                  }
                break;
              default:
              case '#text':
              case 'LI':
              case 'OL':
              case 'DD':
                textstring = enclosingLink(aEvent.target);
                if (textstring != "")
                  htmlstring = "<a href=\"" + textstring + "\">" + textstring + "</a>";
                else
                  throw Components.results.NS_ERROR_FAILURE;
                break;
            }
        }
  
      var flavourList = { };
      flavourList["text/html"] = { width: 2, data: htmlstring };
      flavourList["text/unicode"] = { width: 2, data: textstring };
      return flavourList;
    },

  onDrop: function (aEvent, aData)
    {
      var aData = aData.length ? aData[0] : aData;
      var url = retrieveURLFromData(aData);
      if (url.length == 0)
        return;
      // valid urls don't contain spaces ' '; if we have a space it isn't a valid url so bail out
      var urlstr = url.toString();
      if ( urlstr.indexOf(" ", 0) != -1 )
        return;

      var urlBar = document.getElementById("urlbar");
      urlBar.value = url;
      BrowserLoadURL();
    },

  getSupportedFlavours: function ()
    {
      var flavourList = { };
      //flavourList["moz/toolbaritem"] = { width: 2 };
      flavourList["text/unicode"] = { width: 2, iid: "nsISupportsWString" };
      flavourList["application/x-moz-file"] = { width: 2, iid: "nsIFile" };
      return flavourList;
    },
};

//
// DragProxyIcon
//
// Called when the user is starting a drag from the proxy icon next to the URL bar. Basically
// just gets the url from the url bar and places the data (as plain text) in the drag service.
//
// This is by no means the final implementation, just another example of what you can do with
// JS. Much still left to do here.
// 

var proxyIconDNDObserver = {
  onDragStart: function ()
    {
      var urlBar = document.getElementById("urlbar");
      var flavourList = { };
      flavourList["text/unicode"] = { width: 2, data: urlBar.value };
//      flavourList["text/x-moz-url"] = { width: 2, data: urlBar.value + " " + window.title };
//*** hack until bug 41984 is fixed. uncomment the above line, it is the "correct" fix
      flavourList["text/x-moz-url"] = { width: 2, data: urlBar.value + " " + "( TEMP TITLE )" };
      var htmlString = "<a href=\"" + urlBar.value + "\">" + urlBar.value + "</a>";
      flavourList["text/html"] = { width: 2, data: htmlString };
      return flavourList;
    }
};

var homeButtonObserver = {
  onDrop: function (aEvent, aData)
    {
      var aData = aData.length ? aData[0] : aData;
      var url = retrieveURLFromData(aData);
      var showDialog = nsPreferences.getBoolPref("browser.homepage.enable_home_button_drop", false);
      var setHomepage;
      if (showDialog)
        {
          var commonDialogService = nsJSComponentManager.getService("component://netscape/appshell/commonDialogs",
                                                                    "nsICommonDialogs");
          var block = nsJSComponentManager.createInstanceByID("c01ad085-4915-11d3-b7a0-85cf-55c3523c",
                                                              "nsIDialogParamBlock");
          var checkValue = { value: true };
          var pressedVal = { };                            
          var promptTitle = bundle.GetStringFromName("droponhometitle");
          var promptMsg   = bundle.GetStringFromName("droponhomemsg");
          var checkMsg    = bundle.GetStringFromName("dontremindme");
          var okButton    = bundle.GetStringFromName("droponhomeokbutton");
          var iconURL     = "chrome://navigator/skin/home.gif"; // evil evil common dialog code! evil! 
/*
          block.SetInt(2, 2);
          block.SetString(0, bundle.GetStringFromName("droponhomemsg"));
          block.SetString(3, bundle.GetStringFromName("droponhometitle"));
          block.SetString(2, "chrome://navigator/skin/home.gif");
          block.SetString(1, bundle.GetStringFromName("dontremindme"));
          block.SetInt(1, 1); // checkbox is checked
          block.SetString(8, bundle.GetStringFromName("droponhomeokbutton"));
          
*/
          commonDialogService.UniversalDialog(window, null, promptTitle, promptMsg, checkMsg, 
                                              okButton, null, null, null, null, null, { }, { },
                                              iconURL, checkValue, 2, 0, null, pressedVal);
          nsPreferences.setBoolPref("browser.homepage.enable_home_button_drop", checkValue.value);

          setHomepage = pressedVal.value == 0 ? true : false;
        }
      else
        setHomepage = true;
      if (setHomepage) 
        {
          nsPreferences.setUnicharPref("browser.startup.homepage", url);                                           
          setTooltipText("homebutton", url);
        }
    },
    
  onDragOver: function (aEvent, aFlavour)
    {
      var homeButton = aEvent.target;
      // preliminary attribute name for now
      homeButton.setAttribute("home-dragover","true");
		  var statusTextFld = document.getElementById("statusbar-display");
      gStatus = gStatus ? gStatus : statusTextFld.value;
      statusTextFld.setAttribute("value", bundle.GetStringFromName("droponhomebutton"));
    },
    
  onDragExit: function ()
    {
      var homeButton = document.getElementById("homebutton");
      homeButton.removeAttribute("home-dragover");
      statusTextFld.setAttribute("value", gStatus);
      gStatus = null;
    },
        
  onDragStart: function ()
    {
      var homepage = nsPreferences.getLocalizedUnicharPref("browser.startup.homepage", "about:blank");
      var flavourList = { };
      flavourList["text/unicode"] = { width: 2, data: homepage };
      flavourList["text/x-moz-url"] = { width: 2, data: homepage };
      var htmlString = "<a href=\"" + homepage + "\">" + homepage + "</a>";
      flavourList["text/html"] = { width: 2, data: htmlString };
      return flavourList;
    },
  
  getSupportedFlavours: function ()
    {
      var flavourList = { };
      //flavourList["moz/toolbaritem"] = { width: 2 };
      flavourList["text/unicode"] = { width: 2, iid: "nsISupportsWString" };
      flavourList["application/x-moz-file"] = { width: 2, iid: "nsIFile" };
      return flavourList;
    },  
};

function retrieveURLFromData (aData)
  {
    switch (aData.flavour)
      {
        case "text/unicode":
          return aData.data.data; // XXX this is busted. 
          break;
        case "application/x-moz-file":
          var dataObj = aData.data.data.QueryInterface(Components.interfaces.nsIFile);
          if (dataObj)
            {
              var fileURL = nsJSComponentManager.createInstance("component://netscape/network/standard-url",
                                                                "nsIFileURL");
              fileURL.file = dataObj;
              return fileURL.spec;
            }
      }             
    return null;                                                         
  }
