/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/ 
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License. 
 *
 * The Original Code is The JavaScript Debugger
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation
 * Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 *
 * Contributor(s):
 *  Robert Ginda, <rginda@netscape.com>, original author
 *
 */

function initPrefs()
{
    var dir = getSpecialDirectory("ProfD");
    dir.append("venkman-settings.js");
    var defaultSettingsFile = dir.path;

    console.prefManager = new PrefManager("extensions.venkman.");
    console.prefs = console.prefManager.prefs;
    
    var prefs =
        [
         ["enableChromeFilter", true],
         ["guessContext", 5],
         ["guessPattern", "(\\w+)\\s*[:=]\\s*$"],
         ["initialScripts", ""],
         ["lastErrorMode", "ignore"],
         ["lastThrowMode", "ignore"],
         ["maxStringLength", 4000],
         ["menubarInFloaters", navigator.platform.indexOf ("Mac") != -1],
         ["permitStartupHit", true],
         ["prettyprint", false],
         ["rememberPrettyprint", false],
         ["saveSettingsOnExit", false],
         ["settingsFile", defaultSettingsFile],
         ["startupCount", 0],
         ["statusDuration", 5 * 1000],
         ["tabWidth", 4]
        ];

    console.prefManager.addPrefs(prefs);
}

function destroyPrefs()
{
    console.prefManager.destroy();
}
