/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

platform.beos = true;

// The other platforms roll this all into "toolbar mode".
pref("browser.chrome.toolbar_tips", true);
pref("browser.chrome.show_menubar", true);

// Instead of "delay_images"
pref("browser.autoload_images", true);

// Not sure what this one does...
pref("browser.fancy_ftp", true);

// Fortezza stuff
pref("fortezza.toggle", 1);
pref("fortezza.timeout", 30);

pref("intl.font_charset", "");
pref("intl.font_spec_list", "");
pref("mail.signature_date", 0);

// Outliner column defaults
pref("addressbook.searchpane.outliner_geometry", "");
pref("addressbook.addresseepane.outliner_geometry", "");
pref("addressbook.listpane.outliner_geometry", "");
pref("addressbook.outliner_geometry", "");
pref("mail.composition.addresspane.outliner_geometry", "");
pref("bookmarks.outliner_geometry", "");
pref("mail.categorypane.outliner_geometry", "");
pref("mail.folderpane.outliner_geometry", "");
pref("history.outliner_geometry", "");
pref("mail.ldapsearchpane.outliner_geometry", "");
pref("mail.searchpane.outliner_geometry", "");
pref("mail.mailfilter.outliner_geometry", "");
pref("preferences.lang.outliner_geometry", "");
pref("preferences.dir.outliner_geometry", "");
pref("preferences.font.outliner_geometry", "");
pref("mail.subscribepane.all_groups.outliner_geometry", "");
pref("mail.subscribepane.new_groups.outliner_geometry", "");
pref("mail.subscribepane.search_groups.outliner_geometry", "");
pref("mail.threadpane.outliner_geometry", "");
pref("mail.threadpane.messagepane_height", 400);

// Customizable toolbar stuff
pref("custtoolbar.personal_toolbar_folder", "Personal Toolbar Folder");
pref("custtoolbar.has_toolbar_folder", true);
pref("custtoolbar.Browser.Navigation_Toolbar.position", 0);
pref("custtoolbar.Browser.Navigation_Toolbar.showing", true);
pref("custtoolbar.Browser.Navigation_Toolbar.open", true);
pref("custtoolbar.Browser.Location_Toolbar.position", 1);
pref("custtoolbar.Browser.Location_Toolbar.showing", true);
pref("custtoolbar.Browser.Location_Toolbar.open", true);
pref("custtoolbar.Browser.Personal_Toolbar.position", 2);
pref("custtoolbar.Browser.Personal_Toolbar.showing", true);
pref("custtoolbar.Browser.Personal_Toolbar.open", true);
pref("custtoolbar.Messenger.Navigation_Toolbar.position", 0);
pref("custtoolbar.Messenger.Navigation_Toolbar.showing", true);
pref("custtoolbar.Messenger.Navigation_Toolbar.open", true);
pref("custtoolbar.Messenger.Location_Toolbar.position", 1);
pref("custtoolbar.Messenger.Location_Toolbar.showing", true);
pref("custtoolbar.Messenger.Location_Toolbar.open", true);
pref("custtoolbar.Messages.Navigation_Toolbar.position", 0);
pref("custtoolbar.Messages.Navigation_Toolbar.showing", true);
pref("custtoolbar.Messages.Navigation_Toolbar.open", true);
pref("custtoolbar.Messages.Location_Toolbar.position", 1);
pref("custtoolbar.Messages.Location_Toolbar.showing", true);
pref("custtoolbar.Messages.Location_Toolbar.open", true);
pref("custtoolbar.Folders.Navigation_Toolbar.position", 0);
pref("custtoolbar.Folders.Navigation_Toolbar.showing", true);
pref("custtoolbar.Folders.Navigation_Toolbar.open", true);
pref("custtoolbar.Folders.Location_Toolbar.position", 1);
pref("custtoolbar.Folders.Location_Toolbar.showing", true);
pref("custtoolbar.Folders.Location_Toolbar.open", true);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.position", 0);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.showing", true);
pref("custtoolbar.Address_Book.Address_Book_Toolbar.open", true);
pref("custtoolbar.Compose_Message.Message_Toolbar.position", 0);
pref("custtoolbar.Compose_Message.Message_Toolbar.showing", true);
pref("custtoolbar.Compose_Message.Message_Toolbar.open", true);
pref("custtoolbar.Composer.Composition_Toolbar.position", 0);
pref("custtoolbar.Composer.Composition_Toolbar.showing", true);
pref("custtoolbar.Composer.Composition_Toolbar.open", true);
pref("custtoolbar.Composer.Formatting_Toolbar.position", 1);
pref("custtoolbar.Composer.Formatting_Toolbar.showing", true);
pref("custtoolbar.Composer.Formatting_Toolbar.open", true);

pref("taskbar.x", -1);
pref("taskbar.y", -1);
pref("taskbar.floating", false);
pref("taskbar.horizontal", false);
pref("taskbar.ontop", false);
pref("taskbar.button_style", -1);

config("menu.help.item_1.url", "http://home.netscape.com/eng/mozilla/5.0/");
