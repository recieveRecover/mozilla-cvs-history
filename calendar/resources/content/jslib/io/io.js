/* -*- Mode: Javascript; tab-width: 2; */

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
 * The Original Code is Collabnet code.
 *
 * The Initial Developer of the Original Code is
 * Collabnet.
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pete Collins, Doug Turner, Brendan Eich, Warren Harris
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

if(typeof(JS_LIB_LOADED)=='boolean')
{
/********************* INCLUDED FILES **************/
include(JS_LIB_PATH+'io/filesystem.js');
include(JS_LIB_PATH+'io/file.js');
include(JS_LIB_PATH+'io/dir.js');
include(JS_LIB_PATH+'io/fileUtils.js');
include(JS_LIB_PATH+'io/dirUtils.js');
/********************* INCLUDED FILES **************/
}

else
{
    dump("JSLIB library not loaded:\n"                                  +
         " \tTo load use: chrome://jslib/content/jslib.js\n"            +
         " \tThen: include('chrome://jslib/content/io/io.js');\n\n");
}

