var gDescriptionField = null;
var gExtensionField   = null;
var gMIMEField        = null;
var gAppPath          = null;
var gOutgoingMIME     = null;

var gBundle           = null;

function Startup()
{
  doSetOKCancel(onOK);
  
  gDescriptionField = document.getElementById("description");
  gExtensionField   = document.getElementById("extensions");
  gMIMEField        = document.getElementById("mimeType");
  gAppPath          = document.getElementById("appPath");
  gOutgoingMime     = document.getElementById("outgoingDefault");
    
  gBundle           = srGetStrBundle("chrome://communicator/locale/pref/pref-applications.properties");  
  
  gDescriptionField.focus();
}

function chooseApp()
{
  var filePicker = Components.classes["component://mozilla/filepicker"].createInstance();
  if (filePicker)
    filePicker = filePicker.QueryInterface(Components.interfaces.nsIFilePicker);
  if (filePicker) {
    const FP = Components.interfaces.nsIFilePicker
    var windowTitle = gBundle.GetStringFromName("chooseHandler");
    var programsFilter = gBundle.GetStringFromName("programsFilter");
    filePicker.init(window, windowTitle, FP.modeOpen);
    if (navigator.platform == "Windows")
      filePicker.appendFilter(programsFilter, "*.exe; *.com");
    else
      filePicker.appendFilters(FP.filterAll);
    filePicker.show();
    var file = filePicker.file.QueryInterface(Components.interfaces.nsILocalFile);
    gAppPath.value = file.path;
    gAppPath.select();
  }
}

var gDS = null;
function onOK()
{
  const mimeTypes = 66638;
  var fileLocator = Components.classes["component://netscape/filelocator"].getService();
  if (fileLocator)
    fileLocator = fileLocator.QueryInterface(Components.interfaces.nsIFileLocator);
  var file = fileLocator.GetFileLocation(mimeTypes);
  if (file)
    file = file.QueryInterface(Components.interfaces.nsIFileSpec);
  gDS = gRDF.GetDataSource(file.URLString);
  if (gDS)
    gDS = gDS.QueryInterface(Components.interfaces.nsIRDFDataSource);

  // figure out if this mime type already exists. 
  var exists = mimeHandlerExists(gMIMEField.value);
  if (exists) {
    var titleMsg = gBundle.GetStringFromName("handlerExistsTitle");
    var dialogMsg = gBundle.GetStringFromName("handlerExists");
    dialogMsg = dialogMsg.replace(/%mime%/g, gMIMEField.value);
    var commonDialogService = nsJSComponentManager.getService("component://netscape/appshell/commonDialogs",
                                                              "nsICommonDialogs");
    var replace = commonDialogService.Confirm(window, titleMsg, dialogMsg);
    if (!replace)
      window.close();
  }
  
  
  // now save the information
  var handlerInfo = new HandlerOverride(MIME_URI(gMIMEField.value));
  handlerInfo.mUpdateMode = exists; // XXX Somewhat sleazy, I know...
  handlerInfo.mimeType = gMIMEField.value;
  handlerInfo.description = gDescriptionField.value;
  
  var extensionString = gExtensionField.value.replace(/[*.;]/g, "");
  var extensions = extensionString.split(" ");
  for (var i = 0; i < extensions.length; i++) {
    var currExtension = extensions[i];
    handlerInfo.addExtension(currExtension);
  }
  handlerInfo.appPath = gAppPath.value;

  // other info we need to set (not reflected in UI)
  handlerInfo.isEditable = true;
  handlerInfo.saveToDisk = false;
  handlerInfo.handleInternal = false;
  handlerInfo.alwaysAsk = true;
  var file = Components.classes["component://mozilla/file/local"].createInstance();
  if (file)
    file = file.QueryInterface(Components.interfaces.nsILocalFile);
  if (file) {
    try {
      file.initWithPath(gAppPath.value);
      handlerInfo.appDisplayName = file.unicodeLeafName;
    }
    catch(e) {
      handlerInfo.appDisplayName = gAppPath.value;    
    }
  }
  // do the rest of the work (ugly yes, but it works)
  handlerInfo.buildLinks();
  
  // flush the ds to disk.   
  gDS = gDS.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
  if (gDS)
    gDS.Flush();
  
  window.opener.gNewTypeRV = true;
  window.close();  
}

