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
 * The Original Code is ChatZilla.
 *
 * The Initial Developer of the Original Code is James Ross.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   James Ross <silver@warwickcompsoc.co.uk>
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

var rv, rad, box1, box2;

function changeType()
{
    box2.disabled = (rad.value == "net");
}

function onOK()
{
    rv.ok = true;
    
    rv.type = rad.value;
    rv.net = box1.value;
    rv.chan = box2.value;
    
    return true;
}

function onCancel()
{
    rv.ok = false;
    
    return true;
}

function onLoad()
{
    rad = document.getElementById("prefType");
    box1 = document.getElementById("prefName1");
    box2 = document.getElementById("prefName2");
    
    rv = window.arguments[0];
    
    if (!("type" in rv))
        rv.type = "";
    if (!("net" in rv))
        rv.net = "";
    if (!("chan" in rv))
        rv.chan = "";
    rv.ok = false;
    
    if (rv.type == "net")
        rad.selectedIndex = 0;
    if (rv.type == "chan")
        rad.selectedIndex = 1;
    if (rv.type == "user")
        rad.selectedIndex = 2;
    
    box1.value = rv.net || "";
    box2.value = rv.chan || "";
}
