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
    File Name:          11.4.9.js
    ECMA Section:       11.4.9 Logical NOT Operator (!)
    Description:        if the ToBoolean( VALUE ) result is true, return
                        true.  else return false.
    Author:             christine@netscape.com
    Date:               7 july 1997

    Static variables:
        none
*/
    var SECTION = "11.4.9";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Logical NOT operator (!)";

    writeHeaderToLog( SECTION + " "+ TITLE);

//    version("130")

    var testcases = new Array();

    testcases[tc++] = new TestCase( SECTION,   "!(null)",                true,   !(null) );
    testcases[tc++] = new TestCase( SECTION,   "!(var x)",               true,   !(eval("var x")) );
    testcases[tc++] = new TestCase( SECTION,   "!(void 0)",              true,   !(void 0) );

    testcases[tc++] = new TestCase( SECTION,   "!(false)",               true,   !(false) );
    testcases[tc++] = new TestCase( SECTION,   "!(true)",                false,  !(true) );
    testcases[tc++] = new TestCase( SECTION,   "!()",                    true,   !(eval()) );
    testcases[tc++] = new TestCase( SECTION,   "!(0)",                   true,   !(0) );
    testcases[tc++] = new TestCase( SECTION,   "!(-0)",                  true,   !(-0) );
    testcases[tc++] = new TestCase( SECTION,   "!(NaN)",                 true,   !(Number.NaN) );
    testcases[tc++] = new TestCase( SECTION,   "!(Infinity)",            false,  !(Number.POSITIVE_INFINITY) );
    testcases[tc++] = new TestCase( SECTION,   "!(-Infinity)",           false,  !(Number.NEGATIVE_INFINITY) );
    testcases[tc++] = new TestCase( SECTION,   "!(Math.PI)",             false,  !(Math.PI) );
    testcases[tc++] = new TestCase( SECTION,   "!(1)",                   false,  !(1) );
    testcases[tc++] = new TestCase( SECTION,   "!(-1)",                  false,  !(-1) );
    testcases[tc++] = new TestCase( SECTION,   "!('')",                  true,   !("") );
    testcases[tc++] = new TestCase( SECTION,   "!('\t')",                false,  !("\t") );
    testcases[tc++] = new TestCase( SECTION,   "!('0')",                 false,  !("0") );
    testcases[tc++] = new TestCase( SECTION,   "!('string')",            false,  !("string") );
    testcases[tc++] = new TestCase( SECTION,   "!(new String(''))",      false,  !(new String("")) );
    testcases[tc++] = new TestCase( SECTION,   "!(new String('string'))",    false,  !(new String("string")) );
    testcases[tc++] = new TestCase( SECTION,   "!(new String())",        false,  !(new String()) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Boolean(true))",   false,   !(new Boolean(true)) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Boolean(false))",  false,   !(new Boolean(false)) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Array())",         false,  !(new Array()) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Array(1,2,3)",     false,  !(new Array(1,2,3)) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Number())",        false,  !(new Number()) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Number(0))",       false,  !(new Number(0)) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Number(NaN))",     false,  !(new Number(Number.NaN)) );
    testcases[tc++] = new TestCase( SECTION,   "!(new Number(Infinity))", false, !(new Number(Number.POSITIVE_INFINITY)) );

    test();

function test() {
        for ( tc = 0; tc < testcases.length; tc++ ) {
            testcases[tc].passed = writeTestCaseResult(
                    testcases[tc].expect,
                    testcases[tc].actual,
                    testcases[tc].description +" = "+ testcases[tc].actual );

            testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value "
        }
        stopTest();

    //  all tests must return a boolean value
        return ( testcases );
}
