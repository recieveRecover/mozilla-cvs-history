/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
 *  Preferred Argument Conversion.
 *
 *  Passing a JavaScript boolean to a Java method should prefer to call
 *  a Java method of the same name that expects a Java boolean.
 *
 */
    var SECTION = "Preferred argument conversion: JavaScript object to double";
    var VERSION = "1_4";
    var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
                    SECTION;
    startTest();

    var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_004;

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new String() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous(new String()) +'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new Boolean() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new Boolean() )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new Number() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new Number() )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new Date() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new Date() )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new Function() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new Function() )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( this ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( this )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new RegExp() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new RegExp() )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( Math ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( Math )+'' );

    testcases[testcases.length] = new TestCase(
        "TEST_CLASS.ambiguous( new Object() ) +''",
        "DOUBLE",
        TEST_CLASS.ambiguous( new Object() )+'' );

    test();