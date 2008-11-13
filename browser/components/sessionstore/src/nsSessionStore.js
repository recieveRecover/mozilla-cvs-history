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
 * The Original Code is the nsSessionStore component.
 *
 * The Initial Developer of the Original Code is
 * Simon Bünzli <zeniko@gmail.com>
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Dietrich Ayala <dietrich@mozilla.com>
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

/**
 * Session Storage and Restoration
 * 
 * Overview
 * This service keeps track of a user's session, storing the various bits
 * required to return the browser to it's current state. The relevant data is 
 * stored in memory, and is periodically saved to disk in a file in the 
 * profile directory. The service is started at first window load, in
 * delayedStartup, and will restore the session from the data received from
 * the nsSessionStartup service.
 */

/* :::::::: Constants and Helpers ::::::::::::::: */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = -1;
const STATE_DISABLED = -2;

const STATE_STOPPED_STR = "stopped";
const STATE_RUNNING_STR = "running";

const PRIVACY_NONE = 0;
const PRIVACY_ENCRYPTED = 1;
const PRIVACY_FULL = 2;

const NOTIFY_WINDOWS_RESTORED = "sessionstore-windows-restored";

// global notifications observed
const OBSERVING = [
  "domwindowopened", "domwindowclosed",
  "quit-application-requested", "quit-application-granted",
  "quit-application", "browser:purge-session-history"
];

/*
XUL Window properties to (re)store
Restored in restoreDimensions()
*/
const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];

/* 
Hideable window features to (re)store
Restored in restoreWindowFeatures()
*/
const WINDOW_HIDEABLE_FEATURES = [
  "menubar", "toolbar", "locationbar", 
  "personalbar", "statusbar", "scrollbars"
];

/*
docShell capabilities to (re)store
Restored in restoreHistory()
eg: browser.docShell["allow" + aCapability] = false;
*/
const CAPABILITIES = [
  "Subframes", "Plugins", "Javascript", "MetaRedirects", "Images"
];

// module for JSON conversion (needed for the nsISessionStore API)
Cu.import("resource://gre/modules/JSON.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function debug(aMsg) {
  aMsg = ("SessionStore: " + aMsg).replace(/\S{80}/g, "$&\n");
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService)
                                     .logStringMessage(aMsg);
}

/* :::::::: The Service ::::::::::::::: */

function SessionStoreService() {
}

SessionStoreService.prototype = {
  classDescription: "Browser Session Store Service",
  contractID: "@mozilla.org/browser/sessionstore;1",
  classID: Components.ID("{5280606b-2510-4fe0-97ef-9b5a22eafe6b}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISessionStore,
                                         Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  // xul:tab attributes to (re)store (extensions might want to hook in here)
  xulAttributes: [],

  // set default load state
  _loadState: STATE_STOPPED,

  // minimal interval between two save operations (in milliseconds)
  _interval: 10000,

  // when crash recovery is disabled, session data is not written to disk
  _resume_from_crash: true,

  // During the initial restore tracks the number of windows yet to be restored
  _restoreCount: 0,

  // time in milliseconds (Date.now()) when the session was last written to file
  _lastSaveTime: 0, 

  // states for all currently opened windows
  _windows: {},

  // in case the last closed window ain't a navigator:browser one
  _lastWindowClosed: null,

  // not-"dirty" windows usually don't need to have their data updated
  _dirtyWindows: {},

  // flag all windows as dirty
  _dirty: false,

/* ........ Global Event Handlers .............. */

  /**
   * Initialize the component
   */
  init: function sss_init(aWindow) {
    if (this._loadState == STATE_DISABLED)
      return;

    if (!aWindow || this._loadState == STATE_RUNNING) {
      // make sure that all browser windows which try to initialize
      // SessionStore are really tracked by it
      if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
        this.onLoad(aWindow);
      return;
    }

    this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).getBranch("browser.");
    this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

    var observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);

    // if the service is disabled, do not init 
    if (!this._prefBranch.getBoolPref("sessionstore.enabled")) {
      // Notify observers that the sessionstore has done everything it is going to.
      observerService.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
      // Mark as disabled so we don't even try to initialise again.
      this._loadState = STATE_DISABLED;
      return;
    }

    OBSERVING.forEach(function(aTopic) {
      observerService.addObserver(this, aTopic, true);
    }, this);
    
    // get interval from prefs - used often, so caching/observing instead of fetching on-demand
    this._interval = this._prefBranch.getIntPref("sessionstore.interval");
    this._prefBranch.addObserver("sessionstore.interval", this, true);
    
    // get crash recovery state from prefs and allow for proper reaction to state changes
    this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    this._prefBranch.addObserver("sessionstore.resume_from_crash", this, true);
    
    // observe prefs changes so we can modify stored data to match
    this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);

    // get file references
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");
   
    // get string containing session state
    var iniString;
    try {
      var ss = Cc["@mozilla.org/browser/sessionstartup;1"].
               getService(Ci.nsISessionStartup);
      if (ss.doRestore())
        iniString = ss.state;
    }
    catch(ex) { dump(ex + "\n"); } // no state to restore, which is ok

    if (iniString) {
      try {
        // parse the session state into JS objects
        this._initialState = this._safeEval(iniString);
        // set bool detecting crash
        this._lastSessionCrashed =
          this._initialState.session && this._initialState.session.state &&
          this._initialState.session.state == STATE_RUNNING_STR;
        
        // restore the features of the first window from localstore.rdf
        WINDOW_ATTRIBUTES.forEach(function(aAttr) {
          delete this._initialState.windows[0][aAttr];
        }, this);
        delete this._initialState.windows[0].hidden;
      }
      catch (ex) { debug("The session file is invalid: " + ex); }
    }
    
    // if last session crashed, backup the session
    if (this._lastSessionCrashed) {
      try {
        this._writeFile(this._sessionFileBackup, iniString);
      }
      catch (ex) { } // nothing else we can do here
    }

    // remove the session data files if crash recovery is disabled
    if (!this._resume_from_crash)
      this._clearDisk();
    
    // As this is called at delayedStartup, restoration must be initiated here
    this.onLoad(aWindow);
  },

  /**
   * Called on application shutdown, after notifications:
   * quit-application-granted, quit-application
   */
  _uninit: function sss_uninit() {
    if (this._doResumeSession()) { // save all data for session resuming 
      this.saveState(true);
    }
    else { // discard all session related data 
      this._clearDisk();
    }
    // Make sure to break our cycle with the save timer
    if (this._saveTimer) {
      this._saveTimer.cancel();
      this._saveTimer = null;
    }
  },

  /**
   * Handle notifications
   */
  observe: function sss_observe(aSubject, aTopic, aData) {
    // for event listeners
    var _this = this;

    switch (aTopic) {
    case "domwindowopened": // catch new windows
      aSubject.addEventListener("load", function(aEvent) {
        aEvent.currentTarget.removeEventListener("load", arguments.callee, false);
        _this.onLoad(aEvent.currentTarget);
        }, false);
      break;
    case "domwindowclosed": // catch closed windows
      this.onClose(aSubject);
      break;
    case "quit-application-requested":
      // get a current snapshot of all windows
      this._forEachBrowserWindow(function(aWindow) {
        this._collectWindowData(aWindow);
      });
      this._dirtyWindows = [];
      this._dirty = false;
      break;
    case "quit-application-granted":
      // freeze the data at what we've got (ignoring closing windows)
      this._loadState = STATE_QUITTING;
      break;
    case "quit-application":
      if (aData == "restart")
        this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
      this._loadState = STATE_QUITTING; // just to be sure
      this._uninit();
      break;
    case "browser:purge-session-history": // catch sanitization 
      this._forEachBrowserWindow(function(aWindow) {
        Array.forEach(aWindow.getBrowser().browsers, function(aBrowser) {
          delete aBrowser.parentNode.__SS_data;
        });
      });
      this._lastWindowClosed = null;
      this._clearDisk();
      // also clear all data about closed tabs
      for (ix in this._windows) {
        this._windows[ix]._closedTabs = [];
      }
      // give the tabbrowsers a chance to clear their histories first
      var win = this._getMostRecentBrowserWindow();
      if (win)
        win.setTimeout(function() { _this.saveState(true); }, 0);
      else
        this.saveState(true);
      break;
    case "nsPref:changed": // catch pref changes
      switch (aData) {
      // if the user decreases the max number of closed tabs they want
      // preserved update our internal states to match that max
      case "sessionstore.max_tabs_undo":
        var ix;
        for (ix in this._windows) {
          this._windows[ix]._closedTabs.splice(this._prefBranch.getIntPref("sessionstore.max_tabs_undo"));
        }
        break;
      case "sessionstore.interval":
        this._interval = this._prefBranch.getIntPref("sessionstore.interval");
        // reset timer and save
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
        }
        this.saveStateDelayed(null, -1);
        break;
      case "sessionstore.resume_from_crash":
        this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
        // either create the file with crash recovery information or remove it
        // (when _loadState is not STATE_RUNNING, that file is used for session resuming instead)
        if (this._resume_from_crash)
          this.saveState(true);
        else if (this._loadState == STATE_RUNNING)
          this._clearDisk();
        break;
      }
      break;
    case "timer-callback": // timer call back for delayed saving
      this._saveTimer = null;
      this.saveState();
      break;
    }
  },

/* ........ Window Event Handlers .............. */

  /**
   * Implement nsIDOMEventListener for handling various window and tab events
   */
  handleEvent: function sss_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "load":
      case "pageshow":
        this.onTabLoad(aEvent.currentTarget.ownerDocument.defaultView, aEvent.currentTarget, aEvent);
        break;
      case "input":
      case "DOMAutoComplete":
        this.onTabInput(aEvent.currentTarget.ownerDocument.defaultView, aEvent.currentTarget, aEvent);
        break;
      case "TabOpen":
      case "TabClose":
        var panelID = aEvent.originalTarget.linkedPanel;
        var tabpanel = aEvent.originalTarget.ownerDocument.getElementById(panelID);
        if (aEvent.type == "TabOpen") {
          this.onTabAdd(aEvent.currentTarget.ownerDocument.defaultView, tabpanel);
        }
        else {
          this.onTabClose(aEvent.currentTarget.ownerDocument.defaultView, aEvent.originalTarget);
          this.onTabRemove(aEvent.currentTarget.ownerDocument.defaultView, tabpanel);
        }
        break;
      case "TabSelect":
        var tabpanels = aEvent.currentTarget.mPanelContainer;
        this.onTabSelect(aEvent.currentTarget.ownerDocument.defaultView, tabpanels);
        break;
    }
  },

  /**
   * If it's the first window load since app start...
   * - determine if we're reloading after a crash or a forced-restart
   * - restore window state
   * - restart downloads
   * Set up event listeners for this window's tabs
   * @param aWindow
   *        Window reference
   */
  onLoad: function sss_onLoad(aWindow) {
    // return if window has already been initialized
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    // ignore non-browser windows and windows opened while shutting down
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" ||
      this._loadState == STATE_QUITTING)
      return;

    // assign it a unique identifier (timestamp)
    aWindow.__SSi = "window" + Date.now();

    // and create its data object
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [] };
    
    // perform additional initialization when the first window is loading
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();
      
      // don't save during the first ten seconds
      // (until most of the pages have been restored)
      this.saveStateDelayed(aWindow, 10000);

      // restore a crashed session resp. resume the last session if requested
      if (this._initialState) {
        // make sure that the restored tabs are first in the window
        this._initialState._firstTabs = true;
        this._restoreCount = this._initialState.windows ? this._initialState.windows.length : 0;
        this.restoreWindow(aWindow, this._initialState, this._isCmdLineEmpty(aWindow));
        delete this._initialState;
      }
      else {
        // Nothing to restore, notify observers things are complete.
        var observerService = Cc["@mozilla.org/observer-service;1"].
                              getService(Ci.nsIObserverService);
        observerService.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
      }
    }
    
    var tabbrowser = aWindow.getBrowser();
    var tabpanels = tabbrowser.mPanelContainer;
    
    // add tab change listeners to all already existing tabs
    for (var i = 0; i < tabpanels.childNodes.length; i++) {
      this.onTabAdd(aWindow, tabpanels.childNodes[i], true);
    }
    // notification of tab add/remove/selection
    tabbrowser.addEventListener("TabOpen", this, true);
    tabbrowser.addEventListener("TabClose", this, true);
    tabbrowser.addEventListener("TabSelect", this, true);
  },

  /**
   * On window close...
   * - remove event listeners from tabs
   * - save all window data
   * @param aWindow
   *        Window reference
   */
  onClose: function sss_onClose(aWindow) {
    // ignore windows not tracked by SessionStore
    if (!aWindow.__SSi || !this._windows[aWindow.__SSi]) {
      return;
    }
    
    if (this.windowToFocus && this.windowToFocus == aWindow) {
      delete this.windowToFocus;
    }
    
    var tabbrowser = aWindow.getBrowser();
    var tabpanels = tabbrowser.mPanelContainer;

    tabbrowser.removeEventListener("TabOpen", this, true);
    tabbrowser.removeEventListener("TabClose", this, true);
    tabbrowser.removeEventListener("TabSelect", this, true);
    
    if (this._loadState == STATE_RUNNING) { // window not closed during a regular shut-down 
      // update all window data for a last time
      this._collectWindowData(aWindow);
      
      // preserve this window's data (in case it was the last navigator:browser)
      this._lastWindowClosed = this._windows[aWindow.__SSi];
      this._lastWindowClosed.title = aWindow.content.document.title;
      this._updateCookies([this._lastWindowClosed]);
      
      // clear this window from the list
      delete this._windows[aWindow.__SSi];
      
      // save the state without this window to disk
      this.saveStateDelayed();
    }
    
    for (var i = 0; i < tabpanels.childNodes.length; i++) {
      this.onTabRemove(aWindow, tabpanels.childNodes[i], true);
    }
    
    // cache the window state until the window is completely gone
    aWindow.__SS_dyingCache = this._windows[aWindow.__SSi] || this._lastWindowClosed;
    
    // reset the _tab property to avoid keeping the tab's XUL element alive
    // longer than we need it
    var tabCount = aWindow.__SS_dyingCache.tabs.length;
    for (var t = 0; t < tabCount; t++) {
      delete aWindow.__SS_dyingCache.tabs[t]._tab;
    }
    
    delete aWindow.__SSi;
  },

  /**
   * set up listeners for a new tab
   * @param aWindow
   *        Window reference
   * @param aPanel
   *        TabPanel reference
   * @param aNoNotification
   *        bool Do not save state if we're updating an existing tab
   */
  onTabAdd: function sss_onTabAdd(aWindow, aPanel, aNoNotification) {
    aPanel.addEventListener("load", this, true);
    aPanel.addEventListener("pageshow", this, true);
    aPanel.addEventListener("input", this, true);
    aPanel.addEventListener("DOMAutoComplete", this, true);
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  /**
   * remove listeners for a tab
   * @param aWindow
   *        Window reference
   * @param aPanel
   *        TabPanel reference
   * @param aNoNotification
   *        bool Do not save state if we're updating an existing tab
   */
  onTabRemove: function sss_onTabRemove(aWindow, aPanel, aNoNotification) {
    aPanel.removeEventListener("load", this, true);
    aPanel.removeEventListener("pageshow", this, true);
    aPanel.removeEventListener("input", this, true);
    aPanel.removeEventListener("DOMAutoComplete", this, true);
    
    delete aPanel.__SS_data;
    delete aPanel.__SS_text;
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  /**
   * When a tab closes, collect it's properties
   * @param aWindow
   *        Window reference
   * @param aTab
   *        TabPanel reference
   */
  onTabClose: function sss_onTabClose(aWindow, aTab) {
    // notify the tabbrowser that the tab state will be retrieved for the last time
    // (so that extension authors can easily set data on soon-to-be-closed tabs)
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabClosing", true, false);
    aTab.dispatchEvent(event);
    
    var maxTabsUndo = this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    // don't update our internal state if we don't have to
    if (maxTabsUndo == 0) {
      return;
    }
    
    // make sure that the tab related data is up-to-date
    var tabState = this._collectTabData(aTab);
    this._updateTextAndScrollDataForTab(aWindow, aTab.linkedBrowser, tabState);

    // reset the _tab property to avoid keeping the tab's XUL element alive
    // longer than we need it
    delete tabState._tab;
    
    // store closed-tab data for undo
    if (tabState.entries.length > 1 || tabState.entries[0].url != "about:blank") {
      this._windows[aWindow.__SSi]._closedTabs.unshift({
        state: tabState,
        title: aTab.getAttribute("label"),
        image: aTab.getAttribute("image"),
        pos: aTab._tPos
      });
      var length = this._windows[aWindow.__SSi]._closedTabs.length;
      if (length > maxTabsUndo)
        this._windows[aWindow.__SSi]._closedTabs.splice(maxTabsUndo, length - maxTabsUndo);
    }
  },

  /**
   * When a tab loads, save state.
   * @param aWindow
   *        Window reference
   * @param aPanel
   *        TabPanel reference
   * @param aEvent
   *        Event obj
   */
  onTabLoad: function sss_onTabLoad(aWindow, aPanel, aEvent) { 
    // react on "load" and solitary "pageshow" events (the first "pageshow"
    // following "load" is too late for deleting the data caches)
    if (aEvent.type != "load" && !aEvent.persisted) {
      return;
    }
    
    delete aPanel.__SS_data;
    delete aPanel.__SS_text;
    this.saveStateDelayed(aWindow);
    
    // attempt to update the current URL we send in a crash report
    this._updateCrashReportURL(aWindow);
  },

  /**
   * Called when a tabpanel sends the "input" notification 
   * stores textarea data
   * @param aWindow
   *        Window reference
   * @param aPanel
   *        TabPanel reference
   * @param aEvent
   *        Event obj
   */
  onTabInput: function sss_onTabInput(aWindow, aPanel, aEvent) {
    if (this._saveTextData(aPanel, aEvent.originalTarget)) {
      this.saveStateDelayed(aWindow, 3000);
    }
  },

  /**
   * When a tab is selected, save session data
   * @param aWindow
   *        Window reference
   * @param aPanels
   *        TabPanel reference
   */
  onTabSelect: function sss_onTabSelect(aWindow, aPanels) {
    if (this._loadState == STATE_RUNNING) {
      this._windows[aWindow.__SSi].selected = aPanels.selectedIndex;
      this.saveStateDelayed(aWindow);

      // attempt to update the current URL we send in a crash report
      this._updateCrashReportURL(aWindow);
    }
  },

/* ........ nsISessionStore API .............. */

  getBrowserState: function sss_getBrowserState() {
    return this._toJSONString(this._getCurrentState());
  },

  setBrowserState: function sss_setBrowserState(aState) {
    var window = this._getMostRecentBrowserWindow();
    if (!window) {
      this._openWindowWithState("(" + aState + ")");
      return;
    }

    // close all other browser windows
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow != window) {
        aWindow.close();
      }
    });

    // restore to the given state
    this.restoreWindow(window, "(" + aState + ")", true);
  },

  getWindowState: function sss_getWindowState(aWindow) {
    if (!aWindow.__SSi && aWindow.__SS_dyingCache)
      return this._toJSONString({ windows: [aWindow.__SS_dyingCache] });
    
    return this._toJSONString(this._getWindowState(aWindow));
  },

  setWindowState: function sss_setWindowState(aWindow, aState, aOverwrite) {
    this.restoreWindow(aWindow, "(" + aState + ")", aOverwrite);
  },

  getTabState: function sss_getTabState(aTab) {
    var tabState = this._collectTabData(aTab);
    
    var window = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(window, aTab.linkedBrowser, tabState);
    
    return this._toJSONString(tabState);
  },

  setTabState: function sss_setTabState(aTab, aState) {
    var tabState = this._safeEval("(" + aState + ")");
    if (!tabState.entries || !tabState.entries.length) {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
      return;
    }
    tabState._tab = aTab;
    
    var window = aTab.ownerDocument.defaultView;
    this.restoreHistoryPrecursor(window, [tabState], 0, 0, 0);
  },

  duplicateTab: function sss_duplicateTab(aWindow, aTab) {
    var tabState = this._collectTabData(aTab, true);
    var sourceWindow = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(sourceWindow, aTab.linkedBrowser, tabState, true);
    
    var newTab = aWindow.getBrowser().addTab();
    tabState._tab = newTab;
    this.restoreHistoryPrecursor(aWindow, [tabState], 0, 0, 0);
    
    return newTab;
  },

  getClosedTabCount: function sss_getClosedTabCount(aWindow) {
    if (!aWindow.__SSi && aWindow.__SS_dyingCache)
      return aWindow.__SS_dyingCache._closedTabs.length;
    if (!aWindow.__SSi)
      return 0; // not a browser window, or not otherwise tracked by SS.
    
    return this._windows[aWindow.__SSi]._closedTabs.length;
  },

  closedTabNameAt: function sss_closedTabNameAt(aWindow, aIx) {
    var tabs;
    
    if (aWindow.__SSi && aWindow.__SSi in this._windows)
      tabs = this._windows[aWindow.__SSi]._closedTabs;
    else if (aWindow.__SS_dyingCache)
      tabs = aWindow.__SS_dyingCache._closedTabs;
    else
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    
    return tabs && aIx in tabs ? tabs[aIx].title : null;
  },

  getClosedTabData: function sss_getClosedTabDataAt(aWindow) {
    if (!aWindow.__SSi && aWindow.__SS_dyingCache)
      return this._toJSONString(aWindow.__SS_dyingCache._closedTabs);
    
    return this._toJSONString(this._windows[aWindow.__SSi]._closedTabs);
  },

  undoCloseTab: function sss_undoCloseTab(aWindow, aIndex) {
    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    // default to the most-recently closed tab
    aIndex = aIndex || 0;

    if (aIndex in closedTabs) {
      var browser = aWindow.getBrowser();

      // fetch the data of closed tab, while removing it from the array
      var closedTab = closedTabs.splice(aIndex, 1).shift();
      var closedTabState = closedTab.state;

      // create a new tab
      closedTabState._tab = browser.addTab();
        
      // restore the tab's position
      browser.moveTabTo(closedTabState._tab, closedTab.pos);
  
      // restore tab content
      this.restoreHistoryPrecursor(aWindow, [closedTabState], 1, 0, 0);

      // focus the tab's content area
      var content = browser.getBrowserForTab(closedTabState._tab).contentWindow;
      aWindow.setTimeout(function() { content.focus(); }, 0);
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  getWindowValue: function sss_getWindowValue(aWindow, aKey) {
    if (aWindow.__SSi) {
      var data = this._windows[aWindow.__SSi].extData || {};
      return data[aKey] || "";
    }
    else if (aWindow.__SS_dyingCache) {
      data = aWindow.__SS_dyingCache.extData || {};
      return data[aKey] || "";
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  setWindowValue: function sss_setWindowValue(aWindow, aKey, aStringValue) {
    if (aWindow.__SSi) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {};
      }
      this._windows[aWindow.__SSi].extData[aKey] = aStringValue;
      this.saveStateDelayed(aWindow);
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  deleteWindowValue: function sss_deleteWindowValue(aWindow, aKey) {
    if (this._windows[aWindow.__SSi].extData[aKey])
      delete this._windows[aWindow.__SSi].extData[aKey];
    else
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
  },

  getTabValue: function sss_getTabValue(aTab, aKey) {
    var data = aTab.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function sss_setTabValue(aTab, aKey, aStringValue) {
    if (!aTab.__SS_extdata) {
      aTab.__SS_extdata = {};
    }
    aTab.__SS_extdata[aKey] = aStringValue;
    this.saveStateDelayed(aTab.ownerDocument.defaultView);
  },

  deleteTabValue: function sss_deleteTabValue(aTab, aKey) {
    if (aTab.__SS_extdata[aKey])
      delete aTab.__SS_extdata[aKey];
    else
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
  },


  persistTabAttribute: function sss_persistTabAttribute(aName) {
    this.xulAttributes.push(aName);
    this.saveStateDelayed();
  },

/* ........ Saving Functionality .............. */

  /**
   * Store all session data for a window
   * @param aWindow
   *        Window reference
   */
  _saveWindowHistory: function sss_saveWindowHistory(aWindow) {
    var tabbrowser = aWindow.getBrowser();
    var tabs = tabbrowser.mTabs;
    var tabsData = this._windows[aWindow.__SSi].tabs = [];
    
    for (var i = 0; i < tabs.length; i++)
      tabsData.push(this._collectTabData(tabs[i]));
    
    this._windows[aWindow.__SSi].selected = tabbrowser.mTabBox.selectedIndex + 1;
  },

  /**
   * Collect data related to a single tab
   * @param aTab
   *        tabbrowser tab
   * @param aFullData
   *        always return privacy sensitive data (use with care)
   * @returns object
   */
  _collectTabData: function sss_collectTabData(aTab, aFullData) {
    var tabData = { entries: [], index: 0 };
    var browser = aTab.linkedBrowser;
    
    if (!browser || !browser.currentURI)
      // can happen when calling this function right after .addTab()
      return tabData;
    else if (browser.parentNode.__SS_data && browser.parentNode.__SS_data._tab)
      // use the data to be restored when the tab hasn't been completely loaded
      return browser.parentNode.__SS_data;
    
    var history = null;
    try {
      history = browser.sessionHistory;
    }
    catch (ex) { } // this could happen if we catch a tab during (de)initialization
    
    if (history && browser.parentNode.__SS_data &&
        browser.parentNode.__SS_data.entries[history.index] && !aFullData) {
      tabData = browser.parentNode.__SS_data;
      tabData.index = history.index + 1;
    }
    else if (history && history.count > 0) {
      for (var j = 0; j < history.count; j++)
        tabData.entries.push(this._serializeHistoryEntry(history.getEntryAtIndex(j, false),
                                                         aFullData));
      tabData.index = history.index + 1;

      // make sure not to cache privacy sensitive data which shouldn't get out
      if (!aFullData)
        browser.parentNode.__SS_data = tabData;
    }
    else {
      tabData.entries[0] = { url: browser.currentURI.spec };
      tabData.index = 1;
    }
    
    var disallow = [];
    for (var i = 0; i < CAPABILITIES.length; i++)
      if (!browser.docShell["allow" + CAPABILITIES[i]])
        disallow.push(CAPABILITIES[i]);
    if (disallow.length > 0)
      tabData.disallow = disallow.join(",");
    else if (tabData.disallow)
      delete tabData.disallow;
    
    if (this.xulAttributes.length > 0) {
      var xulattr = Array.filter(aTab.attributes, function(aAttr) {
        return this.xulAttributes.indexOf(aAttr.name) > -1;
      }, this).map(function(aAttr) {
        return aAttr.name + "=" + encodeURI(aAttr.value);
      });
      tabData.xultab = xulattr.join(" ");
    }
    
    if (aTab.__SS_extdata)
      tabData.extData = aTab.__SS_extdata;
    else if (tabData.extData)
      delete tabData.extData;
    
    return tabData;
  },

  /**
   * Get an object that is a serialized representation of a History entry
   * Used for data storage
   * @param aEntry
   *        nsISHEntry instance
   * @param aFullData
   *        always return privacy sensitive data (use with care)
   * @returns object
   */
  _serializeHistoryEntry: function sss_serializeHistoryEntry(aEntry, aFullData) {
    var entry = { url: aEntry.URI.spec };
    
    if (aEntry.title && aEntry.title != entry.url) {
      entry.title = aEntry.title;
    }
    if (aEntry.isSubFrame) {
      entry.subframe = true;
    }
    if (!(aEntry instanceof Ci.nsISHEntry)) {
      return entry;
    }
    
    var cacheKey = aEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 &&
        cacheKey.data != 0) {
      // XXXbz would be better to have cache keys implement
      // nsISerializable or something.
      entry.cacheKey = cacheKey.data;
    }
    entry.ID = aEntry.ID;
    
    if (aEntry.contentType)
      entry.contentType = aEntry.contentType;
    
    var x = {}, y = {};
    aEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;
    
    try {
      var prefPostdata = this._prefBranch.getIntPref("sessionstore.postdata");
      if (aEntry.postData && (aFullData ||
            prefPostdata && this._checkPrivacyLevel(aEntry.URI.schemeIs("https")))) {
        aEntry.postData.QueryInterface(Ci.nsISeekableStream).
                        seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
        var stream = Cc["@mozilla.org/binaryinputstream;1"].
                     createInstance(Ci.nsIBinaryInputStream);
        stream.setInputStream(aEntry.postData);
        var postBytes = stream.readByteArray(stream.available());
        var postdata = String.fromCharCode.apply(null, postBytes);
        if (aFullData || prefPostdata == -1 ||
            postdata.replace(/^(Content-.*\r\n)+(\r\n)*/, "").length <=
              prefPostdata) {
          // We can stop doing base64 encoding once our serialization into JSON
          // is guaranteed to handle all chars in strings, including embedded
          // nulls.
          entry.postdata_b64 = btoa(postdata);
        }
      }
    }
    catch (ex) { debug(ex); } // POSTDATA is tricky - especially since some extensions don't get it right

    if (aEntry.owner) {
      // Not catching anything specific here, just possible errors
      // from writeCompoundObject and the like.
      try {
        var binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].
                           createInstance(Ci.nsIObjectOutputStream);
        var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
        pipe.init(false, false, 0, 0xffffffff, null);
        binaryStream.setOutputStream(pipe.outputStream);
        binaryStream.writeCompoundObject(aEntry.owner, Ci.nsISupports, true);
        binaryStream.close();

        // Now we want to read the data from the pipe's input end and encode it.
        var scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].
                               createInstance(Ci.nsIBinaryInputStream);
        scriptableStream.setInputStream(pipe.inputStream);
        var ownerBytes =
          scriptableStream.readByteArray(scriptableStream.available());
        // We can stop doing base64 encoding once our serialization into JSON
        // is guaranteed to handle all chars in strings, including embedded
        // nulls.
        entry.owner_b64 = btoa(String.fromCharCode.apply(null, ownerBytes));
      }
      catch (ex) { debug(ex); }
    }
    
    if (!(aEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }
    
    if (aEntry.childCount > 0) {
      entry.children = [];
      for (var i = 0; i < aEntry.childCount; i++) {
        var child = aEntry.GetChildAt(i);
        if (child) {
          entry.children.push(this._serializeHistoryEntry(child, aFullData));
        }
        else { // to maintain the correct frame order, insert a dummy entry 
          entry.children.push({ url: "about:blank" });
        }
      }
    }
    
    return entry;
  },

  /**
   * Updates the current document's cache of user entered text data
   * @param aPanel
   *        TabPanel reference
   * @param aTextarea
   *        HTML content element
   * @returns bool
   */
  _saveTextData: function sss_saveTextData(aPanel, aTextarea) {
    var id = aTextarea.id ? "#" + aTextarea.id :
                            aTextarea.name;
    if (!id
      || !(aTextarea instanceof Ci.nsIDOMHTMLTextAreaElement 
      || aTextarea instanceof Ci.nsIDOMHTMLInputElement && aTextarea.type != "password")) {
      return false; // nothing to save
    }
    
    if (!aPanel.__SS_text) {
      aPanel.__SS_text = [];
      aPanel.__SS_text._refs = [];
    }
    
    // get the index of the reference to the text element
    var ix = aPanel.__SS_text._refs.indexOf(aTextarea);
    if (ix == -1) {
      // we haven't registered this text element yet - do so now
      aPanel.__SS_text._refs.push(aTextarea);
      ix = aPanel.__SS_text.length;
    }
    else if (!aPanel.__SS_text[ix].cache) {
      // we've already marked this text element for saving (the cache is
      // added during save operations and would have to be updated here)
      return false;
    }
    
    // determine the frame we're in and encode it into the textarea's ID
    var content = aTextarea.ownerDocument.defaultView;
    while (content != content.top) {
      var frames = content.parent.frames;
      for (var i = 0; i < frames.length && frames[i] != content; i++);
      id = i + "|" + id;
      content = content.parent;
    }
    
    // mark this element for saving
    aPanel.__SS_text[ix] = { id: id, element: aTextarea };
    
    return true;
  },

  /**
   * go through all tabs and store the current scroll positions
   * and innerHTML content of WYSIWYG editors
   * @param aWindow
   *        Window reference
   */
  _updateTextAndScrollData: function sss_updateTextAndScrollData(aWindow) {
    var browsers = aWindow.getBrowser().browsers;
    for (var i = 0; i < browsers.length; i++) {
      try {
        var tabData = this._windows[aWindow.__SSi].tabs[i];
        if (tabData.entries.length == 0 ||
            browsers[i].parentNode.__SS_data && browsers[i].parentNode.__SS_data._tab)
          continue; // ignore incompletely initialized tabs
        this._updateTextAndScrollDataForTab(aWindow, browsers[i], tabData);
      }
      catch (ex) { debug(ex); } // get as much data as possible, ignore failures (might succeed the next time)
    }
  },

  /**
   * go through all frames and store the current scroll positions
   * and innerHTML content of WYSIWYG editors
   * @param aWindow
   *        Window reference
   * @param aBrowser
   *        single browser reference
   * @param aTabData
   *        tabData object to add the information to
   * @param aFullData
   *        always return privacy sensitive data (use with care)
   */
  _updateTextAndScrollDataForTab:
    function sss_updateTextAndScrollDataForTab(aWindow, aBrowser, aTabData, aFullData) {
    var text = [];
    if (aBrowser.parentNode.__SS_text &&
        (aFullData || this._checkPrivacyLevel(aBrowser.currentURI.schemeIs("https")))) {
      for (var ix = aBrowser.parentNode.__SS_text.length - 1; ix >= 0; ix--) {
        var data = aBrowser.parentNode.__SS_text[ix];
        if (!data.cache)
          // update the text element's value before adding it to the data structure
          data.cache = encodeURI(data.element.value);
        text.push(data.id + "=" + data.cache);
      }
    }
    if (aBrowser.currentURI.spec == "about:config")
      text = ["#textbox=" + encodeURI(aBrowser.contentDocument.getElementById("textbox").
                                               wrappedJSObject.value)];
    if (text.length > 0)
      aTabData.text = text.join(" ");
    else if (aTabData.text)
      delete aTabData.text;
    
    var tabIndex = (aTabData.index || aTabData.entries.length) - 1;
    // entry data needn't exist for tabs just initialized with an incomplete session state
    if (aTabData.entries[tabIndex])
      this._updateTextAndScrollDataForFrame(aWindow, aBrowser.contentWindow,
                                            aTabData.entries[tabIndex], aFullData);
  },

  /**
   * go through all subframes and store the current scroll positions
   * and innerHTML content of WYSIWYG editors
   * @param aWindow
   *        Window reference
   * @param aContent
   *        frame reference
   * @param aData
   *        part of a tabData object to add the information to
   * @param aFullData
   *        always return privacy sensitive data (use with care)
   */
  _updateTextAndScrollDataForFrame:
    function sss_updateTextAndScrollDataForFrame(aWindow, aContent, aData, aFullData) {
    for (var i = 0; i < aContent.frames.length; i++) {
      if (aData.children && aData.children[i])
        this._updateTextAndScrollDataForFrame(aWindow, aContent.frames[i], aData.children[i], aFullData);
    }
    // designMode is undefined e.g. for XUL documents (as about:config)
    var isHTTPS = this._getURIFromString((aContent.parent || aContent).
                                         document.location.href).schemeIs("https");
    if ((aContent.document.designMode || "") == "on" &&
        (aFullData || this._checkPrivacyLevel(isHTTPS))) {
      if (aData.innerHTML === undefined && !aFullData) {
        // we get no "input" events from iframes - listen for keypress here
        var _this = this;
        aContent.addEventListener("keypress", function(aEvent) {
          _this.saveStateDelayed(aWindow, 3000); }, true);
      }
      aData.innerHTML = aContent.document.body.innerHTML;
    }
    aData.scroll = aContent.scrollX + "," + aContent.scrollY;
   },

  /**
   * store all hosts for a URL
   * @param aWindow
   *        Window reference
   */
  _updateCookieHosts: function sss_updateCookieHosts(aWindow) {
    var hosts = this._windows[aWindow.__SSi]._hosts = {};
    
    // get all possible subdomain levels for a given URL
    var _this = this;
    function extractHosts(aEntry) {
      if (/^https?:\/\/(?:[^@\/\s]+@)?([\w.-]+)/.test(aEntry.url) &&
        !hosts[RegExp.$1] && _this._checkPrivacyLevel(_this._getURIFromString(aEntry.url).schemeIs("https"))) {
        var host = RegExp.$1;
        var ix;
        for (ix = host.indexOf(".") + 1; ix; ix = host.indexOf(".", ix) + 1) {
          hosts[host.substr(ix)] = true;
        }
        hosts[host] = true;
      }
      else if (/^file:\/\/([^\/]*)/.test(aEntry.url)) {
        hosts[RegExp.$1] = true;
      }
      if (aEntry.children) {
        aEntry.children.forEach(extractHosts);
      }
    }
    
    this._windows[aWindow.__SSi].tabs.forEach(function(aTabData) { aTabData.entries.forEach(extractHosts); });
  },

  /**
   * Serialize cookie data
   * @param aWindows
   *        array of Window references
   */
  _updateCookies: function sss_updateCookies(aWindows) {
    var cookiesEnum = Cc["@mozilla.org/cookiemanager;1"].
                      getService(Ci.nsICookieManager).enumerator;
    // collect the cookies per window
    for (var i = 0; i < aWindows.length; i++)
      aWindows[i].cookies = [];
    
    // MAX_EXPIRY should be 2^63-1, but JavaScript can't handle that precision
    var MAX_EXPIRY = Math.pow(2, 62);
    while (cookiesEnum.hasMoreElements()) {
      var cookie = cookiesEnum.getNext().QueryInterface(Ci.nsICookie2);
      if (cookie.isSession && this._checkPrivacyLevel(cookie.isSecure)) {
        var jscookie = null;
        aWindows.forEach(function(aWindow) {
          if (aWindow._hosts && aWindow._hosts[cookie.rawHost]) {
            // serialize the cookie when it's first needed
            if (!jscookie) {
              jscookie = { host: cookie.host, value: cookie.value };
              // only add attributes with non-default values (saving a few bits)
              if (cookie.path) jscookie.path = cookie.path;
              if (cookie.name) jscookie.name = cookie.name;
              if (cookie.isSecure) jscookie.secure = true;
              if (cookie.isHttpOnly) jscookie.httponly = true;
              if (cookie.expiry < MAX_EXPIRY) jscookie.expiry = cookie.expiry;
            }
            aWindow.cookies.push(jscookie);
          }
        });
      }
    }
    
    // don't include empty cookie sections
    for (i = 0; i < aWindows.length; i++)
      if (aWindows[i].cookies.length == 0)
        delete aWindows[i].cookies;
  },

  /**
   * Store window dimensions, visibility, sidebar
   * @param aWindow
   *        Window reference
   */
  _updateWindowFeatures: function sss_updateWindowFeatures(aWindow) {
    var winData = this._windows[aWindow.__SSi];
    
    WINDOW_ATTRIBUTES.forEach(function(aAttr) {
      winData[aAttr] = this._getWindowDimension(aWindow, aAttr);
    }, this);
    
    var hidden = WINDOW_HIDEABLE_FEATURES.filter(function(aItem) {
      return aWindow[aItem] && !aWindow[aItem].visible;
    });
    if (hidden.length != 0)
      winData.hidden = hidden.join(",");
    else if (winData.hidden)
      delete winData.hidden;

    var sidebar = aWindow.document.getElementById("sidebar-box").getAttribute("sidebarcommand");
    if (sidebar)
      winData.sidebar = sidebar;
    else if (winData.sidebar)
      delete winData.sidebar;
  },

  /**
   * serialize session data as Ini-formatted string
   * @returns string
   */
  _getCurrentState: function sss_getCurrentState() {
    var activeWindow = this._getMostRecentBrowserWindow();
    
    if (this._loadState == STATE_RUNNING) {
      // update the data for all windows with activities since the last save operation
      this._forEachBrowserWindow(function(aWindow) {
        if (this._dirty || this._dirtyWindows[aWindow.__SSi] || aWindow == activeWindow) {
          this._collectWindowData(aWindow);
        }
        else { // always update the window features (whose change alone never triggers a save operation)
          this._updateWindowFeatures(aWindow);
        }
      }, this);
      this._dirtyWindows = [];
      this._dirty = false;
    }
    
    // collect the data for all windows
    var total = [], windows = [];
    var ix;
    for (ix in this._windows) {
      total.push(this._windows[ix]);
      windows.push(ix);
    }
    this._updateCookies(total);
    
    // if no browser window remains open, return the state of the last closed window
    if (total.length == 0 && this._lastWindowClosed) {
      total.push(this._lastWindowClosed);
    }
    if (activeWindow) {
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    }
    ix = this.activeWindowSSiCache ? windows.indexOf(this.activeWindowSSiCache) : -1;

    return { windows: total, selectedWindow: ix + 1 };
  },

  /**
   * serialize session data for a window 
   * @param aWindow
   *        Window reference
   * @returns string
   */
  _getWindowState: function sss_getWindowState(aWindow) {
    if (this._loadState == STATE_RUNNING) {
      this._collectWindowData(aWindow);
    }
    
    var total = [this._windows[aWindow.__SSi]];
    this._updateCookies(total);
    
    return { windows: total };
  },

  _collectWindowData: function sss_collectWindowData(aWindow) {
    // update the internal state data for this window
    this._saveWindowHistory(aWindow);
    this._updateTextAndScrollData(aWindow);
    this._updateCookieHosts(aWindow);
    this._updateWindowFeatures(aWindow);
    
    this._dirtyWindows[aWindow.__SSi] = false;
  },

/* ........ Restoring Functionality .............. */

  /**
   * restore features to a single window
   * @param aWindow
   *        Window reference
   * @param aState
   *        JS object or its eval'able source
   * @param aOverwriteTabs
   *        bool overwrite existing tabs w/ new ones
   * @param aFollowUp
   *        bool this isn't the restoration of the first window
   */
  restoreWindow: function sss_restoreWindow(aWindow, aState, aOverwriteTabs, aFollowUp) {
    if (this._restoreCount) {
      this._restoreCount--;
      if (this._restoreCount == 0) {
        // This was the last window restored at startup, notify observers.
        var observerService = Cc["@mozilla.org/observer-service;1"].
                              getService(Ci.nsIObserverService);
        observerService.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
      }
    }

    if (!aFollowUp) {
      this.windowToFocus = aWindow;
    }
    // initialize window if necessary
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    try {
      var root = typeof aState == "string" ? this._safeEval(aState) : aState;
      if (!root.windows[0]) {
        return; // nothing to restore
      }
    }
    catch (ex) { // invalid state object - don't restore anything 
      debug(ex);
      return;
    }
    
    var winData;
    if (!aState.selectedWindow) {
      aState.selectedWindow = 0;
    }
    // open new windows for all further window entries of a multi-window session
    // (unless they don't contain any tab data)
    for (var w = 1; w < root.windows.length; w++) {
      winData = root.windows[w];
      if (winData && winData.tabs && winData.tabs[0]) {
        var window = this._openWindowWithState({ windows: [winData] });
        if (w == aState.selectedWindow - 1) {
          this.windowToFocus = window;
        }
      }
    }
    winData = root.windows[0];
    if (!winData.tabs) {
      winData.tabs = [];
    }
    
    var tabbrowser = aWindow.getBrowser();
    var openTabCount = aOverwriteTabs ? tabbrowser.browsers.length : -1;
    var newTabCount = winData.tabs.length;
    
    for (var t = 0; t < newTabCount; t++) {
      winData.tabs[t]._tab = t < openTabCount ? tabbrowser.mTabs[t] : tabbrowser.addTab();
      // when resuming at startup: add additionally requested pages to the end
      if (!aOverwriteTabs && root._firstTabs) {
        tabbrowser.moveTabTo(winData.tabs[t]._tab, t);
      }
    }

    // when overwriting tabs, remove all superflous ones
    for (t = openTabCount - 1; t >= newTabCount; t--) {
      tabbrowser.removeTab(tabbrowser.mTabs[t]);
    }
    
    if (aOverwriteTabs) {
      this.restoreWindowFeatures(aWindow, winData);
    }
    if (winData.cookies) {
      this.restoreCookies(winData.cookies);
    }
    if (winData.extData) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {}
      }
      for (var key in winData.extData) {
        this._windows[aWindow.__SSi].extData[key] = winData.extData[key];
      }
    }
    if (winData._closedTabs && (root._firstTabs || aOverwriteTabs)) {
      this._windows[aWindow.__SSi]._closedTabs = winData._closedTabs;
    }
    
    this.restoreHistoryPrecursor(aWindow, winData.tabs, (aOverwriteTabs ?
      (parseInt(winData.selected) || 1) : 0), 0, 0);
  },

  /**
   * Manage history restoration for a window
   * @param aTabs
   *        Array of tab data
   * @param aCurrentTabs
   *        Array of tab references
   * @param aSelectTab
   *        Index of selected tab
   * @param aIx
   *        Index of the next tab to check readyness for
   * @param aCount
   *        Counter for number of times delaying b/c browser or history aren't ready
   */
  restoreHistoryPrecursor: function sss_restoreHistoryPrecursor(aWindow, aTabs, aSelectTab, aIx, aCount) {
    var tabbrowser = aWindow.getBrowser();
    
    // make sure that all browsers and their histories are available
    // - if one's not, resume this check in 100ms (repeat at most 10 times)
    for (var t = aIx; t < aTabs.length; t++) {
      try {
        if (!tabbrowser.getBrowserForTab(aTabs[t]._tab).webNavigation.sessionHistory) {
          throw new Error();
        }
      }
      catch (ex) { // in case browser or history aren't ready yet 
        if (aCount < 10) {
          var restoreHistoryFunc = function(self) {
            self.restoreHistoryPrecursor(aWindow, aTabs, aSelectTab, aIx, aCount + 1);
          }
          aWindow.setTimeout(restoreHistoryFunc, 100, this);
          return;
        }
      }
    }
    
    // mark the tabs as loading
    for (t = 0; t < aTabs.length; t++) {
      if (!aTabs[t].entries || !aTabs[t].entries[0])
        continue; // there won't be anything to load
      
      var tab = aTabs[t]._tab;
      var browser = tabbrowser.getBrowserForTab(tab);
      browser.stop(); // in case about:blank isn't done yet
      
      tab.setAttribute("busy", "true");
      tabbrowser.updateIcon(tab);
      tabbrowser.setTabTitleLoading(tab);
      
      // keep the data around to prevent dataloss in case
      // a tab gets closed before it's been properly restored
      browser.parentNode.__SS_data = aTabs[t];
    }
    
    // make sure to restore the selected tab first (if any)
    if (aSelectTab-- && aTabs[aSelectTab]) {
        aTabs.unshift(aTabs.splice(aSelectTab, 1)[0]);
        tabbrowser.selectedTab = aTabs[0]._tab;
    }

    // helper hash for ensuring unique frame IDs
    var idMap = { used: {} };
    this.restoreHistory(aWindow, aTabs, idMap);
  },

  /**
   * Restory history for a window
   * @param aWindow
   *        Window reference
   * @param aTabs
   *        Array of tab data
   * @param aIdMap
   *        Hash for ensuring unique frame IDs
   */
  restoreHistory: function sss_restoreHistory(aWindow, aTabs, aIdMap) {
    var _this = this;
    while (aTabs.length > 0 && (!aTabs[0]._tab || !aTabs[0]._tab.parentNode)) {
      aTabs.shift(); // this tab got removed before being completely restored
    }
    if (aTabs.length == 0) {
      return; // no more tabs to restore
    }
    
    var tabData = aTabs.shift();

    var tab = tabData._tab;
    var browser = aWindow.getBrowser().getBrowserForTab(tab);
    var history = browser.webNavigation.sessionHistory;
    
    if (history.count > 0) {
      history.PurgeHistory(history.count);
    }
    history.QueryInterface(Ci.nsISHistoryInternal);
    
    if (!tabData.entries) {
      tabData.entries = [];
    }
    if (tabData.extData) {
      tab.__SS_extdata = tabData.extData;
    }
    
    for (var i = 0; i < tabData.entries.length; i++) {
      history.addEntry(this._deserializeHistoryEntry(tabData.entries[i], aIdMap), true);
    }
    
    // make sure to reset the capabilities and attributes, in case this tab gets reused
    var disallow = (tabData.disallow)?tabData.disallow.split(","):[];
    CAPABILITIES.forEach(function(aCapability) {
      browser.docShell["allow" + aCapability] = disallow.indexOf(aCapability) == -1;
    });
    Array.filter(tab.attributes, function(aAttr) {
      return (_this.xulAttributes.indexOf(aAttr.name) > -1);
    }).forEach(tab.removeAttribute, tab);
    if (tabData.xultab) {
      tabData.xultab.split(" ").forEach(function(aAttr) {
        if (/^([^\s=]+)=(.*)/.test(aAttr)) {
          tab.setAttribute(RegExp.$1, decodeURI(RegExp.$2));
        }
      });
    }
    
    // notify the tabbrowser that the tab chrome has been restored
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabRestoring", true, false);
    tab.dispatchEvent(event);
    
    var activeIndex = (tabData.index || tabData.entries.length) - 1;
    try {
      browser.webNavigation.gotoIndex(activeIndex);
    }
    catch (ex) { } // ignore an invalid tabData.index
    
    // restore those aspects of the currently active documents
    // which are not preserved in the plain history entries
    // (mainly scroll state and text data)
    browser.__SS_restore_data = tabData.entries[activeIndex] || {};
    browser.__SS_restore_text = tabData.text || "";
    browser.__SS_restore_tab = tab;
    browser.__SS_restore = this.restoreDocument_proxy;
    browser.addEventListener("load", browser.__SS_restore, true);
    
    aWindow.setTimeout(function(){ _this.restoreHistory(aWindow, aTabs, aIdMap); }, 0);
  },

  /**
   * expands serialized history data into a session-history-entry instance
   * @param aEntry
   *        Object containing serialized history data for a URL
   * @param aIdMap
   *        Hash for ensuring unique frame IDs
   * @returns nsISHEntry
   */
  _deserializeHistoryEntry: function sss_deserializeHistoryEntry(aEntry, aIdMap) {
    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);
    
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    shEntry.setURI(ioService.newURI(aEntry.url, null, null));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    
    if (aEntry.cacheKey) {
      var cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].
                     createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = aEntry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }

    if (aEntry.ID) {
      // get a new unique ID for this frame (since the one from the last
      // start might already be in use)
      var id = aIdMap[aEntry.ID] || 0;
      if (!id) {
        for (id = Date.now(); aIdMap.used[id]; id++);
        aIdMap[aEntry.ID] = id;
        aIdMap.used[id] = true;
      }
      shEntry.ID = id;
    }
    
    if (aEntry.scroll) {
      var scrollPos = (aEntry.scroll || "0,0").split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    var postdata;
    if (aEntry.postdata_b64) {  // Firefox 3
      postdata = atob(aEntry.postdata_b64);
    } else if (aEntry.postdata) { // Firefox 2
      postdata = aEntry.postdata;
    }

    if (postdata) {
      var stream = Cc["@mozilla.org/io/string-input-stream;1"].
                   createInstance(Ci.nsIStringInputStream);
      stream.setData(postdata, postdata.length);
      shEntry.postData = stream;
    }

    if (aEntry.owner_b64) {  // Firefox 3
      var ownerInput = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
      var binaryData = atob(aEntry.owner_b64);
      ownerInput.setData(binaryData, binaryData.length);
      var binaryStream = Cc["@mozilla.org/binaryinputstream;1"].
                         createInstance(Ci.nsIObjectInputStream);
      binaryStream.setInputStream(ownerInput);
      try { // Catch possible deserialization exceptions
        shEntry.owner = binaryStream.readObject(true);
      } catch (ex) { debug(ex); }
    } else if (aEntry.ownerURI) { // Firefox 2
      var uriObj = ioService.newURI(aEntry.ownerURI, null, null);
      shEntry.owner = Cc["@mozilla.org/scriptsecuritymanager;1"].
                      getService(Ci.nsIScriptSecurityManager).
                      getCodebasePrincipal(uriObj);
    }
    
    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (var i = 0; i < aEntry.children.length; i++) {
        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap), i);
      }
    }
    
    return shEntry;
  },

  /**
   * Restore properties to a loaded document
   */
  restoreDocument_proxy: function sss_restoreDocument_proxy(aEvent) {
    // wait for the top frame to be loaded completely
    if (!aEvent || !aEvent.originalTarget || !aEvent.originalTarget.defaultView || aEvent.originalTarget.defaultView != aEvent.originalTarget.defaultView.top) {
      return;
    }
    
    var textArray = this.__SS_restore_text ? this.__SS_restore_text.split(" ") : [];
    function restoreTextData(aContent, aPrefix) {
      textArray.forEach(function(aEntry) {
        if (/^((?:\d+\|)*)(#?)([^\s=]+)=(.*)$/.test(aEntry) && (!RegExp.$1 || RegExp.$1 == aPrefix)) {
          var document = aContent.document;
          var node = RegExp.$2 ? document.getElementById(RegExp.$3) : document.getElementsByName(RegExp.$3)[0] || null;
          if (node && "value" in node) {
            node.value = decodeURI(RegExp.$4);
            
            var event = document.createEvent("UIEvents");
            event.initUIEvent("input", true, true, aContent, 0);
            node.dispatchEvent(event);
          }
        }
      });
    }
    
    function restoreTextDataAndScrolling(aContent, aData, aPrefix) {
      restoreTextData(aContent, aPrefix);
      if (aData.innerHTML) {
        aContent.setTimeout(
              function(aHTML) {
                if (aContent.document.designMode == "on") {
                  aContent.document.body.innerHTML = aHTML;
                }
              }, 0, aData.innerHTML);
      }
      if (aData.scroll && /(\d+),(\d+)/.test(aData.scroll)) {
        aContent.scrollTo(RegExp.$1, RegExp.$2);
      }
      for (var i = 0; i < aContent.frames.length; i++) {
        if (aData.children && aData.children[i]) {
          restoreTextDataAndScrolling(aContent.frames[i], aData.children[i], i + "|" + aPrefix);
        }
      }
    }
    
    var content = aEvent.originalTarget.defaultView;
    if (this.currentURI.spec == "about:config") {
      // unwrap the document for about:config because otherwise the properties
      // of the XBL bindings - as the textbox - aren't accessible (see bug 350718)
      content = content.wrappedJSObject;
    }
    restoreTextDataAndScrolling(content, this.__SS_restore_data, "");
    
    // notify the tabbrowser that this document has been completely restored
    var event = this.ownerDocument.createEvent("Events");
    event.initEvent("SSTabRestored", true, false);
    this.__SS_restore_tab.dispatchEvent(event);
    
    this.removeEventListener("load", this.__SS_restore, true);
    delete this.__SS_restore_data;
    delete this.__SS_restore_text;
    delete this.__SS_restore_tab;
    delete this.__SS_restore;
  },

  /**
   * Restore visibility and dimension features to a window
   * @param aWindow
   *        Window reference
   * @param aWinData
   *        Object containing session data for the window
   */
  restoreWindowFeatures: function sss_restoreWindowFeatures(aWindow, aWinData) {
    var hidden = (aWinData.hidden)?aWinData.hidden.split(","):[];
    WINDOW_HIDEABLE_FEATURES.forEach(function(aItem) {
      aWindow[aItem].visible = hidden.indexOf(aItem) == -1;
    });
    
    var _this = this;
    aWindow.setTimeout(function() {
      _this.restoreDimensions.apply(_this, [aWindow, aWinData.width || 0, 
        aWinData.height || 0, "screenX" in aWinData ? aWinData.screenX : NaN,
        "screenY" in aWinData ? aWinData.screenY : NaN,
        aWinData.sizemode || "", aWinData.sidebar || ""]);
    }, 0);
  },

  /**
   * Restore a window's dimensions
   * @param aWidth
   *        Window width
   * @param aHeight
   *        Window height
   * @param aLeft
   *        Window left
   * @param aTop
   *        Window top
   * @param aSizeMode
   *        Window size mode (eg: maximized)
   * @param aSidebar
   *        Sidebar command
   */
  restoreDimensions: function sss_restoreDimensions(aWindow, aWidth, aHeight, aLeft, aTop, aSizeMode, aSidebar) {
    var win = aWindow;
    var _this = this;
    function win_(aName) { return _this._getWindowDimension(win, aName); }
    
    // only modify those aspects which aren't correct yet
    if (aWidth && aHeight && (aWidth != win_("width") || aHeight != win_("height"))) {
      aWindow.resizeTo(aWidth, aHeight);
    }
    if (!isNaN(aLeft) && !isNaN(aTop) && (aLeft != win_("screenX") || aTop != win_("screenY"))) {
      aWindow.moveTo(aLeft, aTop);
    }
    if (aSizeMode == "maximized" && win_("sizemode") != "maximized") {
      aWindow.maximize();
    }
    else if (aSizeMode && aSizeMode != "maximized" && win_("sizemode") != "normal") {
      aWindow.restore();
    }
    var sidebar = aWindow.document.getElementById("sidebar-box");
    if (sidebar.getAttribute("sidebarcommand") != aSidebar) {
      aWindow.toggleSidebar(aSidebar);
    }
    // since resizing/moving a window brings it to the foreground,
    // we might want to re-focus the last focused window
    if (this.windowToFocus) {
      this.windowToFocus.content.focus();
    }
  },

  /**
   * Restores cookies (accepting both Firefox 2.0 and current format)
   * @param aCookies
   *        Array of cookie objects
   */
  restoreCookies: function sss_restoreCookies(aCookies) {
    if (aCookies.count && aCookies.domain1) {
      // convert to the new cookie serialization format
      var converted = [];
      for (var i = 1; i <= aCookies.count; i++) {
        // for simplicity we only accept the format we produced ourselves
        var parsed = aCookies["value" + i].match(/^([^=;]+)=([^;]*);(?:domain=[^;]+;)?(?:path=([^;]*);)?(secure;)?(httponly;)?/);
        if (parsed && /^https?:\/\/([^\/]+)/.test(aCookies["domain" + i]))
          converted.push({
            host: RegExp.$1, path: parsed[3], name: parsed[1], value: parsed[2],
            secure: parsed[4], httponly: parsed[5]
          });
      }
      aCookies = converted;
    }
    
    var cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                        getService(Ci.nsICookieManager2);
    // MAX_EXPIRY should be 2^63-1, but JavaScript can't handle that precision
    var MAX_EXPIRY = Math.pow(2, 62);
    for (i = 0; i < aCookies.length; i++) {
      var cookie = aCookies[i];
      try {
        cookieManager.add(cookie.host, cookie.path || "", cookie.name || "", cookie.value, !!cookie.secure, !!cookie.httponly, true, "expiry" in cookie ? cookie.expiry : MAX_EXPIRY);
      }
      catch (ex) { Cu.reportError(ex); } // don't let a single cookie stop recovering
    }
  },

/* ........ Disk Access .............. */

  /**
   * save state delayed by N ms
   * marks window as dirty (i.e. data update can't be skipped)
   * @param aWindow
   *        Window reference
   * @param aDelay
   *        Milliseconds to delay
   */
  saveStateDelayed: function sss_saveStateDelayed(aWindow, aDelay) {
    if (aWindow) {
      this._dirtyWindows[aWindow.__SSi] = true;
    }

    if (!this._saveTimer && this._resume_from_crash) {
      // interval until the next disk operation is allowed
      var minimalDelay = this._lastSaveTime + this._interval - Date.now();
      
      // if we have to wait, set a timer, otherwise saveState directly
      aDelay = Math.max(minimalDelay, aDelay || 2000);
      if (aDelay > 0) {
        this._saveTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._saveTimer.init(this, aDelay, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.saveState();
      }
    }
  },

  /**
   * save state to disk
   * @param aUpdateAll
   *        Bool update all windows 
   */
  saveState: function sss_saveState(aUpdateAll) {
    // if crash recovery is disabled, only save session resuming information
    if (!this._resume_from_crash && this._loadState == STATE_RUNNING)
      return;
    
    this._dirty = aUpdateAll;
    var oState = this._getCurrentState();
    oState.session = { state: ((this._loadState == STATE_RUNNING) ? STATE_RUNNING_STR : STATE_STOPPED_STR) };
    this._writeFile(this._sessionFile, oState.toSource());
    this._lastSaveTime = Date.now();
  },

  /**
   * delete session datafile and backup
   */
  _clearDisk: function sss_clearDisk() {
    if (this._sessionFile.exists()) {
      try {
        this._sessionFile.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } // couldn't remove the file - what now?
    }
    if (this._sessionFileBackup.exists()) {
      try {
        this._sessionFileBackup.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } // couldn't remove the file - what now?
    }
  },

/* ........ Auxiliary Functions .............. */

  /**
   * call a callback for all currently opened browser windows
   * (might miss the most recent one)
   * @param aFunc
   *        Callback each window is passed to
   */
  _forEachBrowserWindow: function sss_forEachBrowserWindow(aFunc) {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    var windowsEnum = windowMediator.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      if (window.__SSi) {
        aFunc.call(this, window);
      }
    }
  },

  /**
   * Returns most recent window
   * @returns Window reference
   */
  _getMostRecentBrowserWindow: function sss_getMostRecentBrowserWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    return windowMediator.getMostRecentWindow("navigator:browser");
  },

  /**
   * open a new browser window for a given session state
   * called when restoring a multi-window session
   * @param aState
   *        Object containing session data
   */
  _openWindowWithState: function sss_openWindowWithState(aState) {
    var argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";

    //XXXzeniko shouldn't it be possible to set the window's dimensions here (as feature)?
    var window = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                 getService(Ci.nsIWindowWatcher).
                 openWindow(null, this._prefBranch.getCharPref("chromeURL"), "_blank",
                            "chrome,dialog=no,all", argString);
    
    window.__SS_state = aState;
    var _this = this;
    window.addEventListener("load", function(aEvent) {
      aEvent.currentTarget.removeEventListener("load", arguments.callee, true);
      _this.restoreWindow(aEvent.currentTarget, aEvent.currentTarget.__SS_state, true, true);
      delete aEvent.currentTarget.__SS_state;
    }, true);
    
    return window;
  },

  /**
   * Whether or not to resume session, if not recovering from a crash.
   * @returns bool
   */
  _doResumeSession: function sss_doResumeSession() {
    return this._prefBranch.getIntPref("startup.page") == 3 ||
      this._prefBranch.getBoolPref("sessionstore.resume_session_once");
  },

  /**
   * whether the user wants to load any other page at startup
   * (except the homepage) - needed for determining whether to overwrite the current tabs
   * C.f.: nsBrowserContentHandler's defaultArgs implementation.
   * @returns bool
   */
  _isCmdLineEmpty: function sss_isCmdLineEmpty(aWindow) {
    var defaultArgs = Cc["@mozilla.org/browser/clh;1"].
                      getService(Ci.nsIBrowserHandler).defaultArgs;
    if (aWindow.arguments && aWindow.arguments[0] &&
        aWindow.arguments[0] == defaultArgs)
      aWindow.arguments[0] = null;

    return !aWindow.arguments || !aWindow.arguments[0];
  },

  /**
   * don't save sensitive data if the user doesn't want to
   * (distinguishes between encrypted and non-encrypted sites)
   * @param aIsHTTPS
   *        Bool is encrypted
   * @returns bool
   */
  _checkPrivacyLevel: function sss_checkPrivacyLevel(aIsHTTPS) {
    return this._prefBranch.getIntPref("sessionstore.privacy_level") < (aIsHTTPS ? PRIVACY_ENCRYPTED : PRIVACY_FULL);
  },

  /**
   * on popup windows, the XULWindow's attributes seem not to be set correctly
   * we use thus JSDOMWindow attributes for sizemode and normal window attributes
   * (and hope for reasonable values when maximized/minimized - since then
   * outerWidth/outerHeight aren't the dimensions of the restored window)
   * @param aWindow
   *        Window reference
   * @param aAttribute
   *        String sizemode | width | height | other window attribute
   * @returns string
   */
  _getWindowDimension: function sss_getWindowDimension(aWindow, aAttribute) {
    if (aAttribute == "sizemode") {
      switch (aWindow.windowState) {
      case aWindow.STATE_MAXIMIZED:
        return "maximized";
      case aWindow.STATE_MINIMIZED:
        return "minimized";
      default:
        return "normal";
      }
    }
    
    var dimension;
    switch (aAttribute) {
    case "width":
      dimension = aWindow.outerWidth;
      break;
    case "height":
      dimension = aWindow.outerHeight;
      break;
    default:
      dimension = aAttribute in aWindow ? aWindow[aAttribute] : "";
      break;
    }
    
    if (aWindow.windowState == aWindow.STATE_NORMAL) {
      return dimension;
    }
    return aWindow.document.documentElement.getAttribute(aAttribute) || dimension;
  },

  /**
   * Convenience method to get localized string bundles
   * @param aURI
   * @returns nsIStringBundle
   */
  _getStringBundle: function sss_getStringBundle(aURI) {
     var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                         getService(Ci.nsIStringBundleService);
     var appLocale = Cc["@mozilla.org/intl/nslocaleservice;1"].
                     getService(Ci.nsILocaleService).getApplicationLocale();
     return bundleService.createBundle(aURI, appLocale);
  },

  /**
   * Get nsIURI from string
   * @param string
   * @returns nsIURI
   */
  _getURIFromString: function sss_getURIFromString(aString) {
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    return ioService.newURI(aString, null, null);
  },

  /**
   * Annotate a breakpad crash report with the currently selected tab's URL.
   */
  _updateCrashReportURL: function sss_updateCrashReportURL(aWindow) {
    if (!Ci.nsICrashReporter) {
      // if breakpad isn't built, don't bother next time at all
      this._updateCrashReportURL = function(aWindow) {};
      return;
    }
    try {
      var currentUrl = aWindow.getBrowser().currentURI.spec;
      var cr = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsICrashReporter);
      cr.annotateCrashReport("URL", currentUrl);
    }
    catch (ex) {
      // don't make noise when crashreporter is built but not enabled
      if (ex.result != Components.results.NS_ERROR_NOT_INITIALIZED)
        debug(ex);
    }
  },

  /**
   * safe eval'ing
   */
  _safeEval: function sss_safeEval(aStr) {
    return Cu.evalInSandbox(aStr, new Cu.Sandbox("about:blank"));
  },

  /**
   * Converts a JavaScript object into a JSON string
   * (see http://www.json.org/ for more information).
   *
   * The inverse operation consists of eval("(" + JSON_string + ")");
   * and should be provably safe.
   *
   * @param aJSObject is the object to be converted
   * @return the object's JSON representation
   */
  _toJSONString: function sss_toJSONString(aJSObject) {
    var str = JSON.toString(aJSObject, ["_tab", "_hosts"] /* keys to drop */);
    
    // sanity check - so that API consumers can just eval this string
    if (!JSON.isMostlyHarmless(str))
      throw new Error("JSON conversion failed unexpectedly!");
    
    return str;
  },

/* ........ Storage API .............. */

  /**
   * write file to disk
   * @param aFile
   *        nsIFile
   * @param aData
   *        String data
   */
  _writeFile: function sss_writeFile(aFile, aData) {
    // init stream
    var stream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                 createInstance(Ci.nsIFileOutputStream);
    stream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

    // convert to UTF-8
    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    var convertedData = converter.ConvertFromUnicode(aData);
    convertedData += converter.Finish();

    // write and close stream
    stream.write(convertedData, convertedData.length);
    if (stream instanceof Ci.nsISafeOutputStream) {
      stream.finish();
    } else {
      stream.close();
    }
  }
};

function NSGetModule(aComMgr, aFileSpec)
  XPCOMUtils.generateModule([SessionStoreService]);
