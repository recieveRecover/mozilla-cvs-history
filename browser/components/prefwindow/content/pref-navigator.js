/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 */

const nsIFilePicker     = Components.interfaces.nsIFilePicker;
const nsIWindowMediator = Components.interfaces.nsIWindowMediator;
const nsIPrefService    = Components.interfaces.nsIPrefService;
const nsIPrefLocalizedString = Components.interfaces.nsIPrefLocalizedString;

const FILEPICKER_CONTRACTID     = "@mozilla.org/filepicker;1";
const WINDOWMEDIATOR_CONTRACTID = "@mozilla.org/appshell/window-mediator;1";
const PREFSERVICE_CONTRACTID    = "@mozilla.org/preferences-service;1";

function setHomePageToCurrentPage()
{
  var windowManager = Components.classes[WINDOWMEDIATOR_CONTRACTID]
                                .getService(nsIWindowMediator);

  var browserWindow = windowManager.getMostRecentWindow("navigator:browser");
  if (browserWindow) {
    var browser = browserWindow.document.getElementById("content");
    var url = browser.webNavigation.currentURI.spec;
    if (url) {
      var homePageField = document.getElementById("browserStartupHomepage");
      homePageField.value = url;
    }
  }
}

function setHomePageToDefaultPage()
{
  var prefService = Components.classes[PREFSERVICE_CONTRACTID]
                              .getService(nsIPrefService);
  var pref = prefService.getDefaultBranch(null);
  var url = pref.getComplexValue("browser.startup.homepage",
                                  nsIPrefLocalizedString).data;
  var homePageField = document.getElementById("browserStartupHomepage");
  homePageField.value = url;
}
