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
    File Name:          15.8.1.7-2.js
    ECMA Section:       15.8.1.7.js
    Description:        All value properties of the Math object should have
                        the attributes [DontEnum, DontDelete, ReadOnly]

                        this test checks the DontDelete attribute of Math.SQRT1_2

    Author:             christine@netscape.com
    Date:               16 september 1997
*/

    var SECTION = "15.8.1.7-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Math.SQRT1_2";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;
    array[item++] = new TestCase( SECTION, "delete Math.SQRT1_2; Math.SQRT1_2", 0.7071067811865476, "delete Math.SQRT1_2; Math.SQRT1_2" );
    array[item++] = new TestCase( SECTION, "delete Math.SQRT1_2",                false,              "delete Math.SQRT1_2" );
    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].actual = eval( testcases[tc].actual );
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "property should be read-only ";
    }
    stopTest();
    return ( testcases );
}
