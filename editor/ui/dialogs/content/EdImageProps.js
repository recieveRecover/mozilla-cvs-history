/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *   Pete Collins
 *   Brian King
 *   Ben Goodger
 */

var insertNew     = true;
var insertNewIMap = true;
var wasEnableAll  = false;
var constrainOn = false;
// Note used in current version, but these are set correctly
//  and could be used to reset width and height used for constrain ratio
var constrainWidth  = 0;
var constrainHeight = 0;
var imageElement;
var imageMap = 0;
var canRemoveImageMap = false;
var imageMapDisabled = false;
var dialog;
var globalMap;
var doAltTextError = true;
var actualWidth = "";
var actualHeight = "";

// These must correspond to values in EditorDialog.css for each theme
// (unfortunately, setting "style" attribute here doesn't work!)
var gPreviewImageWidth = 80;
var gPreviewImageHeight = 50;
var StartupCalled = false;

// dialog initialization code

function Startup()
{
  //XXX Very weird! When calling this with an existing image,
  //    we get called twice. That causes dialog layout
  //    to explode to fullscreen!
  if (StartupCalled)
  {
    dump("*** CALLING IMAGE DIALOG Startup() AGAIN! ***\n");
    return;
  }
  StartupCalled = true;

  if (!InitEditorShell())
    return;

  doSetOKCancel(onOK, onCancel);

  // Create dialog object to store controls for easy access
  dialog = new Object;

  dialog.srcInput          = document.getElementById( "srcInput" );
  dialog.altTextInput      = document.getElementById( "altTextInput" );
  dialog.MoreFewerButton   = document.getElementById( "MoreFewerButton" );
  dialog.AdvancedEditButton = document.getElementById( "AdvancedEditButton" );
  dialog.AdvancedEditButton2 = document.getElementById( "AdvancedEditButton2" );
  dialog.MoreSection       = document.getElementById( "MoreSection" );
  dialog.customSizeRadio   = document.getElementById( "customSizeRadio" );
  dialog.actualSizeRadio = document.getElementById( "actualSizeRadio" );
  dialog.constrainCheckbox = document.getElementById( "constrainCheckbox" );
  dialog.widthInput        = document.getElementById( "widthInput" );
  dialog.heightInput       = document.getElementById( "heightInput" );
  dialog.widthUnitsMenulist   = document.getElementById( "widthUnitsMenulist" );
  dialog.heightUnitsMenulist  = document.getElementById( "heightUnitsMenulist" );
  dialog.imagelrInput      = document.getElementById( "imageleftrightInput" );
  dialog.imagetbInput      = document.getElementById( "imagetopbottomInput" );
  dialog.border            = document.getElementById( "border" );
  dialog.alignTypeSelect   = document.getElementById( "alignTypeSelect" );
  dialog.alignImage        = document.getElementById( "alignImage" );
  dialog.alignText         = document.getElementById( "alignText" );
  dialog.editImageMap      = document.getElementById( "editImageMap" );
  dialog.removeImageMap    = document.getElementById( "removeImageMap" );
  dialog.ImageHolder       = document.getElementById( "preview-image-holder" );
  dialog.PreviewImage      = document.getElementById( "preview-image" );
  dialog.PreviewBox        = document.getElementById( "preview-image-box" );
  dialog.PreviewWidth      = document.getElementById( "PreviewWidth" );
  dialog.PreviewHeight     = document.getElementById( "PreviewHeight" );
  dialog.PreviewSize       = document.getElementById( "PreviewSize" );

  // Get a single selected image element
  var tagName = "img"
  imageElement = editorShell.GetSelectedElement(tagName);

  if (imageElement)
  {
    // We found an element and don't need to insert one
    insertNew = false;
    actualWidth  = imageElement.naturalWidth;
    actualHeight = imageElement.naturalHeight;
  }
  else
  {
    insertNew = true;

    // We don't have an element selected,
    //  so create one with default attributes

    imageElement = editorShell.CreateElementWithDefaults(tagName);
    if( !imageElement )
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
  }

  // Make a copy to use for AdvancedEdit
  globalElement = imageElement.cloneNode(false);


  // Get image map for image
  var usemap = globalElement.getAttribute("usemap");
  if (usemap)
  {
    var mapname = usemap.substring(1, usemap.length);
    var mapCollection = editorShell.editorDocument.getElementsByName(mapname);
    if (mapCollection[0] != null)
    {
      imageMap = mapCollection[0];
      globalMap = imageMap;
      canRemoveImageMap = true;
      insertNewIMap = false;
    }
    else
    {
      insertNewIMap = true;
      globalMap = null;
    }
  }
  else
  {
    insertNewIMap = true;
    globalMap = null;
  }
  InitDialog();

  // By default turn constrain on, but both width and height must be in pixels
  dialog.constrainCheckbox.checked =
    dialog.widthUnitsMenulist.selectedIndex == 0 &&
    dialog.heightUnitsMenulist.selectedIndex == 0;

  // Set SeeMore bool to the OPPOSITE of the current state,
  //   which is automatically saved by using the 'persist="more"'
  //   attribute on the MoreFewerButton button
  //   onMoreFewer will toggle the state and redraw the dialog
  SeeMore = (dialog.MoreFewerButton.getAttribute("more") != "1");

  // Initialize widgets with image attributes in the case where the entire dialog isn't visible
  onMoreFewer();  // this call will initialize all widgets if entire dialog is visible

  SetTextboxFocus(dialog.srcInput);

  SetWindowLocation();
}

// Set dialog widgets with attribute data
// We get them from globalElement copy so this can be used
//   by AdvancedEdit(), which is shared by all property dialogs
function InitDialog()
{
  // Set the controls to the image's attributes

  var str = globalElement.getAttribute("src");
  if (str)
  {
    dialog.srcInput.value = str;
    GetImageFromURL();
  }

  str = globalElement.getAttribute("alt");
  if (str)
    dialog.altTextInput.value = str;

  // setup the height and width widgets
  var width = InitPixelOrPercentMenulist(globalElement,
                    insertNew ? null : imageElement,
                    "width", "widthUnitsMenulist", gPixel);
  var height = InitPixelOrPercentMenulist(globalElement,
                    insertNew ? null : imageElement,
                    "height", "heightUnitsMenulist", gPixel);

  // Set actual radio button if both set values are the same as actual
  if (actualWidth && actualHeight)
    dialog.actualSizeRadio.checked = (width == actualWidth) && (height == actualHeight);
  else if ( !(width || height) )
    dialog.actualSizeRadio.checked = true;

  if (!dialog.actualSizeRadio.checked)
    dialog.customSizeRadio.checked = true;

  dialog.widthInput.value  = constrainWidth = width ? width : (actualWidth ? actualWidth : "");
  dialog.heightInput.value = constrainHeight = height ? height : (actualHeight ? actualHeight : "");

  // set spacing editfields
  dialog.imagelrInput.value = globalElement.getAttribute("hspace");
  dialog.imagetbInput.value = globalElement.getAttribute("vspace");
  dialog.border.value       = globalElement.getAttribute("border");

  // Get alignment setting
  var align = globalElement.getAttribute("align");
  if (align) {
    align = align.toLowerCase();
  }
  var imgClass;
  var textID;
  switch ( align )
  {
    case "top":
    case "center":
    case "right":
    case "left":
      dialog.alignTypeSelect.value = align;
      break;
    default:  // Default or "bottom"
      dialog.alignTypeSelect.value = "bottom";
  }

  // we want to force an update so initialize "wasEnableAll" to be the opposite of what the actual state is
  wasEnableAll = !IsValidImage(dialog.srcInput.value);
  doOverallEnabling();
  doDimensionEnabling();
}

function chooseFile()
{
  // Get a local file, converted into URL format
  var fileName = GetLocalFileURL("img");
  if (fileName) {
    dialog.srcInput.value = fileName;
    doOverallEnabling();
  }
  GetImageFromURL();
  // Put focus into the input field
  SetTextboxFocus(dialog.srcInput);
}

function PreviewImageLoaded()
{
  if (dialog.PreviewImage)
  {
    // Image loading has completed -- we can get actual width
    actualWidth  = dialog.PreviewImage.naturalWidth;
    actualHeight = dialog.PreviewImage.naturalHeight;

    if (actualWidth && actualHeight)
    {
      // Use actual size or scale to fit preview if either dimension is too large
      var width = actualWidth;
      var height = actualHeight;
      if (actualWidth > gPreviewImageWidth)
      {
          width = gPreviewImageWidth;
          height = actualHeight * (gPreviewImageWidth / actualWidth);
      }
      if (height > gPreviewImageHeight)
      {
        height = gPreviewImageHeight;
        width = actualWidth * (gPreviewImageHeight / actualHeight);
      }
      if (actualWidth > gPreviewImageWidth || actualHeight > gPreviewImageHeight)
      {
        // Resize image to fit preview frame
        dialog.PreviewImage.setAttribute("width", width);
        dialog.PreviewImage.setAttribute("height", height);
      }

      dialog.PreviewWidth.setAttribute("value", actualWidth);
      dialog.PreviewHeight.setAttribute("value", actualHeight);

      dialog.PreviewSize.setAttribute("collapsed", "false");
      dialog.ImageHolder.setAttribute("collapsed", "false");

      // Use values as start for constrain proportions
    }

    if (dialog.actualSizeRadio.checked)
      SetActualSize();
  }
}

function GetImageFromURL()
{
  dialog.PreviewSize.setAttribute("collapsed", "true");

  var imageSrc = dialog.srcInput.value;
  if (imageSrc) imageSrc = imageSrc.trimString();
  if (!imageSrc) return;

  if (IsValidImage(imageSrc))
    dialog.PreviewImage.src = imageSrc;
}

function SetActualSize()
{
  dialog.widthInput.value = actualWidth ? actualWidth : "";
  dialog.widthUnitsMenulist.selectedIndex = 0;
  dialog.heightInput.value = actualHeight ? actualHeight : "";
  dialog.heightUnitsMenulist.selectedIndex = 0;
  doDimensionEnabling();
}

function ChangeImageSrc()
{
  GetImageFromURL();
  doOverallEnabling();
}

// This overrides the default onMoreFewer in EdDialogCommon.js
function onMoreFewer()
{
  if (SeeMore)
  {
    dialog.MoreSection.setAttribute("collapsed","true");
    dialog.MoreFewerButton.setAttribute("label", GetString("MoreProperties"));
    dialog.MoreFewerButton.setAttribute("more","0");
    SeeMore = false;
    // Show the "Advanced Edit" button on same line as "More Properties"
    dialog.AdvancedEditButton.setAttribute("collapsed","false");
    dialog.AdvancedEditButton2.setAttribute("collapsed","true");
    // Weird caret appearing when we collapse, so force focus to URL textbox
    dialog.srcInput.focus();
  }
  else
  {
    dialog.MoreSection.setAttribute("collapsed","false");
    dialog.MoreFewerButton.setAttribute("label", GetString("FewerProperties"));
    dialog.MoreFewerButton.setAttribute("more","1");
    SeeMore = true;

    // Show the "Advanced Edit" button at bottom
    dialog.AdvancedEditButton.setAttribute("collapsed","true");
    dialog.AdvancedEditButton2.setAttribute("collapsed","false");
  }
  window.sizeToContent();
}

function doDimensionEnabling()
{
  // Enabled only if "Custom" is checked
  var enable = (dialog.customSizeRadio.checked);

  // BUG 74145: After input field is disabled,
  //   setting it enabled causes blinking caret to appear
  //   even though focus isn't set to it.
  SetElementEnabledById( "heightInput", enable );
  SetElementEnabledById( "heightLabel", enable );
  SetElementEnabledById( "heightUnitsMenulist", enable );

  SetElementEnabledById( "widthInput", enable );
  SetElementEnabledById( "widthLabel", enable);
  SetElementEnabledById( "widthUnitsMenulist", enable );

  var constrainEnable = enable
         && ( dialog.widthUnitsMenulist.selectedIndex == 0 )
         && ( dialog.heightUnitsMenulist.selectedIndex == 0 );

  SetElementEnabledById( "constrainCheckbox", constrainEnable );
}

function doOverallEnabling()
{
  var canEnableOk = IsValidImage(dialog.srcInput.value);
  if ( wasEnableAll == canEnableOk )
    return;

  wasEnableAll = canEnableOk;

  SetElementEnabledById("ok", canEnableOk );

  SetElementEnabledById( "imagemapLabel",  canEnableOk );
  SetElementEnabledById( "editImageMap",   canEnableOk );
  SetElementEnabledById( "removeImageMap", canRemoveImageMap);
}

function ToggleConstrain()
{
  // If just turned on, save the current width and height as basis for constrain ratio
  // Thus clicking on/off lets user say "Use these values as aspect ration"
  if (dialog.constrainCheckbox.checked && !dialog.constrainCheckbox.disabled
     && (dialog.widthUnitsMenulist.selectedIndex == 0)
     && (dialog.heightUnitsMenulist.selectedIndex == 0))
  {
    constrainWidth = Number(dialog.widthInput.value.trimString());
    constrainHeight = Number(dialog.heightInput.value.trimString());
  }
}

function constrainProportions( srcID, destID )
{
  var srcElement = document.getElementById ( srcID );
  if ( !srcElement )
    return;

  var destElement = document.getElementById( destID );
  if ( !destElement )
    return;

  // always force an integer (whether we are constraining or not)
  forceInteger( srcID );

  if (!actualWidth || !actualHeight ||
      !(dialog.constrainCheckbox.checked && !dialog.constrainCheckbox.disabled))
    return;

  // double-check that neither width nor height is in percent mode; bail if so!
  if ( (dialog.widthUnitsMenulist.selectedIndex != 0)
     || (dialog.heightUnitsMenulist.selectedIndex != 0) )
    return;

  // This always uses the actual width and height ratios
  // which is kind of funky if you change one number without the constrain
  // and then turn constrain on and change a number
  // I prefer the old strategy (below) but I can see some merit to this solution
  if (srcID == "widthInput")
    destElement.value = Math.round( srcElement.value * actualHeight / actualWidth );
  else
    destElement.value = Math.round( srcElement.value * actualWidth / actualHeight );

/*
  // With this strategy, the width and height ratio
  //   can be reset to whatever the user entered.
  if (srcID == "widthInput")
    destElement.value = Math.round( srcElement.value * constrainHeight / constrainWidth );
  else
    destElement.value = Math.round( srcElement.value * constrainWidth / constrainHeight );
*/
}

function editImageMap()
{
  // Make a copy to use for image map editor
  if (insertNewIMap)
  {
    imageMap = editorShell.CreateElementWithDefaults("map");
    globalMap = imageMap.cloneNode(true);
  }
  else
    globalMap = imageMap;

  window.openDialog("chrome://editor/content/EdImageMap.xul", "_blank", "chrome,close,titlebar,modal", globalElement, globalMap);
}

function removeImageMap()
{
  globalElement.removeAttribute("usemap");
  if (imageMap){
    canRemoveImageMap = false;
    SetElementEnabledById( "removeImageMap", false);
    editorShell.DeleteElement(imageMap);
    insertNewIMap = true;
    globalMap = null;
  }
}

// Get data from widgets, validate, and set for the global element
//   accessible to AdvancedEdit() [in EdDialogCommon.js]
function ValidateData()
{
  if ( !IsValidImage(dialog.srcInput.value )) {
    ShowInputErrorMessage(GetString("MissingImageError"));
    return false;
  }

  //TODO: WE NEED TO DO SOME URL VALIDATION HERE, E.G.:
  // We must convert to "file:///" or "http://" format else image doesn't load!
  var src = dialog.srcInput.value.trimString();
  globalElement.setAttribute("src", src);

  // Note: allow typing spaces,
  // Warn user if empty string just once per dialog session
  //   but don't consider this a failure
  var alt = dialog.altTextInput.value;
  if (doAltTextError && !alt)
  {
    ShowInputErrorMessage(GetString("NoAltText"));
    SetTextboxFocus(dialog.altTextInput);
    doAltTextError = false;
    return false;
  }
  globalElement.setAttribute("alt", alt);

  var width = "";
  var height = "";

  if (!dialog.actualSizeRadio.checked)
  {
    // Get user values for width and height
    width = ValidateNumber(dialog.widthInput, dialog.widthUnitsMenulist, 1, maxPixels, 
                           globalElement, "width", false, true);
    if (gValidationError)
      return false;

    height = ValidateNumber(dialog.heightInput, dialog.heightUnitsMenulist, 1, maxPixels, 
                            globalElement, "height", false, true);
    if (gValidationError)
      return false;
  }

  // We always set the width and height attributes, even if same as actual.
  //  This speeds up layout of pages since sizes are known before image is downloaded
  // (But don't set if we couldn't obtain actual dimensions)
  if (!width)
    width = actualWidth;
  if (!height)
    height = actualHeight;

  if (width)
    globalElement.setAttribute("width", width);
  else
    globalElement.removeAttribute("width");

  if (height)
    globalElement.setAttribute("height", height);
  else
    globalElement.removeAttribute("height");

  // spacing attributes

  ValidateNumber(dialog.imagelrInput, null, 0, maxPixels, 
                 globalElement, "hspace", false, true, true);
  if (gValidationError)
    return false;

  ValidateNumber(dialog.imagetbInput, null, 0, maxPixels, 
                 globalElement, "vspace", false, true);
  if (gValidationError)
    return false;

  // note this is deprecated and should be converted to stylesheets
  ValidateNumber(dialog.border, null, 0, maxPixels, 
                 globalElement, "border", false, true);
  if (gValidationError)
    return false;

  // Default or setting "bottom" means don't set the attribute
  // Note that the attributes "left" and "right" are opposite
  //  of what we use in the UI, which describes where the TEXT wraps,
  //  not the image location (which is what the HTML describes)
  switch ( dialog.alignTypeSelect.value )
  {
    case "top":
    case "center":
    case "right":
    case "left":
      globalElement.setAttribute( "align", dialog.alignTypeSelect.value );
      break;
    default:
      globalElement.removeAttribute( "align" );
  }

  return true;
}

function onOK()
{
  // handle insertion of new image
  if (ValidateData())
  {
    // Assign to map if there is one
    if ( globalMap )
    {
      var mapName = globalMap.getAttribute("name");
      if (mapName != "")
      {
        globalElement.setAttribute("usemap", ("#"+mapName));
        if (globalElement.getAttribute("border") == "")
          globalElement.setAttribute("border", 0);
      }

      // Copy or insert image map
      imageMap = globalMap.cloneNode(true);
      if (insertNewIMap)
      {
        try
        {
          editorShell.editorDocument.body.appendChild(imageMap);
        //editorShell.InsertElementAtSelection(imageMap, false);
        }
        catch (e)
        {
          dump("Exception occured in InsertElementAtSelection\n");
        }
      }
    }

    // All values are valid - copy to actual element in doc or
    //   element created to insert
    editorShell.CloneAttributes(imageElement, globalElement);
    if (insertNew)
    {
      try {
        // 'true' means delete the selection before inserting
        editorShell.InsertElementAtSelection(imageElement, true);
      } catch (e) {
        dump(e);
      }
    }

    // un-comment to see that inserting image maps does not work!
    /*test = editorShell.CreateElementWithDefaults("map");
    test.setAttribute("name", "testing");
    testArea = editorShell.CreateElementWithDefaults("area");
    testArea.setAttribute("shape", "circle");
    testArea.setAttribute("coords", "86,102,52");
    testArea.setAttribute("href", "test");
    test.appendChild(testArea);
    editorShell.InsertElementAtSelection(test, false);*/

    SaveWindowLocation();
    return true;
  }
  return false;
}
