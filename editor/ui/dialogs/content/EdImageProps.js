// OnOK(), Undo(), and Cancel() are in EdDialogCommon.js
// applyChanges() must be implemented here

var appCore;
var toolkitCore;
var insertNew = true;
var undoCount = 0;
var imageElement;
var tagName = "img"

// dialog initialization code
function Startup()
{
  dump("Doing Startup...\n");
  toolkitCore = XPAppCoresManager.Find("ToolkitCore");
  if (!toolkitCore) {
    toolkitCore = new ToolkitCore();
    if (toolkitCore)
      toolkitCore.Init("ToolkitCore");
  }
  if(!toolkitCore) {
    dump("toolkitCore not found!!! And we can't close the dialog!\n");
  }

  // temporary while this window is opend with ShowWindowWithArgs
  dump("Getting parent appcore\n");
  var editorName = document.getElementById("args").getAttribute("value");
  dump("Got editorAppCore called " + editorName + "\n");
  
  // NEVER create an appcore here - we must find parent editor's
  appCore = XPAppCoresManager.Find(editorName);  
  if(!appCore || !toolkitCore) {
    dump("EditorAppCore not found!!!\n");
    toolkitCore.CloseWindow(window);
    return;
  }
  dump("EditorAppCore found for Image Properties dialog\n");

  // Create dialog object to store controls for easy access
  dialog = new Object;
  // This is the "combined" widget:
  dialog.Src = document.getElementById("image.Src");

  dialog.AltText = document.getElementById("image.AltText");
  if (null == dialog.Src || 
      null == dialog.AltText )
  {
    dump("Not all dialog controls were found!!!\n");
  }
      
  initDialog();
  
  dialog.Src.focus();
}

function initDialog() {
  // Get a single selected anchor element
  imageElement = appCore.getSelectedElement(tagName);

  if (imageElement) {
    dump("Found existing image\n");
    // We found an element and don't need to insert one
    insertNew = false;

    // Set the controls to the image's attributes
    dialog.Src.value = imageElement.getAttribute("src");
    dialog.AltText.value = imageElement.getAttribute("alt");
  } else {
    insertNew = true;
    // We don't have an element selected, 
    //  so create one with default attributes
    dump("Element not selected - calling createElementWithDefaults\n");
    imageElement = appCore.createElementWithDefaults(tagName);
  }

  if(!imageElement)
  {
    dump("Failed to get selected element or create a new one!\n");
    //toolkitCore.CloseWindow(window);
  }
}

function applyChanges()
{
  // TODO: BE SURE Src AND AltText are completed!
  imageElement.setAttribute("src",dialog.Src.value);
  // We must convert to "file:///" format else image doesn't load!
  imageElement.setAttribute("alt",dialog.AltText.value);  
  if (insertNew) {
    dump("Src="+imageElement.getAttribute("src")+" Alt="+imageElement.getAttribute("alt")+"\n");
    appCore.insertElement(imageElement, true)
    
  }
}