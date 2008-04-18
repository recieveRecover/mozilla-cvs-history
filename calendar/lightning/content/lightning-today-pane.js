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
    - The Original Code is Sun Microsystems code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Berend Cornelius <berend.cornelius@sun.com>
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


 var ltnTodaypaneButton = "mail-show-todaypane-button";
 
 function ltnAddButtonToDefaultset(toolbarSetString) {
     var mailToolbar = getMailBar();
     var elementcount = mailToolbar.childNodes.length;
     var buttonisWithinSet = false;
     // by default the todaypane-button is to be placed before the first
     // separator of the toolbar
     for (var i = 0; i < elementcount; i++) {
         var element = mailToolbar.childNodes[i];
         if (element.localName == "toolbarseparator") {
             var separatorindex = toolbarSetString.indexOf("separator");
             if (separatorindex > -1) {
                 var firstSubString = toolbarSetString.substring(0, separatorindex);
                 var secondSubString = toolbarSetString.substring(separatorindex);
                 toolbarSetString = firstSubString + ltnTodaypaneButton + "," + secondSubString;
             }
             buttonisWithinSet = true;
             break;
         }
     }
     if (!buttonisWithinSet) {
         // in case there is no separator within the toolbar we append the
         // todaypane-button
         toolbarSetString += "," + ltnTodaypaneButton;
     }
     return toolbarSetString;
 }
 
 function ltnAddButtonToToolbarset() {
     var mailToolbar = getMailBar();
     var elementcount = mailToolbar.childNodes.length;
     var buttonisWithinSet = false;
     // by default the todaypane-button is to be placed before the first
     // separator of the toolbar
     for (var i = 0; i < elementcount; i++) {
         var element = mailToolbar.childNodes[i];
         if (element.localName == "toolbarseparator") {
             mailToolbar.insertItem(ltnTodaypaneButton, element, null, false);
             buttonisWithinSet = true;
             break;
         }
     }
     if (!buttonisWithinSet) {
         // in case there is no separator within the toolbar we append the
         // todaypane-button
         mailToolbar.insertItem(ltnTodaypaneButton, null, null, false);
     }
 }
 
 /**  ltnInitTodayPane()
 *    initializes TodayPane for Lightning and adds a toolbarbutton to mail-toolbar once
 */
 function ltnInitTodayPane() {
     // set Lightning attributes for current mode and restore last PaneView
 
     // add a menuitem to the 'View/Layout' -menu. As the respective "Layout" menupopup
     // carries no 'id' attribute it cannot be overlaid
     var todayMenuItem = document.getElementById("ltnShowTodayPane");
     todayMenuItem = todayMenuItem.cloneNode(false);
     todayMenuItem.setAttribute("id", "ltnShowTodayPaneMailMode");
     todayMenuItem.removeAttribute("mode");
     var messagePaneMenu = document.getElementById("menu_MessagePaneLayout");
     if (messagePaneMenu != null) {
         var messagePanePopupMenu = messagePaneMenu.firstChild;
         messagePanePopupMenu.appendChild(todayMenuItem);
     }
 
     // add toolbar-button to mail-toolbar
     var mailToolbar = getMailBar();
     var addToolbarbutton = false;
     var defaultSetString = mailToolbar.getAttribute("defaultset");
     if (defaultSetString.indexOf(ltnTodaypaneButton) == -1) {
         defaultSetString = ltnAddButtonToDefaultset(defaultSetString);
         mailToolbar.setAttribute("defaultset", defaultSetString);
     }
 
     // add the toolbarbutton to the toolbarpalette on first startup
     var todaypanebox = document.getElementById("today-pane-panel");
     if (todaypanebox.hasAttribute("addtoolbarbutton")) {
         addToolbarbutton = (todaypanebox.getAttribute("addtoolbarbutton") == "true");
     }
     if (addToolbarbutton) {
         var currentSetString = mailToolbar.getAttribute("currentset");
         if (currentSetString.indexOf(ltnTodaypaneButton) == -1) {
             ltnAddButtonToToolbarset();
        }
         todaypanebox.setAttribute("addtoolbarbutton", "false");
     }
 }