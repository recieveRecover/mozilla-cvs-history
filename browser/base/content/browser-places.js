/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Places Browser Integration
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Ben Goodger <beng@google.com>
 *   Annie Sullivan <annie.sullivan@gmail.com>
 *   Joe Hughes <joe@retrovirus.com>
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

var PlacesCommandHook = {
  /**
   * Adds a bookmark to the page targeted by a link.
   * @param   url
   *          The address of the link target
   * @param   title
   *          The link text
   */
  bookmarkLink: function PCH_bookmarkLink(url, title) {
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var linkURI = ios.newURI(url, null, null);

    PlacesUtils.showAddBookmarkUI(linkURI, title);
  },

  /**
   * Adds a bookmark to the page loaded in the given browser 
   * @param aBrowser
   *        a <browser> element
   */
  bookmarkPage: function PCH_bookmarkCurrentPage(aBrowser) {
    PlacesUtils.showAddBookmarkUI(aBrowser.currentURI);
  },

  /**
   * Adds a bookmark to the page loaded in the current tab. 
   */
  bookmarkCurrentPage: function PCH_bookmarkCurrentPage() {
    this.bookmarkPage(getBrowser().selectedBrowser);
  },


  /**
   * This function returns a list of nsIURI objects characterizing the
   * tabs currently open in the browser.  The URIs will appear in the
   * list in the order in which their corresponding tabs appeared.  However,
   * only the first instance of each URI will be returned.
   *
   * @returns a list of nsIURI objects representing unique locations open
   */
  _getUniqueTabInfo: function BATC__getUniqueTabInfo() {
    var tabList = [];
    var seenURIs = [];

    var browsers = getBrowser().browsers;
    for (var i = 0; i < browsers.length; ++i) {
      var webNav = browsers[i].webNavigation;
      var uri = webNav.currentURI;

      // skip redundant entries
      if (uri.spec in seenURIs)
        continue;

      // add to the set of seen URIs
      seenURIs[uri.spec] = true;
      tabList.push(uri);
    }
    return tabList;
  },

  /**
   * Adds a folder with bookmarks to all of the currently open tabs in this 
   * window.
   */
  bookmarkCurrentPages: function PCH_bookmarkCurrentPages() {
    var tabURIs = this._getUniqueTabInfo();
    PlacesUtils.showAddMultiBookmarkUI(tabURIs);
  },

  /**
   * Get the description associated with a document, as specified in a <META> 
   * element.
   * @param   doc
   *          A DOM Document to get a description for
   * @returns A description string if a META element was discovered with a 
   *          "description" or "httpequiv" attribute, empty string otherwise.
   */
  _getDescriptionFromDocument: function PCH_getDescriptionFromDocument(doc) {
    var metaElements = doc.getElementsByTagName("META");
    for (var i = 0; i < metaElements.length; ++i) {
      if (metaElements[i].localName.toLowerCase() == "description" || 
          metaElements[i].httpEquiv.toLowerCase() == "description") {
        return metaElements[i].content;
        break;
      }
    }
    return "";
  },
  
  /**
   * Adds a Live Bookmark to a feed associated with the current page. 
   * @param     url
   *            The nsIURI of the page the feed was attached to
   * @title     title
   *            The title of the feed. Optional.
   * @subtitle  subtitle
   *            A short description of the feed. Optional. 
   *            Not yet used. TODO: implement description annotation
   */
  addLiveBookmark: function PCH_addLiveBookmark(url, feedTitle, feedSubtitle) {
    var ios = 
        Cc["@mozilla.org/network/io-service;1"].
        getService(Ci.nsIIOService);
    var feedURI = ios.newURI(url, null, null);
    
    var browser = gBrowser.selectedBrowser;
   
    var title = (arguments.length > 1) ? feedTitle :
                                         browser.contentDocument.title;
 
    // TODO: implement description annotation
#if 0
    var description;
    if (arguments.length > 2)
      description = feedSubtitle;
    else
      description = BookmarksUtils.getDescriptionFromDocument(doc);
#endif

    // TODO: add dialog for filing/confirmation
    var bms = PlacesUtils.bookmarks;
    var livemarks = PlacesUtils.livemarks;
    livemarks.createLivemark(bms.toolbarRoot, title, browser.currentURI, 
                             feedURI, -1);
  },

  /**
   * Opens the Places Organizer. 
   * @param   place
   *          The place to select in the organizer window (a place: URI) 
   */
  showPlacesOrganizer: function PCH_showPlacesOrganizer(place) {
    var wm = 
        Cc["@mozilla.org/appshell/window-mediator;1"].
        getService(Ci.nsIWindowMediator);
    var organizer = wm.getMostRecentWindow("Places:Organizer");
    if (!organizer) {
      // No currently open places window, so open one with the specified mode.
      openDialog("chrome://browser/content/places/places.xul", 
                 "", "chrome,toolbar=yes,dialog=no,resizable", place);
    }
    else {
      // Set the mode on an existing places window. 
      organizer.selectPlaceURI(place);
      organizer.focus();
    }
  },

  /**
   * Update the state of the tagging icon, depending on whether or not the 
   * current page is bookmarked. 
   */
  updateTagButton: function PCH_updateTagButton() {
    var bookmarkButton = document.getElementById("places-bookmark");
    if (!bookmarkButton) 
      return;

    var currentLocation = getBrowser().selectedBrowser.webNavigation.currentURI;
    if (PlacesUtils.bookmarks.isBookmarked(currentLocation)) {
      bookmarkButton.label = PlacesUtils.getString("locationStatusBookmarked");
      bookmarkButton.setAttribute("bookmarked", "true");
    } else {
      bookmarkButton.label = PlacesUtils.getString("locationStatusNotBookmarked");
      bookmarkButton.removeAttribute("bookmarked");
    }
  },

  /**
   * This method should be called when the bookmark button is clicked.
   */
  onBookmarkButtonClick: function PCH_onBookmarkButtonClick() {
    var currentURI = getBrowser().selectedBrowser.webNavigation.currentURI;
    PlacesUtils.showAddBookmarkUI(currentURI);
  }
};

// Functions for the history menu.
var HistoryMenu = {

  /**
   * Updates the history menu with the session history of the current tab.
   * This function is called every time the history menu is shown.
   * @param menu 
   *        XULNode for the history menu
   */
  update: function PHM_update(menu) {
    FillHistoryMenu(menu, "history", document.getElementById("endTabHistorySeparator"));
  },

  /**
   * Shows the places search page.
   * (Will be fully implemented when there is a places search page.)
   */
  showPlacesSearch: function PHM_showPlacesSearch() {
    // XXX The places view needs to be updated before this
    // does something different than show history.
    PlacesCommandHook.showPlacesOrganizer(ORGANIZER_ROOT_HISTORY);
  }
};

/**
 * Functions for handling events in the Bookmarks Toolbar and menu.
 */
var BookmarksEventHandler = {  
  /**
   * Handler for click event for an item in the bookmarks toolbar or menu.
   * Menus and submenus from the folder buttons bubble up to this handler.
   * Only handle middle-click; left-click is handled in the onCommand function.
   * When items are middle-clicked, open them in tabs.
   * If the click came through a menu, close the menu.
   * @param aEvent
   *        DOMEvent for the click
   */
  onClick: function BT_onClick(aEvent) {
    // Only handle middle-clicks.
    if (aEvent.button != 1)
      return;

    var target = aEvent.originalTarget;
    var view = PlacesUtils.getViewForNode(target);
    if (PlacesUtils.nodeIsFolder(view.selectedNode)) {
      // Don't open the root folder in tabs when the empty area on the toolbar
      // is middle-clicked or when a non-bookmark item in a bookmarks menupopup
      // middle-clicked.
      if (!view.controller.rootNodeIsSelected())
        view.controller.openLinksInTabs();
    }
    else
      this.onCommand(aEvent);

    // If this event bubbled up from a menu or menuitem,
    // close the menus.
    if (target.localName == "menu" ||
        target.localName == "menuitem") {
      var node = target.parentNode;
      while (node && 
             (node.localName == "menu" || 
              node.localName == "menupopup")) {
        if (node.localName == "menupopup")
          node.hidePopup();
        
        node = node.parentNode;
      }
    }
    // The event target we get if someone middle clicks on a bookmark in the
    // bookmarks toolbar overflow menu is different from if they click on a
    // bookmark in a folder or in the bookmarks menu; handle this case
    // separately.
    var bookmarksBar = document.getElementById("bookmarksBarContent");
    if (bookmarksBar._chevron.getAttribute("open") == "true")
      bookmarksBar._chevron.firstChild.hidePopupAndChildPopups();
  },
  
  /**
   * Handler for command event for an item in the bookmarks toolbar.
   * Menus and submenus from the folder buttons bubble up to this handler.
   * Opens the item.
   * @param aEvent 
   *        DOMEvent for the command
   */
  onCommand: function BM_onCommand(aEvent) {
    // If this is the special "Open All in Tabs" menuitem,
    // load all the menuitems in tabs.

    var target = aEvent.originalTarget;
    if (target.hasAttribute("openInTabs"))
      PlacesUtils.getViewForNode(target).controller.openLinksInTabs();
    else if (target.hasAttribute("siteURI"))
      openUILink(target.getAttribute("siteURI"), aEvent);
    // If this is a normal bookmark, just load the bookmark's URI.
    else
      PlacesUtils.getViewForNode(target)
                 .controller
                 .openSelectedNodeWithEvent(aEvent);
  },

  /**
   * Handler for popupshowing event for an item in bookmarks toolbar or menu.
   * If the item isn't the main bookmarks menu, add an "Open All in Tabs"
   * menuitem to the bottom of the popup.
   * @param event 
   *        DOMEvent for popupshowing
   */
  onPopupShowing: function BM_onPopupShowing(event) {
    var target = event.target;

    if (target.localName == "menupopup" && target.id != "bookmarksMenuPopup") {
      // Show "Open All in Tabs" menuitem if there are at least
      // two menuitems with places result nodes, and "Open (Feed Name)"
      // if it's a livemark with a siteURI.
      var numNodes = 0;
      var hasMultipleEntries = false;
      var hasFeedHomePage = false;
      var currentChild = target.firstChild;
      while (currentChild && numNodes < 2) {
        if (currentChild.node && currentChild.localName == "menuitem")
          numNodes++;
        currentChild = currentChild.nextSibling;
      }
      if (numNodes > 1)
        hasMultipleEntries = true;
      var button = target.parentNode;
      if (button.getAttribute("livemark") == "true" &&
          button.hasAttribute("siteURI"))
        hasFeedHomePage = true;

      if (hasMultipleEntries || hasFeedHomePage) {
        var separator = document.createElement("menuseparator");
        target.appendChild(separator);

        if (hasFeedHomePage) {
          var openHomePage = document.createElement("menuitem");
          openHomePage.setAttribute(
            "siteURI", button.getAttribute("siteURI"));
          openHomePage.setAttribute(
            "label",
            PlacesUtils.getFormattedString("menuOpenLivemarkOrigin.label",
                                           [button.getAttribute("label")]));
          target.appendChild(openHomePage);
        }

        if (hasMultipleEntries) {
          var openInTabs = document.createElement("menuitem");
          openInTabs.setAttribute("openInTabs", "true");
          openInTabs.setAttribute("label",
                     gNavigatorBundle.getString("menuOpenAllInTabs.label"));
          target.appendChild(openInTabs);
        }
      }
    }
  },

  fillInBTTooltip: function(aTipElement) {
    // Fx2XP: Don't show tooltips for bookmarks under sub-folders
    if (aTipElement.localName != "toolbarbutton")
      return false;

    var url = aTipElement.getAttribute("url");
    if (!url) 
      return false;

    var tooltipUrl = document.getElementById("btUrlText");
    tooltipUrl.value = url;

    var title = aTipElement.label;
    var tooltipTitle = document.getElementById("btTitleText");
    if (title && title != url) {
      tooltipTitle.hidden = false;
      tooltipTitle.value = title;
    }
    else
      tooltipTitle.hidden = true;

    // show tooltip
    return true;
  }
};

/**
 * Drag and Drop handling specifically for the Bookmarks Menu item in the
 * top level menu bar
 */
var BookmarksMenuDropHandler = {
  /**
   * Need to tell the session to update the state of the cursor as we drag
   * over the Bookmarks Menu to show the "can drop" state vs. the "no drop"
   * state.
   */
  onDragOver: function BMDH_onDragOver(event, flavor, session) {
    session.canDrop = this.canDrop(event, session);
  },

  /**
   * Advertises the set of data types that can be dropped on the Bookmarks
   * Menu
   * @returns a FlavourSet object per nsDragAndDrop parlance.
   */
  getSupportedFlavours: function BMDH_getSupportedFlavours() {
    var flavorSet = new FlavourSet();
    var view = document.getElementById("bookmarksMenuPopup");
    for (var i = 0; i < view.peerDropTypes.length; ++i)
      flavorSet.appendFlavour(view.peerDropTypes[i]);
    return flavorSet;
  }, 

  /**
   * Determine whether or not the user can drop on the Bookmarks Menu.
   * @param   event
   *          A dragover event
   * @param   session
   *          The active DragSession
   * @returns true if the user can drop onto the Bookmarks Menu item, false 
   *          otherwise.
   */
  canDrop: function BMDH_canDrop(event, session) {
    var view = document.getElementById("bookmarksMenuPopup");
    return PlacesControllerDragHelper.canDrop(view, -1);
  },
  
  /**
   * Called when the user drops onto the top level Bookmarks Menu item.
   * @param   event
   *          A drop event
   * @param   data
   *          Data that was dropped
   * @param   session
   *          The active DragSession
   */
  onDrop: function BMDH_onDrop(event, data, session) {
    var view = document.getElementById("bookmarksMenuPopup");

    // The insertion point for a menupopup view should be -1 during a drag
    // & drop operation.
    NS_ASSERT(view.insertionPoint.index == -1, "Insertion point for an menupopup view during a drag must be -1!");
    PlacesControllerDragHelper.onDrop(null, view, view.insertionPoint, 1);
    view._rebuild();
  }
};

/**
 * Handles special drag and drop functionality for menus on the Bookmarks 
 * Toolbar and Bookmarks Menu.
 */
var PlacesMenuDNDController = {
  
  /**
   * Attach a special context menu hiding listener that ensures that popups 
   * are properly closed after a context menu is hidden. See bug 332845 for 
   * why we have to do this.
   */
  init: function PMDC_init() {
    var placesContext = document.getElementById("placesContext");
    var self = this;
    placesContext.addEventListener("popuphidden", function () { self._closePopups() }, false);
  },

  _springLoadDelay: 350, // milliseconds

  /**
   * All Drag Timers set for the Places UI
   */
  _timers: { },
  
  /**
   * Called when the user drags over the Bookmarks top level <menu> element.
   * @param   event
   *          The DragEnter event that spawned the opening. 
   */
  onBookmarksMenuDragEnter: function PMDC_onDragEnter(event) {
    if ("loadTime" in this._timers) 
      return;
    
    this._setDragTimer("loadTime", this._openBookmarksMenu, 
                       this._springLoadDelay, [event]);
  },
  
  /**
   * When the user drags out of the Bookmarks Menu or Toolbar, set a timer to 
   * manually close the popup chain that was dragged out of. We need to do this
   * since the global popup dismissal listener does not handle multiple extant
   * popup chains well. See bug 332845 for more information.
   */
  onDragExit: function PMDC_onDragExit(event) {
    // Ensure that we don't set multiple timers if there's one already set.
    if ("closeTime" in this._timers)
      return;
      
    this._setDragTimer("closeTime", this._closePopups, 
                       this._springLoadDelay, [event.target]);
  },
  
  /**
   * Creates a timer that will fire during a drag and drop operation.
   * @param   id
   *          The identifier of the timer being set
   * @param   callback
   *          The function to call when the timer "fires"
   * @param   delay
   *          The time to wait before calling the callback function
   * @param   args
   *          An array of arguments to pass to the callback function
   */
  _setDragTimer: function PMDC__setDragTimer(id, callback, delay, args) {
    if (!this._dragSupported)
      return;

    // Cancel this timer if it's already running.
    if (id in this._timers)
      this._timers[id].cancel();
      
    /**
     * An object implementing nsITimerCallback that calls a user-supplied
     * method with the specified args in the context of the supplied object.
     */
    function Callback(object, method, args) {
      this._method = method;
      this._args = args;
      this._object = object;
    }
    Callback.prototype = {
      notify: function C_notify(timer) {
        this._method.apply(this._object, this._args);
      }
    };
    
    var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(new Callback(this, callback, args), delay, 
                           timer.TYPE_ONE_SHOT);
    this._timers[id] = timer;
  },
  
  /**
   * Determines if a XUL element represents a container in the Bookmarks system
   * @returns true if the element is a container element (menu or 
   *`         menu-toolbarbutton), false otherwise.
   */
  _isContainer: function PMDC__isContainer(node) {
    return node.localName == "menu" || 
           node.localName == "toolbarbutton" && node.getAttribute("type") == "menu";
  },
  
  /**
   * Close all pieces of toplevel menu UI in the browser window. Called in 
   * circumstances where there may be multiple chains of menupopups, e.g. 
   * during drag and drop operations between menus, and in context menu-on-
   * menu situations.
   */
  _closePopups: function PMDC__closePopups(target) {
    if (PlacesControllerDragHelper.draggingOverChildNode(target))
      return;

    if ("closeTime" in this._timers)
      delete this._timers.closeTime;
    
    // Close the bookmarks menu
    var bookmarksMenu = document.getElementById("bookmarksMenu");
    bookmarksMenu.firstChild.hidePopupAndChildPopups();

    var bookmarksBar = document.getElementById("bookmarksBarContent");
    if (bookmarksBar) {
      // Close the overflow chevron menu and all its children
      bookmarksBar._chevron.firstChild.hidePopupAndChildPopups();

      // Close all popups on the bookmarks toolbar
      var toolbarItems = bookmarksBar.childNodes;
      for (var i = 0; i < toolbarItems.length; ++i) {
        var item = toolbarItems[i]
        if (this._isContainer(item))
          item.firstChild.hidePopupAndChildPopups();
      }
    }
  },
  
  /**
   * Opens the Bookmarks Menu when it is dragged over. (This is special-cased, 
   * since the toplevel Bookmarks <menu> is not a member of an existing places
   * container, as folders on the personal toolbar or submenus are. 
   * @param   event
   *          The DragEnter event that spawned the opening. 
   */
  _openBookmarksMenu: function PMDC__openBookmarksMenu(event) {
    if ("loadTime" in this._timers)
      delete this._timers.loadTime;
    if (event.target.id == "bookmarksMenu") {
      // If this is the bookmarks menu, tell its menupopup child to show.
      event.target.lastChild.showPopup(event.target.lastChild);
    }  
  },

  // Whether or not drag and drop to menus is supported on this platform
  // Dragging in menus is disabled on OS X due to various repainting issues.
#ifdef XP_MACOSX
  _dragSupported: false
#else
  _dragSupported: true
#endif
};
