/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2003
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): igor@fastmail.fm, PhilSchwartau@aol.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    15 Nov 2003
* SUMMARY: Stressing the byte code generator
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=225831
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 225831;
var summary = 'Stressing the byte code generator';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f() { return {x: 0}; }

var N = 300;
var a = new Array(N + 1);
a[N] = 10;
a[0] = 100;


status = inSection(1);

// build string of the form ++(a[++f().x + ++f().x + ... + ++f().x]) which
// gives ++a[N]
var str = "".concat("++(a[", repeat_str("++f().x + ", (N - 1)), "++f().x])");

// Use Script constructor instead of simple eval to test Rhino optimizer mode
// because in Rhino, eval always uses interpreted mode.
var script = new Script(str);
script();

actual = a[N];
expect = 11;
addThis();


status = inSection(2);

// build string of the form (a[f().x-- + f().x-- + ... + f().x--])--
// which should give (a[0])--
str = "".concat("(a[", repeat_str("f().x-- + ", (N - 1)), "f().x--])--");
script = new Script(str);
script();

actual = a[0];
expect = 99;
addThis();


status = inSection(3);

// build string of the form [[1], [1], ..., [1]]
str = "".concat("[", repeat_str("[1], ", (N - 1)), "[1]]");
script = new Script(str);
script();

actual = uneval(script());
expect = str;
addThis();


status = inSection(4);

// build string of the form ({1:{a:1}, 2:{a:1}, ... N:{a:1}})
str = function() {
  var arr = new Array(N+1);
  arr[0] = "({";
  for (var i = 1; i < N; ++i) {
    arr[i] = i+":{a:1}, ";
  }
  arr[N] = N+":{a:1}})";
  return "".concat.apply("", arr);
}();

script = new Script(str);
script();

actual = uneval(script());
expect = str;
addThis();




//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



function repeat_str(str, repeat_count)
{
  var arr = new Array(--repeat_count);
  while (repeat_count != 0)
    arr[--repeat_count] = str;
  return str.concat.apply(str, arr);
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
