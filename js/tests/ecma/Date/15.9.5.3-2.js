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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
    File Name:          15.9.5.3-2.js
    ECMA Section:       15.9.5.3-2 Date.prototype.valueOf
    Description:

    The valueOf function returns a number, which is this time value.

    The valueOf function is not generic; it generates a runtime error if
    its this value is not a Date object.  Therefore it cannot be transferred
    to other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "15.9.5.3-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Date.prototype.valueOf";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();


    var TZ_ADJUST = TZ_DIFF * msPerHour;
    var now = (new Date()).valueOf();
    var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
    var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001)+
    TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

    addTestCase( now );
    addTestCase( TIME_1970 );
    addTestCase( TIME_1900 );
    addTestCase( TIME_2000 );
    addTestCase( UTC_29_FEB_2000 );
    addTestCase( UTC_1_JAN_2005 );

    test();

function addTestCase( t ) {
    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+t+").valueOf()",
                                    t,
                                    (new Date(t)).valueOf() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t+1)+").valueOf()",
                                    t+1,
                                    (new Date(t+1)).valueOf() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t-1)+").valueOf()",
                                    t-1,
                                    (new Date(t-1)).valueOf() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t-TZ_ADJUST)+").valueOf()",
                                    t-TZ_ADJUST,
                                    (new Date(t-TZ_ADJUST)).valueOf() );

    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+(t+TZ_ADJUST)+").valueOf()",
                                    t+TZ_ADJUST,
                                    (new Date(t+TZ_ADJUST)).valueOf() );
}

function MyObject( value ) {
    this.value = value;
    this.valueOf = Date.prototype.valueOf;
    this.toString = new Function( "return this+\"\";");
    return this;
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
