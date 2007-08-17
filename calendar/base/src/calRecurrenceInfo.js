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
 * The Original Code is lightning code.
 *
 * The Initial Developer of the Original Code is
 *  Oracle Corporation
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir.vukicevic@oracle.com>
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

function calRecurrenceInfo() {
    this.mRecurrenceItems = new Array();
    this.mExceptions = new Array();
}

function calDebug() {
    dump.apply(null, arguments);
}

var calRecurrenceInfoClassInfo = {
    getInterfaces: function (count) {
        var ifaces = [
            Components.interfaces.nsISupports,
            Components.interfaces.calIRecurrenceInfo,
            Components.interfaces.nsIClassInfo
        ];
        count.value = ifaces.length;
        return ifaces;
    },

    getHelperForLanguage: function (language) {
        return null;
    },

    contractID: "@mozilla.org/calendar/recurrence-info;1",
    classDescription: "Calendar Recurrence Info",
    classID: Components.ID("{04027036-5884-4a30-b4af-f2cad79f6edf}"),
    implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
    flags: 0
};

calRecurrenceInfo.prototype = {
    // QI with CI
    QueryInterface: function(aIID) {
        if (aIID.equals(Components.interfaces.nsISupports) ||
            aIID.equals(Components.interfaces.calIRecurrenceInfo))
            return this;

        if (aIID.equals(Components.interfaces.nsIClassInfo))
            return calRecurrenceInfoClassInfo;

        throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    //
    // Mutability bits
    //
    mImmutable: false,
    get isMutable() { return !this.mImmutable; },
    makeImmutable: function() {
        if (this.mImmutable)
            return;

        for each (ritem in this.mRecurrenceItems) {
            if (ritem.isMutable)
                ritem.makeImmutable();
        }

        for each (ex in this.mExceptions) {
            if (ex.item.isMutable)
                ex.item.makeImmutable();
        }

        this.mImmutable = true;
    },

    clone: function() {
        var cloned = new calRecurrenceInfo();
        cloned.mBaseItem = this.mBaseItem;

        var clonedItems = [];
        for each (ritem in this.mRecurrenceItems)
            clonedItems.push(ritem.clone());
        cloned.mRecurrenceItems = clonedItems;

        var clonedExceptions = [];
        for each (exitem in this.mExceptions) {
            var c = exitem.item.cloneShallow(this.mBaseItem);
            clonedExceptions.push( { id: exitem.id, item: c } );
        }
        cloned.mExceptions = clonedExceptions;

        return cloned;
    },

    //
    // calIRecurrenceInfo impl
    //
    mBaseItem: null,

    get item() {
        return this.mBaseItem;
    },

    set item(value) {
        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        this.mBaseItem = value;
        // patch exception's parentItem:
        for each (exitem in this.mExceptions) {
            exitem.item.parentItem = value;
        }
    },

    mRecurrenceItems: null,

    get isFinite() {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        for each (ritem in this.mRecurrenceItems) {
            if (!ritem.isFinite)
                return false;
        }

        return true;
    },

    getRecurrenceItems: function(aCount) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        aCount.value = this.mRecurrenceItems.length;
        return this.mRecurrenceItems;
    },

    setRecurrenceItems: function(aCount, aItems) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        // should we clone these?
        this.mRecurrenceItems = aItems;
    },

    countRecurrenceItems: function() {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        return this.mRecurrenceItems.length;
    },

    getRecurrenceItemAt: function(aIndex) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (aIndex < 0 || aIndex >= this.mRecurrenceItems.length)
            throw Components.results.NS_ERROR_INVALID_ARG;

        return this.mRecurrenceItems[aIndex];
    },

    appendRecurrenceItem: function(aItem) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        this.mRecurrenceItems.push(aItem);
    },

    deleteRecurrenceItemAt: function(aIndex) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        if (aIndex < 0 || aIndex >= this.mRecurrenceItems.length)
            throw Components.results.NS_ERROR_INVALID_ARG;

        this.mRecurrenceItems.splice(aIndex, 1);
    },

    deleteRecurrenceItem: function(aItem) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        // Because xpcom objects can be wrapped in various ways, testing for
        // mere == sometimes returns false even when it should be true.  Use
        // the interface pointer returned by sip to avoid that problem.
        var sip1 = Components.classes["@mozilla.org/supports-interface-pointer;1"]
                            .createInstance(Components.interfaces.nsISupportsInterfacePointer);
        sip1.data = aItem;
        sip1.dataIID = Components.interfaces.calIRecurrenceItem;
        for (var i = 0; i < this.mRecurrenceItems.length; i++) {
            if (this.mRecurrenceItems[i] == sip1.data) {
                this.deleteRecurrenceItemAt(i);
                return;
            }
        }

        throw Components.results.NS_ERROR_INVALID_ARG;
    },

    insertRecurrenceItemAt: function(aItem, aIndex) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        if (aIndex < 0 || aIndex > this.mRecurrenceItems.length)
            throw Components.results.NS_ERROR_INVALID_ARG;

        this.mRecurrenceItems.splice(aIndex, 0, aItem);
    },

    clearRecurrenceItems: function() {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        this.mRecurrenceItems = new Array();
    },

    //
    // calculations
    //

    getNextOccurrenceDate: function (aTime) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        var startDate = this.mBaseItem.recurrenceStartDate;
        var dates = [];

        for each (ritem in this.mRecurrenceItems) {
            var date = ritem.getNextOccurrence(startDate, aTime);
            if (!date)
                continue;

            if (ritem.isNegative)
                dates = dates.filter(function (d) { return (d.compare(date) != 0); });
            else
                dates.push(date);
        }

        // if no dates, there's no next
        if (dates.length == 0)
            return null;

        // find the earliest date
        var earliestDate = dates[0];
        dates.forEach(function (d) { if (d.compare(earliestDate) < 0) earliestDate = d; });

        return earliestDate;
    },

    getNextOccurrence: function (aTime) {
        var earliestDate = this.getNextOccurrenceDate (aTime);
        if (!earliestDate)
            return null;

        if (this.mExceptions) {
            // scan exceptions for any dates earlier than
            // earliestDate (but still after aTime)
            this.mExceptions.forEach (function (ex) {
                                          var dtstart = ex.item.getProperty("DTSTART");
                                          if (aTime.compare(dtstart) <= 0 &&
                                              earliestDate.compare(dtstart) > 0)
                                          {
                                              earliestDate = dtstart;
                                          }
                                      });
        }

        var startDate = earliestDate.clone();
        var endDate = null;

        if (this.mBaseItem.hasProperty("DTEND")) {
            endDate = earliestDate.clone();
            endDate.addDuration(this.mBaseItem.duration);
        }

        var proxy = this.mBaseItem.createProxy();
        proxy.recurrenceId = earliestDate.clone();

        proxy.setProperty("DTSTART", startDate);
        if (endDate)
            proxy.setProperty("DTEND", endDate);

        return proxy;
    },

    // internal helper function; 
    calculateDates: function (aRangeStart, aRangeEnd,
                              aMaxCount, aReturnRIDs)
    {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        // If aRangeStart falls in the middle of an occurrence, libical will
        // not return that occurrence when we go and ask for an
        // icalrecur_iterator_new.  This actually seems fairly rational, so 
        // instead of hacking libical, I'm going to move aRangeStart back far
        // enough to make sure we get the occurrences we might miss.
        var searchStart = aRangeStart.clone();
        try {
            var duration = this.mBaseItem.duration.clone();
            duration.isNegative = true;
            searchStart.isDate = false; // workaround for UTC+ timezones
            searchStart.addDuration(duration);
        } catch(ex) {
            dump("recurrence tweaking exception:"+ex+'\n');
        }

        // workaround for UTC- timezones
        var rangeEnd = aRangeEnd;
        if (rangeEnd && rangeEnd.isDate) {
            rangeEnd = aRangeEnd.clone();
            rangeEnd.isDate = false;
        }

        var startDate = this.mBaseItem.recurrenceStartDate;
        var dates = [];

        function checkRange(item) {
            var dueDate = null;
            var occDate = (item.getProperty("DTSTART") ||
                           (dueDate = item.getProperty("DUE")));
            if (!occDate) // DTSTART or DUE mandatory
                return null;
            // tasks may have a due date set or no duration at all
            var end = (item.getProperty("DTEND") ||
                       (dueDate ? dueDate : item.getProperty("DUE")) ||
                       occDate);
            // is the item an intersection of the range?
            if ((!aRangeStart || aRangeStart.compare(end) <= 0) &&
                (!aRangeEnd || aRangeEnd.compare(occDate) > 0)) {
                return occDate;
            }
            return null;
        }

        // DTSTART/DUE is always part of the (positive) expanded set:
        // the base item cannot be replaced by an exception;
        // an exception can only be defined on an item resulting from an RDATE/RRULE;
        // DTSTART always equals RECURRENCE-ID for items expanded from RRULE
        var baseOccDate = checkRange(this.mBaseItem);
        if (baseOccDate) {
            dates.push(baseOccDate);
        }

        // toss in exceptions first:
        if (this.mExceptions) {
            this.mExceptions.forEach(
                function(ex) {
                    var occDate = checkRange(ex.item);
                    if (occDate) {
                        dates.push(aReturnRIDs ? ex.id : occDate);
                    }
                });
        }

        // apply positive items before negative:
        var sortedRecurrenceItems = [];
        for each ( var ritem in this.mRecurrenceItems ) {
            if (ritem.isNegative)
                sortedRecurrenceItems.push(ritem);
            else
                sortedRecurrenceItems.unshift(ritem);
        }
        for each (ritem in sortedRecurrenceItems) {
            var cur_dates;

            // if both range start and end are specified, we ask for all of the occurrences,
            // to make sure we catch all possible exceptions.  If aRangeEnd isn't specified,
            // then we have to ask for aMaxCount, and hope for the best.
            var maxCount;
            if (aRangeStart && aRangeEnd) {
                maxCount = 0;
            } else {
                maxCount = aMaxCount;
            }
            cur_dates = ritem.getOccurrences(startDate,
                                             searchStart,
                                             rangeEnd,
                                             maxCount, {});

            if (cur_dates.length == 0)
                continue;

            if (ritem.isNegative) {
                // if this is negative, we look for any of the given dates
                // in the existing set, and remove them if they're
                // present.

                // XXX: i'm pretty sure negative dates can't really have exceptions
                // (like, you can't make a date "real" by defining an RECURRENCE-ID which
                // is an EXDATE, and then giving it a real DTSTART) -- so we don't
                // check exceptions here
                cur_dates.forEach (function (dateToRemove) {
                                       dates = dates.filter(function (d) { return d.compare(dateToRemove) != 0; });
                                   });
            } else {
                // XXX todo: IMO a bug here,
                //           if we are asked for DTSTART dates
                //           (aReturnRIDs is false => getOccurrenceDates),
                //           then pumping in the plain expanded rule dates is wrong,
                //           we need to take the "exception'ed" DTSTART dates

                // if positive, we just add these date to the existing set,
                // but only if they're not already there
                var datesToAdd = [];
                var rinfo = this;
                cur_dates.forEach (function (dateToAdd) {
                                       if (!dates.some(function (d) { return d.compare(dateToAdd) == 0; })) {
                                           dates.push(dateToAdd);
                                       }
                                   });
            }
        }

        // now sort the list
        dates.sort(function (a,b) { return a.compare(b); });

        // chop anything over aMaxCount, if specified
        if (aMaxCount && dates.length > aMaxCount)
            dates = dates.splice(aMaxCount, dates.length - aMaxCount);

        return dates;
    },

    getOccurrenceDates: function (aRangeStart, aRangeEnd,
                                  aMaxCount, aCount)
    {
        var dates = this.calculateDates(aRangeStart, aRangeEnd, aMaxCount, false);
        aCount.value = dates.length;
        return dates;
    },

    getOccurrences: function (aRangeStart, aRangeEnd,
                              aMaxCount,
                              aCount)
    {
        var dates = this.calculateDates(aRangeStart, aRangeEnd, aMaxCount, true);
        if (dates.length == 0) {
            aCount.value = 0;
            return [];
        }

        var count = aMaxCount;
        if (!count)
            count = dates.length;

        var results = [];

        for (var i = 0; i < count; i++) {
            var proxy = this.getOccurrenceFor(dates[i]);
            results.push(proxy);
        }

        aCount.value = results.length;
        return results;
    },

    getOccurrenceFor: function (aRecurrenceId) {
        var proxy = this.getExceptionFor(aRecurrenceId, false);
        if (!proxy) {
            var duration = null;
            
            var name = "DTEND";
            if (this.mBaseItem instanceof Components.interfaces.calITodo)
                name = "DUE";
                
            if (this.mBaseItem.hasProperty(name))
                duration = this.mBaseItem.duration;

            proxy = this.mBaseItem.createProxy();
            proxy.recurrenceId = aRecurrenceId;
            proxy.setProperty("DTSTART", aRecurrenceId.clone());
            if (duration) {
                var enddate = aRecurrenceId.clone();
                enddate.addDuration(duration);
                proxy.setProperty(name, enddate);
            }
            if (!this.mBaseItem.isMutable)
                proxy.makeImmutable();
        }
        return proxy;
    },

    removeOccurrenceAt: function (aRecurrenceId) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        var d = Components.classes["@mozilla.org/calendar/recurrence-date;1"].createInstance(Components.interfaces.calIRecurrenceDate);
        d.isNegative = true;
        d.date = aRecurrenceId.clone();

        this.removeExceptionFor(d.date);

        this.appendRecurrenceItem(d);
    },

    restoreOccurrenceAt: function (aRecurrenceId) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        if (this.mImmutable)
            throw Components.results.NS_ERROR_OBJECT_IS_IMMUTABLE;

        for (var i = 0; i < this.mRecurrenceItems.length; i++) {
            if (this.mRecurrenceItems[i] instanceof Components.interfaces.calIRecurrenceDate) {
                var rd = this.mRecurrenceItems[i].QueryInterface(Components.interfaces.calIRecurrenceDate);
                if (rd.isNegative && rd.date.compare(aRecurrenceId) == 0) {
                    return this.deleteRecurrenceItemAt(i);
                }
            }
        }

        throw Components.results.NS_ERROR_INVALID_ARG;
    },

    //
    // exceptions
    //

    //
    // Some notes:
    //
    // The way I read ICAL, RECURRENCE-ID is used to specify a
    // particular instance of a recurring event, according to the
    // RRULEs/RDATEs/etc. specified in the base event.  If one of
    // these is to be changed ("an exception"), then it can be
    // referenced via the UID of the original event, and a
    // RECURRENCE-ID of the start time of the instance to change.
    // This, to me, means that an event where one of the instances has
    // changed to a different time has a RECURRENCE-ID of the original
    // start time, and a DTSTART/DTEND representing the new time.
    //
    // ITIP, however, seems to want something different -- you're
    // supposed to use UID/RECURRENCE-ID to select from the current
    // set of occurrences of an event.  If you change the DTSTART for
    // an instance, you're supposed to use the old (original) DTSTART
    // as the RECURRENCE-ID, and put the new time as the DTSTART.
    // However, after that change, to refer to that instance in the
    // future, you have to use the modified DTSTART as the
    // RECURRENCE-ID.  This madness is described in ITIP end of
    // section 3.7.1.
    // 
    // This implementation does the first approach (RECURRENCE-ID will
    // never change even if DTSTART for that instance changes), which
    // I think is the right thing to do for CalDAV; I don't know what
    // we'll do for incoming ITIP events though.
    //

    mExceptions: null,

    modifyException: function (anItem) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        // the item must be an occurrence
        if (anItem.parentItem == anItem)
            throw Components.results.NS_ERROR_UNEXPECTED;

        if (anItem.parentItem.calendar != this.mBaseItem.calendar &&
            anItem.parentItem.id != this.mBaseItem.id)
        {
            calDebug ("recurrenceInfo::addException: item parentItem != this.mBaseItem (calendar/id)!\n");
            throw Components.results.NS_ERROR_INVALID_ARG;
        }

        if (anItem.recurrenceId == null) {
            calDebug ("recurrenceInfo::addException: item with null recurrenceId!\n");
            throw Components.results.NS_ERROR_INVALID_ARG;
        }

        var itemtoadd;
        if (anItem.isMutable) {
            itemtoadd = anItem.cloneShallow(this.mBaseItem);
            itemtoadd.makeImmutable();
        } else {
            itemtoadd = anItem;
        }

        // we're going to assume that the recurrenceId is valid here,
        // because presumably the item came from one of our functions

        // remove any old one, if present
        this.removeExceptionFor(anItem.recurrenceId);

        this.mExceptions.push( { id: itemtoadd.recurrenceId, item: itemtoadd } );
    },

    createExceptionFor: function (aRecurrenceId) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        // XX should it be an error to createExceptionFor
        // an already-existing recurrenceId?
        var existing = this.getExceptionFor(aRecurrenceId, false);
        if (existing)
            return existing;

        // check if aRecurrenceId is valid.

        // this is a bit of a hack; we know that ranges are defined as [start, end),
        // so we do a search on aRecurrenceId and aRecurrenceId.seconds + 1.
        var rangeStart = aRecurrenceId;
        var rangeEnd = aRecurrenceId.clone();
        rangeEnd.second += 1;

        var dates = this.getOccurrenceDates (rangeStart, rangeEnd, 1, {});
        var found = false;
        for each (d in dates) {
            if (d.compare(aRecurrenceId) == 0) {
                found = true;
                break;
            }
        }

        // not found; the recurrence id is invalid
        if (!found)
            throw Components.results.NS_ERROR_INVALID_ARG;

        var rid = aRecurrenceId.clone();
        rid.makeImmutable();

        var newex = this.mBaseItem.createProxy();
        newex.recurrenceId = rid;

        this.mExceptions.push({id: rid, item: newex});

        return newex;
    },

    getExceptionFor: function (aRecurrenceId, aCreate) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        for each (ex in this.mExceptions) {
            if (ex.id.compare(aRecurrenceId) == 0)
                return ex.item;
        }

        if (aCreate) {
            return this.createExceptionFor(aRecurrenceId);
        }
        return null;
    },

    removeExceptionFor: function (aRecurrenceId) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        this.mExceptions = this.mExceptions.filter (function(ex) {
                                                        return (ex.id.compare(aRecurrenceId) != 0);
                                                    });
    },

    getExceptionIds: function (aCount) {
        if (!this.mBaseItem)
            throw Components.results.NS_ERROR_NOT_INITIALIZED;

        var ids = this.mExceptions.map (function(ex) {
                                            return ex.id;
                                        });

        aCount.value = ids.length;
        return ids;
    },
    
    // changing the startdate of an item needs to take exceptions into account.
    // in case we're about to modify a parentItem (aka 'folded' item), we need
    // to modify the recurrenceId's of all possibly existing exceptions as well.
    onStartDateChange: function (aNewStartTime, aOldStartTime) {

        // passing null for the new starttime would indicate an error condition,
        // since having a recurrence without a starttime is invalid.
        if (!aNewStartTime) {
            throw Components.results.NS_ERROR_INVALID_ARG;
        }
    
        // no need to check for changes if there's no previous starttime.
        if (!aOldStartTime) {
            return;
        }
    
        // convert both dates to UTC since subtractDate is not timezone aware.
        aOldStartTime = aOldStartTime.getInTimezone("UTC");
        aNewStartTime = aNewStartTime.getInTimezone("UTC");
        var timeDiff = aNewStartTime.subtractDate(aOldStartTime);
        var exceptions = this.getExceptionIds({});
        var modifiedExceptions = [];
        for each (var exid in exceptions) {
            var ex = this.getExceptionFor(exid, false);
            if (ex) {
                if (!ex.isMutable) {
                    ex = ex.cloneShallow(this.item);
                }
                ex.recurrenceId.addDuration(timeDiff);
                
                modifiedExceptions.push(ex);
                this.removeExceptionFor(exid);
            }
        }
        for each (var modifiedEx in modifiedExceptions) {
            this.modifyException(modifiedEx);
        }

        // also take RDATE's and EXDATE's into account.
        const kCalIRecurrenceDate = Components.interfaces.calIRecurrenceDate;
        const kCalIRecurrenceDateSet = Components.interfaces.calIRecurrenceDateSet;
        var ritems = this.getRecurrenceItems({});
        for (var i in ritems) {
            var ritem = ritems[i];
            if (ritem instanceof kCalIRecurrenceDate) {
                ritem = ritem.QueryInterface(kCalIRecurrenceDate);
                ritem.date.addDuration(timeDiff);
            } else if (ritem instanceof kCalIRecurrenceDateSet) {
                ritem = ritem.QueryInterface(kCalIRecurrenceDateSet);
                var rdates = ritem.getDates({});
                for each (var date in rdates) {
                    date.addDuration(timeDiff);
                }
                ritem.setDates(rdates.length,rdates);
            }
        }
    }
};
