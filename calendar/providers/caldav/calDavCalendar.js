/* -*- Mode: javascript; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Oracle Corporation code.
 *
 * The Initial Developer of the Original Code is
 *  Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir.vukicevic@oracle.com>
 *   Dan Mosedale <dan.mosedale@oracle.com>
 *   Mike Shaver <mike.x.shaver@oracle.com>
 *   Gary van der Merwe <garyvdm@gmail.com>
 *   Bruno Browning <browning@uwalumni.com>
 *   Matthew Willis <lilmatt@mozilla.com>
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

//
// calDavCalendar.js
//

// XXXdmose deal with generation goop

// XXXdmose need to re-query for add & modify to get up-to-date items

// XXXdmose deal with locking

// XXXdmose need to make and use better error reporting interface for webdav
// (all uses of aStatusCode, probably)

// XXXdmose use real calendar result codes, not NS_ERROR_FAILURE for everything

const xmlHeader = '<?xml version="1.0" encoding="UTF-8"?>\n';

function calDavCalendar() {
    this.wrappedJSObject = this;
    this.mObservers = Array();
    this.unmappedProperties = [];
    this.mUriParams = null;
    this.mEtagCache = Array();
}

// some shorthand
const nsIWebDAVOperationListener = 
    Components.interfaces.nsIWebDAVOperationListener;
const calICalendar = Components.interfaces.calICalendar;
const nsISupportsCString = Components.interfaces.nsISupportsCString;
const calIErrors = Components.interfaces.calIErrors;

var appInfo = Components.classes["@mozilla.org/xre/app-info;1"]
                        .getService(Components.interfaces.nsIXULAppInfo);
var isOnBranch = appInfo.platformVersion.indexOf("1.8") == 0;


function makeOccurrence(item, start, end)
{
    var occ = item.createProxy();
    occ.recurrenceId = start;
    occ.startDate = start;
    occ.endDate = end;

    return occ;
}
  
// END_OF_TIME needs to be the max value a PRTime can be
const START_OF_TIME = -0x7fffffffffffffff;
const END_OF_TIME = 0x7fffffffffffffff;

// used by mAuthenticationStatus
const kCaldavNoAuthentication = 0;
const kCaldavFirstRequestSent = 1;      // Queueing subsequent requests
const kCaldavFreshlyAuthenticated = 2;  // Need to process queue
const kCaldavAuthenticated = 3;         // Queue being processed or empty

// used in checking calendar URI for (Cal)DAV-ness
const kDavResourceTypeNone = 0;
const kDavResourceTypeCollection = 1;
const kDavResourceTypeCalendar = 2;

// used for etag checking
const CALDAV_ADOPT_ITEM = 1;
const CALDAV_MODIFY_ITEM = 2;
const CALDAV_DELETE_ITEM = 3;
const CALDAV_CACHE_ETAG = 4;

calDavCalendar.prototype = {
    //
    // nsISupports interface
    // 
    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(Components.interfaces.calICalendarProvider) &&
            !aIID.equals(Components.interfaces.calICalendar) &&
            !aIID.equals(Components.interfaces.nsIInterfaceRequestor)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    //
    // calICalendarProvider interface
    //
    get prefChromeOverlay() {
        return null;
    },

    get displayName() {
        return calGetString("calendar", "caldavName");
    },

    createCalendar: function caldav_createCal() {
        throw NS_ERROR_NOT_IMPLEMENTED;
    },

    deleteCalendar: function caldav_deleteCal(cal, listener) {
        throw NS_ERROR_NOT_IMPLEMENTED;
    },

    //
    // calICalendar interface
    //

    // attribute AUTF8String id;
    mID: null,
    get id() {
        return this.mID;
    },
    set id(id) {
        if (this.mID)
            throw Components.results.NS_ERROR_ALREADY_INITIALIZED;
        return (this.mID = id);
    },

    // attribute AUTF8String name;
    get name() {
        return getCalendarManager().getCalendarPref(this, "NAME");
    },
    set name(name) {
        getCalendarManager().setCalendarPref(this, "NAME", name);
    },

    // readonly attribute AUTF8String type;
    get type() { return "caldav"; },

    mReadOnly: false,

    get readOnly() { 
        return this.mReadOnly;
    },
    set readOnly(bool) {
        this.mReadOnly = bool;
    },

    // attribute PRUInt8 mAuthenticationStatus;
    mAuthenticationStatus: 0,

    mPendingStartupRequests: Array(),

    get canRefresh() {
        // refresh() is currently not implemented, but we may want to change that
        return false;
    },

    // mUriParams stores trailing ?parameters from the
    // supplied calendar URI. Needed for (at least) Cosmo
    // tickets
    mUriParams: null,

    // attribute nsIURI uri;
    mUri: null,
    get uri() { return this.mUri; },
    set uri(aUri) { this.mUri = aUri; },

    get mCalendarUri() { 
        calUri = this.mUri.clone();
        var parts = calUri.spec.split('?');
        if (parts.length > 1) {
            calUri.spec = parts.shift();
            this.mUriParams = '?' + parts.join('?');
        }
        if (calUri.spec.charAt(calUri.spec.length-1) != '/') {
            calUri.spec += "/";
        }
        return calUri;
    },
    
    mMakeUri: function caldav_makeUri(aInsertString) {
        var spec = this.mCalendarUri.spec + aInsertString;
        if (this.mUriParams) {
            return spec + this.mUriParams;
        }
        return spec;
    },

    get mLocationPath() {
        return decodeURIComponent(this.mCalendarUri.path);
    },

    refresh: function() {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
    },

    // attribute boolean suppressAlarms;
    get suppressAlarms() { return false; },
    set suppressAlarms(aSuppressAlarms) { throw Components.results.NS_ERROR_NOT_IMPLEMENTED; },

    get sendItipInvitations() { return true; },

    // void addObserver( in calIObserver observer );
    addObserver: function (aObserver, aItemFilter) {
        for each (obs in this.mObservers) {
            if (compareObjects(obs, aObserver, Ci.calIObserver)) {
                return;
            }
        }

        this.mObservers.push(aObserver);
    },

    // void removeObserver( in calIObserver observer );
    removeObserver: function (aObserver) {
        this.mObservers = this.mObservers.filter(
            function(o) {return (o != aObserver);});
    },

    /**
     * Fetches etag from server and compares with local cached version
     * before we add/adopt/modify/delete item.
     *
     * @param aMethod     requested method (adopt/modify/delete)
     * @param aItem       item to check
     * @param aListener   listener from original request
     * @param aOldItem    aOldItem argument in modifyItem requests
     */
    fetchEtag: function caldavFE(aMethod, aItem, aListener, aOldItem) {
        if (this.readOnly) {
            throw calIErrors.CAL_IS_READONLY;
        }

        var serverEtag = null;
        var listener = new WebDavListener();

        var thisCalendar = this;

        var itemUri = this.mCalendarUri.clone();

        try {
            itemUri.spec = this.mMakeUri(aItem.getProperty("X-MOZ-LOCATIONPATH"));
            LOG("using X-MOZ-LOCATIONPATH: " + itemUri.spec);
        } catch (ex) {
            // XXX how are we REALLY supposed to figure this out?
            itemUri.spec = this.mMakeUri(aItem.id + ".ics");
        }

        var itemResource = new WebDavResource(itemUri);

        listener.onOperationComplete = function OOC(aStatusCode,
                                                    aResource,
                                                    aOperation,
                                                    aClosure)
        {
            var mismatch = (serverEtag != thisCalendar.mEtagCache[aItem.id]);

            switch (aMethod) {
                case CALDAV_ADOPT_ITEM:
                    if (serverEtag != null) {
                        // The server thinks it already has an item we want to
                        // create as new. This either a server error or we're
                        // trying to copy an item onto itself; either way the
                        // safe thing to do is abort the operation.
                        LOG("CalDAV: non-null etag in adoptItem");
                        thisCalendar.readOnly = true;
                    } else {
                        thisCalendar.performAdoptItem(aItem, aListener);
                    }
                    break;

                case CALDAV_MODIFY_ITEM:
                    if (mismatch) {
                        LOG("CalDAV: etag mismatch in modifyItem");
                        thisCalendar.promptOverwrite(aMethod, aItem,
                                                     aListener, aOldItem);
                    } else {
                        thisCalendar.performModifyItem(aItem, aOldItem, aListener);
                    }
                    break;

                case CALDAV_DELETE_ITEM:
                    if (mismatch) {
                        LOG("CalDAV: etag mismatch in deleteItem");
                        thisCalendar.promptOverwrite(aMethod, aItem,
                                                     aListener, null);
                    } else {
                        thisCalendar.performDeleteItem(aItem, aListener);
                    }
                    break;

                default:
                    thisCalendar.mEtagCache[aItem.id] = serverEtag;
                    break;
            }
        }

        listener.onOperationDetail = function OOD(aStatusCode, aResource,
                                                  aOperation, aDetail,
                                                  aClosure) {
            var props = aDetail.QueryInterface(Ci.nsIProperties);
            serverEtag = props.get("DAV: getetag",
                                   Ci.nsISupportsString).toString();
        }

        var webdavSvc = Cc['@mozilla.org/webdav/service;1'].
                        getService(Ci.nsIWebDAVService);
        webdavSvc.getResourceProperties(itemResource, 1, ["DAV: getetag"],
                                        false, listener, this, null);
    },

    promptOverwrite: function caldavPO(aMethod, aItem, aListener, aOldItem) {
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                            getService(Ci.nsIPromptService);

        var promptTitle = calGetString("calendar", "itemModifiedOnServerTitle");
        var promptMessage = calGetString("calendar", "itemModifiedOnServer");
        var buttonLabel1;

        if (aMethod == CALDAV_MODIFY_ITEM) {
            promptMessage += calGetString("calendar", "modifyWillLoseData");
            buttonLabel1 = calGetString("calendar", "proceedModify");
        } else {
            promptMessage += calGetString("calendar", "deleteWillLoseData");
            buttonLabel1 = calGetString("calendar", "proceedDelete");
        }
        
        var buttonLabel2 = calGetString("calendar", "updateFromServer");

        var flags = promptService.BUTTON_TITLE_IS_STRING *
                    promptService.BUTTON_POS_0 +
                    promptService.BUTTON_TITLE_IS_STRING *
                    promptService.BUTTON_POS_1;

        var choice = promptService.confirmEx(null, promptTitle, promptMessage,
                                             flags, buttonLabel1, buttonLabel2,
                                             null, null, {});

        if (choice == 0) {
            if (aMethod == CALDAV_MODIFY_ITEM) {
                this.performModifyItem(aItem, aOldItem, aListener);
            } else {
                this.performDeleteItem(aItem, aListener);
            }
        } else {
            this.getUpdatedItem(aItem, aListener);
        }

    },

    mEtagCache: Array(),

    /**
     * addItem() is required by the IDL, but simply calls adoptItem().
     * Actually adding the item to the CalDAV store takes place in
     * performAdoptItem().
     *
     * @param aItem       item to check
     * @param aListener   listener for method completion
     */
    addItem: function caldavAI(aItem, aListener) {
        var newItem = aItem.clone();
        return this.adoptItem(newItem, aListener);
    },
    
    /**
     * Sends data regarding the requested new item off for etag checking.
     *
     * @param aItem       item to check
     * @param aListener   listener for method completion
     */
    adoptItem: function caldavAI2(aItem, aListener) {
        this.fetchEtag(CALDAV_ADOPT_ITEM, aItem, aListener, null);
    },

    /**
     * Performs the actual addition of the item to CalDAV store, after etag
     * checking.
     *
     * @param aItem       item to check
     * @param aListener   listener for method completion
     */
    performAdoptItem: function caldavPAI(aItem, aListener) {
        if (aItem.id == null && aItem.isMutable) {
            aItem.id = getUUID();
        }

        if (aItem.id == null) {
            if (aListener)
                aListener.onOperationComplete (this,
                                               Components.results.NS_ERROR_FAILURE,
                                               aListener.ADD,
                                               aItem.id,
                                               "Can't set ID on non-mutable item to addItem");
            return;
        }

        // XXX how are we REALLY supposed to figure this out?
        var locationPath = aItem.id + ".ics";
        var itemUri = this.mCalendarUri.clone();
        itemUri.spec = this.mMakeUri(locationPath);
        LOG("itemUri.spec = " + itemUri.spec);
        var eventResource = new WebDavResource(itemUri);

        var listener = new WebDavListener();
        var thisCalendar = this;
        listener.onOperationComplete = 
        function onPutComplete(aStatusCode, aResource, aOperation, aClosure) {

            // 201 = HTTP "Created"
            //
            if (aStatusCode == 201) {
                LOG("Item added successfully");

                var retVal = Components.results.NS_OK;
                // CalDAV does not require that the etag returned on PUT will
                // be identical to the etag returned on later operations
                // (CalDAV 5.3.4), so we best re-query.
                thisCalendar.fetchEtag(CALDAV_CACHE_ETAG, aItem, null, null);

            } else if (aStatusCode == 200) {
                LOG("CalDAV: 200 received from server: server malfunction");
                retVal = Components.results.NS_ERROR_FAILURE;
            } else {
                if (aStatusCode > 999) {
                    aStatusCode = "0x" + aStatusCode.toString(16);
                }

                // XXX real error handling
                LOG("Error adding item: " + aStatusCode);
                retVal = Components.results.NS_ERROR_FAILURE;
            }

            // notify the listener
            if (aListener) {
                try {
                    aListener.onOperationComplete(thisCalendar,
                                                  retVal,
                                                  aListener.ADD,
                                                  aItem.id,
                                                  aItem);
                } catch (ex) {
                    LOG("addItem's onOperationComplete threw an exception "
                          + ex + "; ignoring");
                }
            }

            // notify observers
            if (Components.isSuccessCode(retVal)) {
                thisCalendar.observeAddItem(aItem);
            }
        }
  
        aItem.calendar = this;
        aItem.generation = 1;
        aItem.setProperty("X-MOZ-LOCATIONPATH", locationPath);
        aItem.makeImmutable();

        LOG("icalString = " + aItem.icalString);

        // XXX use if not exists
        // do WebDAV put
        var webSvc = Components.classes['@mozilla.org/webdav/service;1']
            .getService(Components.interfaces.nsIWebDAVService);
        webSvc.putFromString(eventResource, "text/calendar; charset=utf-8",
                             aItem.icalString, listener, this, null);

        return;
    },

    /**
     * Sends info about the request to modify an item off for etag checking.
     *
     * @param aItem       item to check
     * @param aOldItem    aOldItem argument in modifyItem requests
     * @param aListener   listener from original request
     */
    modifyItem: function caldavMI(aItem, aOldItem, aListener) {
        this.fetchEtag(CALDAV_MODIFY_ITEM, aItem, aListener, aOldItem);
    },

    /**
     * Modifies existing item in CalDAV store.
     *
     * @param aItem       item to check
     * @param aOldItem    previous version of item to be modified
     * @param aListener   listener from original request
     */
    performModifyItem: function caldavPMI(aNewItem, aOldItem, aListener) {

        if (aNewItem.id == null) {

            // XXXYYY fix to match iface spec
            // this is definitely an error
            if (aListener) {
                try {
                    aListener.onOperationComplete(this,
                                                  Components.results.NS_ERROR_FAILURE,
                                                  aListener.MODIFY,
                                                  aItem.id,
                                                  "ID for modifyItem doesn't exist or is null");
                } catch (ex) {
                    LOG("modifyItem's onOperationComplete threw an"
                          + " exception " + ex + "; ignoring");
                }
            }

            return;
        }

        if (aNewItem.parentItem != aNewItem) {
            aNewItem.parentItem.recurrenceInfo.modifyException(aNewItem);
            aNewItem = aNewItem.parentItem;
        }

        var eventUri = this.mCalendarUri.clone();
        try {
            eventUri.spec = this.mMakeUri(aNewItem.getProperty("X-MOZ-LOCATIONPATH"));
            LOG("using X-MOZ-LOCATIONPATH: " + eventUri.spec);
        } catch (ex) {
            // XXX how are we REALLY supposed to figure this out?
            eventUri.spec = this.mMakeUri(aNewItem.id + ".ics");
        }

        // It seems redundant to use generation when we have etags
        // but until the idl is changed we do it.
        if (aOldItem.parentItem.generation != aNewItem.generation) {
            if (aListener) {
                aListener.onOperationComplete (this.calendarToReturn,
                                               Components.results.NS_ERROR_FAILURE,
                                               aListener.MODIFY,
                                               aNewItem.id,
                                               "generation mismatch in modifyItem");
            }
            return;
        }

        aNewItem.generation += 1;

        var eventResource = new WebDavResource(eventUri);

        var listener = new WebDavListener();
        var thisCalendar = this;

        const icssvc = Cc["@mozilla.org/calendar/ics-service;1"].
                       getService(Components.interfaces.calIICSService);
        var modifiedItem = icssvc.createIcalComponent("VCALENDAR");
        modifiedItem.prodid = "-//Mozilla Calendar//NONSGML Sunbird//EN";
        modifiedItem.version = "2.0";
        modifiedItem.addSubcomponent(aNewItem.icalComponent);
        if (aNewItem.recurrenceInfo) {
            var exceptions = aNewItem.recurrenceInfo.getExceptionIds({});
            for each (var exc in exceptions) {
                modifiedItem.addSubcomponent(aNewItem.recurrenceInfo.getExceptionFor(exc, true).icalComponent);
            }
        }
        var modifiedItemICS = modifiedItem.serializeToICS();

        listener.onOperationComplete = function(aStatusCode, aResource,
                                                aOperation, aClosure) {
            // 201 = HTTP "Created"
            // 204 = HTTP "No Content"
            //
            if (aStatusCode == 204 || aStatusCode == 201) {
                LOG("Item modified successfully.");
                var retVal = Components.results.NS_OK;
                // CalDAV does not require that the etag returned on PUT will
                // be identical to the etag returned on later operations
                // (CalDAV 5.3.4), so we best re-query.
                thisCalendar.fetchEtag(CALDAV_CACHE_ETAG, aNewItem, null, null);

            } else {
                if (aStatusCode > 999) {
                    aStatusCode = "0x " + aStatusCode.toString(16);
                }
                LOG("Error modifying item: " + aStatusCode);

                // XXX deal with non-existent item here, other
                // real error handling

                // XXX aStatusCode will be 201 Created for a PUT on an item
                // that didn't exist before.

                retVal = Components.results.NS_ERROR_FAILURE;
            }

            // XXX ensure immutable version returned
            // notify listener
            if (aListener) {
                try {
                    aListener.onOperationComplete(thisCalendar, retVal,
                                                  aListener.MODIFY,
                                                  aNewItem.id, aNewItem);
                } catch (ex) {
                    LOG("modifyItem's onOperationComplete threw an"
                          + " exception " + ex + "; ignoring");
                }
            }

            // notify observers
            if (Components.isSuccessCode(retVal)) {
                thisCalendar.observeModifyItem(aNewItem, aOldItem.parentItem);
            }

            return;
        }

        // XXX use if-exists stuff here
        // XXX use etag as generation
        // do WebDAV put
        LOG("modifyItem: aNewItem.icalString = " + aNewItem.icalString);
        var webSvc = Components.classes['@mozilla.org/webdav/service;1']
            .getService(Components.interfaces.nsIWebDAVService);
        webSvc.putFromString(eventResource, "text/calendar; charset=utf-8",
                             modifiedItemICS, listener, this, null);

        return;
    },

    /**
     * Sends data regarding requested deletion off for etag checking.
     *
     * @param aItem       item to check
     * @param aListener   listener for method completion
     */
    deleteItem: function caldavDI(aItem, aListener) {
        this.fetchEtag(CALDAV_DELETE_ITEM, aItem, aListener, null);
    },

    /**
     * Deletes item from CalDAV store.
     *
     * @param aItem       item to delete
     * @param aListener   listener for method completion
     */
    performDeleteItem: function caldavPDI(aItem, aListener) {

        if (aItem.id == null) {
            if (aListener)
                aListener.onOperationComplete (this,
                                               Components.results.NS_ERROR_FAILURE,
                                               aListener.DELETE,
                                               aItem.id,
                                               "ID doesn't exist for deleteItem");
            return;
        }

        var eventUri = this.mCalendarUri.clone();
        try {
            eventUri.spec = this.mMakeUri(aItem.getProperty("X-MOZ-LOCATIONPATH"));
            LOG("using X-MOZ-LOCATIONPATH: " + eventUri.spec);
        } catch (ex) {
            // XXX how are we REALLY supposed to figure this out?
            eventUri.spec = this.mMakeUri(aItem.id + ".ics");
        }

        var eventResource = new WebDavResource(eventUri);

        var listener = new WebDavListener();
        var thisCalendar = this;

        listener.onOperationComplete = 
        function onOperationComplete(aStatusCode, aResource, aOperation,
                                     aClosure) {

            // 204 = HTTP "No content"
            //
            if (aStatusCode == 204) {
                delete thisCalendar.mEtagCache[aItem.id];
                LOG("Item deleted successfully.");
                var retVal = Components.results.NS_OK;
            } else {
                LOG("Error deleting item: " + aStatusCode);
                // XXX real error handling here
                retVal = Components.results.NS_ERROR_FAILURE;
            }

            // notify the listener
            if (aListener) {
                try {
                    aListener.onOperationComplete(thisCalendar,
                                                  Components.results.NS_OK,
                                                  aListener.DELETE,
                                                  aItem.id,
                                                  null);
                } catch (ex) {
                    LOG("deleteItem's onOperationComplete threw an"
                          + " exception " + ex + "; ignoring");
                }
            }

            // notify observers
            if (Components.isSuccessCode(retVal)) {
                thisCalendar.observeDeleteItem(aItem);
            }
        }

        // XXX check generation
        // do WebDAV remove
        var webSvc = Components.classes['@mozilla.org/webdav/service;1']
            .getService(Components.interfaces.nsIWebDAVService);
        webSvc.remove(eventResource, listener, this, null);

        return;
    },

    /**
     * Retrieves a specific item from the CalDAV store.
     * Use when an outdated copy of the item is in hand.
     *
     * @param aItem       item to fetch
     * @param aListener   listener for method completion
     */
    getUpdatedItem: function caldavGUI(aItem, aListener) {
        if (!aListener) {
            return;
        }

        if (aItem == null) {
            aListener.onOperationComplete(this,
                                          Components.results.NS_ERROR_FAILURE,
                                          aListener.GET,
                                          null,
                                          "passed in null item");
            return;
        }

        var itemType = "VEVENT";
        if (aItem instanceof Ci.calITodo) {
            itemType = "VTODO";
        }

        var C = new Namespace("C", "urn:ietf:params:xml:ns:caldav");
        var D = new Namespace("D", "DAV:");
        default xml namespace = C;

        queryXml =
          <calendar-query xmlns:D="DAV:">
            <D:prop>
              <D:getetag/>
              <calendar-data/>
            </D:prop>
            <filter>
              <comp-filter name="VCALENDAR">
                <comp-filter name={itemType}>
                  <prop-filter name="UID">
                    <text-match collation="i;octet">
                      {aItem.id}
                    </text-match>
                  </prop-filter>
                </comp-filter>
              </comp-filter>
            </filter>
          </calendar-query>;

        this.reportInternal(xmlHeader + queryXml.toXMLString(),
                            false, null, null, 1, aListener, aItem);
        return;

    },

    // void getItem( in string id, in calIOperationListener aListener );
    getItem: function (aId, aListener) {

        if (!aListener)
            return;

        if (aId == null) {
            aListener.onOperationComplete(this,
                                          Components.results.NS_ERROR_FAILURE,
                                          aListener.GET,
                                          null,
                                          "passed in empty iid");
            return;
        }


        // this is our basic search-by-uid xml
        // XXX get rid of vevent filter?
        // XXX need a prefix in the namespace decl?
        default xml namespace = "urn:ietf:params:xml:ns:caldav";
        var D = new Namespace("D", "DAV:");
        queryXml = 
          <calendar-query xmlns:D="DAV:">
            <D:prop>
              <calendar-data/>
            </D:prop>
            <filter>
              <comp-filter name="VCALENDAR">
                <comp-filter name="VEVENT">
                  <prop-filter name="UID">
                    <text-match caseless="no">
                      {aId}
                    </text-match>
                  </prop-filter>
                </comp-filter>
              </comp-filter>
            </filter>
          </calendar-query>;

        this.reportInternal(xmlHeader + queryXml.toXMLString(), 
                            false, null, null, 1, aListener);
        return;
    },

    reportInternal: function caldavRI(aQuery, aOccurrences, aRangeStart,
                                      aRangeEnd, aCount, aListener, aItem)
    {
        var reportListener = new WebDavListener();
        var count = 0;  // maximum number of hits to return
        var thisCalendar = this; // need to access from inside the callback

        reportListener.onOperationDetail = function(aStatusCode, aResource,
                                                    aOperation, aDetail,
                                                    aClosure) {
            var rv;
            var errString;

            // is this detail the entire search result, rather than a single
            // detail within a set?
            //
            if (aResource.path == calendarDirUri.path) {
                // XXX is this even valid?  what should we do here?
                // XXX if it's an error, it might be valid?
                throw("XXX report result for calendar, not event\n");
            }

            var items = null;

            // XXX need to do better than looking for just 200
            if (aStatusCode == 200) {

                // we've already called back the maximum number of hits, so 
                // we're done here.
                // 
                if (aCount && count >= aCount) {
                    return;
                }
                ++count;

                // aDetail is the response element from the multi-status
                // XXX try-catch
                var xSerializer = Components.classes
                    ['@mozilla.org/xmlextras/xmlserializer;1']
                    .getService(Components.interfaces.nsIDOMSerializer);
                // libical needs to see \r\n instead on \n\n in the case of "folded" lines
                var response = xSerializer.serializeToString(aDetail).replace(/\n\n/g, "\r\n");
                var responseElement = new XML(response);

                // create calIItemBase from e4x object
                // XXX error-check that we only have one result, etc
                var C = new Namespace("urn:ietf:params:xml:ns:caldav");
                var D = new Namespace("DAV:");

                var etag = responseElement..D::["getetag"];

                // cause returned data to be parsed into the item
                var calData = responseElement..C::["calendar-data"];
                if (!calData.toString().length) {
                  Components.utils.reportError(
                    "Empty or non-existent <calendar-data> element returned" +
                    " by CalDAV server for URI <" + aResource.spec +
                    ">; ignoring");
                  return;
                }
                LOG("item result = \n" + calData);
                this.mICSService = Cc["@mozilla.org/calendar/ics-service;1"].
                                   getService(Components.interfaces.calIICSService);
                var rootComp = this.mICSService.parseICS(calData);

                var calComp;
                if (rootComp.componentType == 'VCALENDAR') {
                    calComp = rootComp;
                } else {
                    calComp = rootComp.getFirstSubcomponent('VCALENDAR');
                }

                var unexpandedItems = [];
                var uid2parent = {};
                var excItems = [];

                while (calComp) {
                    // Get unknown properties
                    var prop = calComp.getFirstProperty("ANY");
                    while (prop) {
                        thisCalendar.unmappedProperties.push(prop);
                        prop = calComp.getNextProperty("ANY");
                    }

                    var subComp = calComp.getFirstSubcomponent("ANY");
                    while (subComp) {
                        // Place each subcomp in a try block, to hopefully get as
                        // much of a bad calendar as possible
                        try {
                            var item = null;
                            switch (subComp.componentType) {
                            case "VEVENT":
                                item = Cc["@mozilla.org/calendar/event;1"].
                                       createInstance(Components.interfaces.calIEvent);
                                break;
                            case "VTODO":
                                item = Cc["@mozilla.org/calendar/todo;1"].
                                       createInstance(Components.interfaces.calITodo);
                                break;
                            case "VTIMEZONE":
                                // we should already have this, so there's no need to
                                // do anything with it here.
                                break;
                            default:
                                this.unmappedComponents.push(subComp);
                            }
                            if (item != null) {

                                item.icalComponent = subComp;
                                // save the location name in case we need to modify
                                // need to build using thisCalendar since aResource.spec
                                // won't contain any auth info embedded in the URI
                                var locationPath = decodeURIComponent(aResource.path)
                                                   .substr(thisCalendar.mLocationPath.length);
                                item.setProperty("X-MOZ-LOCATIONPATH", locationPath);

                                var rid = item.recurrenceId;
                                if (rid == null) {
                                    unexpandedItems.push( item );
                                    if (item.recurrenceInfo != null) {
                                        uid2parent[item.id] = item;
                                    }
                                } else {
                                    item.calendar = thisCalendar;
                                    // force no recurrence info so we can
                                    // rebuild it cleanly below
                                    item.recurrenceInfo = null;
                                    excItems.push(item);
                                }
                            }
                        } catch (ex) { 
                            this.mObserver.onError(ex.result, ex.toString());
                        }
                        subComp = calComp.getNextSubcomponent("ANY");
                    }
                    calComp = rootComp.getNextSubcomponent('VCALENDAR');
                }

                // tag "exceptions", i.e. items with rid:
                for each (var item in excItems) {
                    var parent = uid2parent[item.id];
                    if (parent == null) {
                        LOG( "no parent item for rid=" + item.recurrenceId );
                    } else {
                        item.parentItem = parent;
                        item.parentItem.recurrenceInfo.modifyException(item);
                    }
                }
                // if we loop over both excItems and unexpandedItems using 'item'
                // we can be confident that 'item' means something below
                for each (var item in unexpandedItems) {
                    item.calendar = thisCalendar;
                }

                thisCalendar.mEtagCache[item.id] = etag;
                if (aItem) {
                    // if aItem is not null, we were called from
                    // getUpdatedItem(), and the view isn't listening to any
                    // changes. So in order to have the updated item displayed
                    // we need to modify the item currently displayed with
                    // the one just fetched
                    thisCalendar.observeModifyItem(item, aItem.parentItem);
                }

                // figure out what type of item to return
                var iid;
                if(aOccurrences) {
                    iid = Ci.calIItemBase;
                    if (item.recurrenceInfo) {
                        LOG("ITEM has recurrence: " + item + " (" + item.title + ")");
                        LOG("rangestart: " + aRangeStart.jsDate + " -> " + aRangeEnd.jsDate);
                        // XXX does getOcc call makeImmutable?
                        items = item.recurrenceInfo.getOccurrences(aRangeStart,
                                                                   aRangeEnd,
                                                                   0, {});
                    } else {
                        // XXX need to make occurrences immutable?
                        items = [ item ];
                    }
                    rv = Components.results.NS_OK;
                } else if (item instanceof Ci.calIEvent) {
                    iid = Ci.calIEvent;
                    rv = Components.results.NS_OK;
                    items = [ item ];
                } else if (item instanceof Ci.calITodo) {
                    iid = Ci.calITodo;
                    rv = Components.results.NS_OK;
                    items = [ item ];
                } else {
                    errString = "Can't deduce item type based on query";
                    rv = Components.results.NS_ERROR_FAILURE;
                }

            } else { 
                // XXX
                LOG("aStatusCode = " + aStatusCode);
                errString = "XXX";
                rv = Components.results.NS_ERROR_FAILURE;
            }

            // XXX  handle aCount
            if (errString) {
                LOG("errString = " + errString);
            }

            try {
                aListener.onGetResult(thisCalendar, rv, iid, null,
                                      items ? items.length : 0,
                                      errString ? errString : items);
            } catch (ex) {
                    LOG("reportInternal's onGetResult threw an"
                          + " exception " + ex + "; ignoring");
            }

            // We have a result, so we must be authenticated
            if (thisCalendar.mAuthenticationStatus == kCaldavFirstRequestSent) {
                thisCalendar.mAuthenticationStatus = kCaldavFreshlyAuthenticated;
            }

            if (thisCalendar.mAuthenticationStatus == kCaldavFreshlyAuthenticated) {
                thisCalendar.mAuthenticationStatus = kCaldavAuthenticated;
                while (thisCalendar.mPendingStartupRequests.length > 0) {
                    thisCalendar.popStartupRequest();
                }
            }
            return;
        };

        reportListener.onOperationComplete = function(aStatusCode, aResource,
                                                      aOperation, aClosure) {
            
            // XXX test that something reasonable happens w/notfound

            // parse aStatusCode
            var rv;
            var errString;
            if (aStatusCode == 200) { // XXX better error checking
                rv = Components.results.NS_OK;
            } else {
                rv = Components.results.NS_ERROR_FAILURE;
                errString = "XXX something bad happened";
            }

            // call back the listener
            try {
                if (aListener) {
                    aListener.onOperationComplete(thisCalendar,
                                                  Components.results.
                                                  NS_ERROR_FAILURE,
                                                  aListener.GET, null,
                                                  errString);
                }
            } catch (ex) {
                    LOG("reportInternal's onOperationComplete threw an"
                          + " exception " + ex + "; ignoring");
            }

            return;
        };

        // convert this into a form the WebDAV service can use
        var xParser = Components.classes['@mozilla.org/xmlextras/domparser;1']
                      .getService(Components.interfaces.nsIDOMParser);
        queryDoc = xParser.parseFromString(aQuery, "application/xml");

        // construct the resource we want to search against
        var calendarDirUri = this.mCalendarUri.clone();
        calendarDirUri.spec = this.mMakeUri('');
        LOG("report uri = " + calendarDirUri.spec);
        var calendarDirResource = new WebDavResource(calendarDirUri);

        var webSvc = Components.classes['@mozilla.org/webdav/service;1']
            .getService(Components.interfaces.nsIWebDAVService);
        webSvc.report(calendarDirResource, queryDoc, true, reportListener,
                      this, null);
        return;    

    },


    // void getItems( in unsigned long aItemFilter, in unsigned long aCount, 
    //                in calIDateTime aRangeStart, in calIDateTime aRangeEnd,
    //                in calIOperationListener aListener );
    getItems: function (aItemFilter, aCount, aRangeStart, aRangeEnd, aListener)
    {
        if (!aListener) {
            return;
        }

        if (this.mAuthenticationStatus == kCaldavNoAuthentication) {
           this.mAuthenticationStatus = kCaldavFirstRequestSent;
           this.checkDavResourceType();
        }

        if (this.mAuthenticationStatus == kCaldavFirstRequestSent) {
            var req = new Array(aItemFilter, aCount, aRangeStart, aRangeEnd, aListener);
            this.mPendingStartupRequests.push(req);
            return;
        }

        // this is our basic report xml
        var C = new Namespace("C", "urn:ietf:params:xml:ns:caldav");
        var D = new Namespace("D", "DAV:");
        default xml namespace = C;

        var queryXml = 
          <calendar-query xmlns:D={D}>
            <D:prop>
              <D:getetag/>
              <calendar-data/>
            </D:prop>
            <filter>
              <comp-filter name="VCALENDAR">
                <comp-filter/>
              </comp-filter>
            </filter>
          </calendar-query>;

        // figure out 
        var compFilterNames = new Array();
        compFilterNames[calICalendar.ITEM_FILTER_TYPE_TODO] = "VTODO";
        compFilterNames[calICalendar.ITEM_FILTER_TYPE_VJOURNAL] = "VJOURNAL";
        compFilterNames[calICalendar.ITEM_FILTER_TYPE_EVENT] = "VEVENT";

        var filterTypes = 0;
        for (var i in compFilterNames) {
            if (aItemFilter & i) {
                // XXX CalDAV only allows you to ask for one filter-type
                // at once, so if the caller wants multiple filtern
                // types, all they're going to get for now is events
                // (since that's last in the Array).  Sorry!
                ++filterTypes;
                queryXml[0].C::filter.C::["comp-filter"]
                    .C::["comp-filter"] = 
                    <comp-filter name={compFilterNames[i]}/>;
            }
        }

        if (filterTypes < 1) {
            LOG("No item types specified");
            // XXX should we just quietly call back the completion method?
            throw NS_ERROR_FAILURE;
        }

        // if a time range has been specified, do the appropriate restriction.
        // XXX express "end of time" in caldav by leaving off "start", "end"
        if (aRangeStart && aRangeStart.isValid && 
            aRangeEnd && aRangeEnd.isValid) {

            var queryRangeStart = aRangeStart.clone();
            var queryRangeEnd = aRangeEnd.clone();
            queryRangeStart.isDate = false;
            if (queryRangeEnd.isDate) {
                // add a day to rangeEnd since we want to match events all that day
                // and isDate=false is converting the date to midnight
                queryRangeEnd.day++;
                queryRangeEnd.normalize();
                queryRangeEnd.isDate = false;
            }
            var rangeXml = <time-range start={queryRangeStart.getInTimezone("UTC").icalString}
                                       end={queryRangeEnd.getInTimezone("UTC").icalString}/>;

            // append the time-range as a child of our innermost comp-filter
            queryXml[0].C::filter.C::["comp-filter"]
                .C::["comp-filter"].appendChild(rangeXml);
        }

        var queryString = xmlHeader + queryXml.toXMLString();
        LOG("getItems(): querying CalDAV server for events: \n" + queryString);

        var occurrences = (aItemFilter &
                           calICalendar.ITEM_FILTER_CLASS_OCCURRENCES) != 0; 
        this.reportInternal(queryString, occurrences, aRangeStart, aRangeEnd,
                            aCount, aListener, null);
    },

    startBatch: function ()
    {
        this.observeBatchChange(true);
    },
    endBatch: function ()
    {
        this.observeBatchChange(false);
    },

    // nsIInterfaceRequestor impl
    getInterface: function(iid) {
        if (iid.equals(Components.interfaces.nsIAuthPrompt)) {
            return new calAuthPrompt();
        }
        else if (iid.equals(Components.interfaces.nsIPrompt)) {
            // use the window watcher service to get a nsIPrompt impl
            return Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                             .getService(Components.interfaces.nsIWindowWatcher)
                             .getNewPrompter(null);
        } else if (iid.equals(Components.interfaces.nsIProgressEventSink)) {
            return this;
        // Needed for Lightning on branch vvv
        } else if (iid.equals(Components.interfaces.nsIDocShellTreeItem)) {
            return this;
        } else if (iid.equals(Components.interfaces.nsIAuthPromptProvider)) {
            return Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                             .getService(Components.interfaces.nsIWindowWatcher)
                             .getNewPrompter(null);
        } else if (!isOnBranch && iid.equals(Components.interfaces.nsIAuthPrompt2)) {
            return new calAuthPrompt();
        }
        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    //
    // Helper functions
    //
    observeBatchChange: function observeBatchChange (aNewBatchMode) {
        for each (obs in this.mObservers) {
            if (aNewBatchMode) {
                try {
                    obs.onStartBatch();
                } catch (ex) {
                    LOG("observer's onStartBatch threw an exception " + ex 
                          + "; ignoring");
                }
            } else {
                try {
                    obs.onEndBatch();
                } catch (ex) {
                    LOG("observer's onEndBatch threw an exception " + ex 
                          + "; ignoring");
                }
            }
        }
        return;
    },

    observeAddItem: 
        function observeAddItem(aItem) {
            for each (obs in this.mObservers) {
                try {
                    obs.onAddItem(aItem);
                } catch (ex) {
                    LOG("observer's onAddItem threw an exception " + ex 
                          + "; ignoring");
                }
            }
            return;
        },

    observeModifyItem: 
        function observeModifyItem(aNewItem, aOldItem) {
            for each (obs in this.mObservers) {
                try {
                    obs.onModifyItem(aNewItem, aOldItem);
                } catch (ex) {
                    LOG("observer's onModifyItem threw an exception " + ex 
                          + "; ignoring");
                }
            }
            return;
        },

    observeDeleteItem: 
        function observeDeleteItem(aDeletedItem) {
            for each (obs in this.mObservers) {
                try {
                    obs.onDeleteItem(aDeletedItem);
                } catch (ex) {
                    LOG("observer's onDeleteItem threw an exception " + ex 
                          + "; ignoring");
                }
            }
            return;
        },

    popStartupRequest: function popStartupRequest() {
        var req = this.mPendingStartupRequests.pop();
        this.getItems(req[0], req[1], req[2], req[3], req[4]);
    },

    /**
     * Checks that the calendar URI exists and is a CalDAV calendar
     *
     */
    checkDavResourceType: function checkDavResourceType() {
        var listener = new WebDavListener();
        var resourceTypeXml = null;
        var resourceType = kDavResourceTypeNone;
        var thisCalendar = this;
        listener.onOperationComplete =
            function checkDavResourceType_oOC(aStatusCode, aResource,
                                              aOperation, aClosure) {

            if (resourceType == null || resourceType == kDavResourceTypeNone) {
                thisCalendar.mReadOnly = true;
                throw("The resource at " + thisCalendar.mUri.spec +
                      " is either not DAV or not available\n");
            }

            if (resourceType == kDavResourceTypeCollection) {
                thisCalendar.mReadOnly = true;
                throw("The resource at " + thisCalendar.mUri.spec +
                      " is a DAV collection but not a CalDAV calendar\n");
            }

            // we've authenticated in the process of PROPFINDing and can flush
            // the getItems request queue
            thisCalendar.mAuthenticationStatus = kCaldavFreshlyAuthenticated;
            if (thisCalendar.mPendingStartupRequests.length > 0) {
                thisCalendar.popStartupRequest();
            }
        }

        listener.onOperationDetail =
            function checkDavResourceType_oOD(aStatusCode, aResource,
                                              aOperation, aDetail, aClosure) {

            var prop = aDetail.QueryInterface(Ci.nsIProperties);

            try {
                resourceTypeXml = prop.get("DAV: resourcetype",
                                           Ci.nsISupportsString).toString();
            } catch (ex) {
                LOG("error " + e + " fetching resource type");
            }

            if (resourceTypeXml.length == 0) {
                resourceType = kDavResourceTypeNone;
            } else if (resourceTypeXml.indexOf("calendar") != -1) {
                resourceType = kDavResourceTypeCalendar;
            } else if (resourceTypeXml.indexOf("collection") != -1) {
                resourceType = kDavResourceTypeCollection;
            }
        }

        var res = new WebDavResource(this.mCalendarUri);
        var webSvc = Cc['@mozilla.org/webdav/service;1'].
                     getService(Ci.nsIWebDAVService);
        try {
            webSvc.getResourceProperties(res, 1, ["DAV: resourcetype"], false,
                                          listener, this, null);
        } catch (ex) {
            thisCalendar.mReadOnly = true;
            Components.utils.reportError(
                "Unable to get properties of resource " + thisCalendar.mUri.spec
                + " (not a network resource?); setting read-only");
        }
    },

    refresh: function calDAV_refresh() {
        // XXX-fill this in, get a report for modifications+onModifyItem
    },
    
    // stubs to keep callbacks we don't support yet from throwing errors 
    // we don't care about
    // nsIProgressEventSink
    onProgress: function onProgress(aRequest, aContext, aProgress, aProgressMax) {},
    onStatus: function onStatus(aRequest, aContext, aStatus, aStatusArg) {},
    // nsIDocShellTreeItem
    findItemWithName: function findItemWithName(name, aRequestor, aOriginalRequestor) {}
};

function WebDavResource(url) {
    this.mResourceURL = url;
}

WebDavResource.prototype = {
    mResourceURL: {},
    get resourceURL() { 
        return this.mResourceURL;}  ,
    QueryInterface: function(iid) {
        if (iid.equals(CI.nsIWebDAVResource) ||
            iid.equals(CI.nsISupports)) {
            return this;
        }
       
        throw Components.interfaces.NS_ERROR_NO_INTERFACE;
    }
};

function WebDavListener() {
}

WebDavListener.prototype = { 

    QueryInterface: function (aIID) {
        if (!aIID.equals(Components.interfaces.nsISupports) &&
            !aIID.equals(nsIWebDavOperationListener)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        return this;
    },

    onOperationComplete: function(aStatusCode, aResource, aOperation,
                                  aClosure) {
        // aClosure is the listener
        aClosure.onOperationComplete(this, aStatusCode, 0, null, null);

        LOG("WebDavListener.onOperationComplete() called");
        return;
    },

    onOperationDetail: function(aStatusCode, aResource, aOperation, aDetail,
                                aClosure) {
        LOG("WebDavListener.onOperationDetail() called");
        return;
    }
}

/****
 **** module registration
 ****/

var calDavCalendarModule = {
    mCID: Components.ID("{a35fc6ea-3d92-11d9-89f9-00045ace3b8d}"),
    mContractID: "@mozilla.org/calendar/calendar;1?type=caldav",

    mUtilsLoaded: false,
    loadUtils: function caldavLoadUtils() {
        if (this.mUtilsLoaded)
            return;

        const jssslContractID = "@mozilla.org/moz/jssubscript-loader;1";
        const jssslIID = Components.interfaces.mozIJSSubScriptLoader;

        const iosvcContractID = "@mozilla.org/network/io-service;1";
        const iosvcIID = Components.interfaces.nsIIOService;

        var loader = Components.classes[jssslContractID].getService(jssslIID);
        var iosvc = Components.classes[iosvcContractID].getService(iosvcIID);

        // Note that unintuitively, __LOCATION__.parent == .
        // We expect to find utils in ./../js
        var appdir = __LOCATION__.parent.parent;
        appdir.append("js");
        var scriptName = "calUtils.js";

        var f = appdir.clone();
        f.append(scriptName);

        try {
            var fileurl = iosvc.newFileURI(f);
            loader.loadSubScript(fileurl.spec, this.__parent__.__parent__);
        } catch (e) {
            LOG("Error while loading " + fileurl.spec );
            throw e;
        }

        this.mUtilsLoaded = true;
    },
    
    registerSelf: function (compMgr, fileSpec, location, type) {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.registerFactoryLocation(this.mCID,
                                        "Calendar CalDAV back-end",
                                        this.mContractID,
                                        fileSpec,
                                        location,
                                        type);
    },

    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.mCID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        this.loadUtils();

        return this.mFactory;
    },

    mFactory: {
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;
            return (new calDavCalendar()).QueryInterface(iid);
        }
    },

    canUnload: function(compMgr) {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return calDavCalendarModule;
}
