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
    File Name:          15.8.2.10.js
    ECMA Section:       15.8.2.10  Math.log(x)
    Description:        return an approximiation to the natural logarithm of
                        the argument.
                        special cases:
                        -   if arg is NaN       result is NaN
                        -   if arg is <0        result is NaN
                        -   if arg is 0 or -0   result is -Infinity
                        -   if arg is 1         result is 0
                        -   if arg is Infinity  result is Infinity
    Author:             christine@netscape.com
    Date:               7 july 1997
*/

    var SECTION = "15.8.2.10";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.log(x)";
    var BUGNUMBER = "77391";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,   "Math.log.length",         1,              Math.log.length );

    array[item++] = new TestCase( SECTION,   "Math.log()",              Number.NaN,     Math.log() );
    array[item++] = new TestCase( SECTION,   "Math.log(void 0)",        Number.NaN,     Math.log(void 0) );
    array[item++] = new TestCase( SECTION,   "Math.log(null)",          Number.NEGATIVE_INFINITY,   Math.log(null) );
    array[item++] = new TestCase( SECTION,   "Math.log(true)",          0,              Math.log(true) );
    array[item++] = new TestCase( SECTION,   "Math.log(false)",         -Infinity,      Math.log(false) );
    array[item++] = new TestCase( SECTION,   "Math.log('0')",           -Infinity,      Math.log('0') );
    array[item++] = new TestCase( SECTION,   "Math.log('1')",           0,              Math.log('1') );
    array[item++] = new TestCase( SECTION,   "Math.log('Infinity')",    Infinity,       Math.log("Infinity") );

    array[item++] = new TestCase( SECTION,   "Math.log(NaN)",           Number.NaN,     Math.log(Number.NaN) );
    array[item++] = new TestCase( SECTION,   "Math.log(-0.0000001)",    Number.NaN,     Math.log(-0.000001)  );
    array[item++] = new TestCase( SECTION,   "Math.log(-1)",            Number.NaN,     Math.log(-1)        );
    array[item++] = new TestCase( SECTION,   "Math.log(0)",             Number.NEGATIVE_INFINITY,   Math.log(0) );
    array[item++] = new TestCase( SECTION,   "Math.log(-0)",            Number.NEGATIVE_INFINITY,   Math.log(-0));
    array[item++] = new TestCase( SECTION,   "Math.log(1)",             0,              Math.log(1) );
    array[item++] = new TestCase( SECTION,   "Math.log(Infinity)",      Number.POSITIVE_INFINITY,   Math.log(Number.POSITIVE_INFINITY) );
    array[item++] = new TestCase( SECTION,   "Math.log(-Infinity)",     Number.NaN,     Math.log(Number.NEGATIVE_INFINITY) );

    return ( array );
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