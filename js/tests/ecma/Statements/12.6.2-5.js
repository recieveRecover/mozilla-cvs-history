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
    File Name:          12.6.2-5.js
    ECMA Section:       12.6.2 The for Statement

                        1. first expression is not present.
                        2. second expression is present
                        3. third expression is present


    Author:             christine@netscape.com
    Date:               15 september 1997
*/
    var SECTION = "12.6.2-5";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The for statment";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();

    test();

function getTestCases() {
    var array = new Array();
    array[0] = new TestCase( SECTION, "for statement",  99,     "" );
    return ( array );
}

function testprogram() {
    myVar = 0;

    for ( ; myVar < 100 ; myVar++ ) {
        if (myVar == 99)
            break;
    }

    return myVar;
}
function test() {
        testcases[0].actual = testprogram();
        testcases[0].passed = writeTestCaseResult(
                    testcases[0].expect,
                    testcases[0].actual,
                    testcases[0].description +" = "+ testcases[0].actual );

        testcases[0].reason += ( testcases[0].passed ) ? "" : "wrong value ";
        stopTest();
        return ( testcases );
}
