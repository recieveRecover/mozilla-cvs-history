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
 */

var anchorElement = null;
var imageElement = null;
var insertNew = false;
var insertLinkAtCaret;
var needLinkText = false;
var href;
var newLinkText;
var HNodeArray;
var haveNamedAnchors = false;
var haveHeadings = false;

// NOTE: Use "href" instead of "a" to distinguish from Named Anchor
// The returned node is has an "a" tagName
var tagName = "href";

// dialog initialization code
function Startup()
{
  if (!InitEditorShell())
    return;

  doSetOKCancel(onOK, null);

  dialog = new Object;
  if (!dialog)
  {
    dump("Failed to create dialog object!!!\n");
    window.close();
  }

  // Message was wrapped in a <label> or <div>, so actual text is a child text node
  dialog.linkTextCaption     = document.getElementById("linkTextCaption");
  dialog.linkTextMessage     = document.getElementById("linkTextMessage");
  dialog.linkTextInput       = document.getElementById("linkTextInput");
  dialog.hrefInput           = document.getElementById("hrefInput");
  dialog.NamedAnchorList     = document.getElementById("NamedAnchorList");
  dialog.HeadingsList        = document.getElementById("HeadingsList");
  dialog.MoreSection         = document.getElementById("MoreSection");
  dialog.MoreFewerButton     = document.getElementById("MoreFewerButton");
  dialog.AdvancedEditSection = document.getElementById("AdvancedEdit");

  var selection = editorShell.editorSelection;
  if (selection) {
    dump("There is a selection: collapsed = "+selection.isCollapsed+"\n");
  } else {
    dump("Failed to get selection\n");
  }

  // See if we have a single selected image
  imageElement = editorShell.GetSelectedElement("img");
  
  if (imageElement)
  {
    // Get the parent link if it exists -- more efficient than GetSelectedElement()
    anchorElement = editorShell.GetElementOrParentByTagName("href", imageElement);
    if (anchorElement)
    {
dump("Existing image link"+anchorElement+"\n");
      if (anchorElement.childNodes.length > 1)
      {
dump("Copying existing image link\n");
        // If there are other children, then we want to break
        //  this image away by inserting a new link around it,
        //  so make a new node and copy existing attributes
        anchorElement = anchorElement.cloneNode(false);
        insertNew = true;
      }
    }
  }
  else
  {
    // Get an anchor element if caret or
    //   entire selection is within the link.
    anchorElement = editorShell.GetSelectedElement(tagName);

    if (anchorElement)
    {
      // Select the entire link
      editorShell.SelectElement(anchorElement);
      selection = editorShell.editorSelection;
    }
    else
    {
      // If selection starts in a link, but extends beyond it,
      //   the user probably wants to extend existing link to new selection,
      //   so check if either end of selection is within a link
      // POTENTIAL PROBLEM: This prevents user from selecting text in an existing
      //   link and making 2 links. 
      // Note that this isn't a problem with images, handled above

      anchorElement = editorShell.GetElementOrParentByTagName("href", selection.anchorNode);
      if (!anchorElement)
        anchorElement = editorShell.GetElementOrParentByTagName("href", selection.focusNode);

      if (anchorElement)
      {
        // But clone it for reinserting/merging around existing
        //   link that only partially overlaps the selection
        anchorElement = anchorElement.cloneNode(false);
        insertNew = true;
      }
    }
  }

  if(!anchorElement)
  {
    // No existing link -- create a new one
    anchorElement = editorShell.CreateElementWithDefaults(tagName);
    insertNew = true;
  }
  if(!anchorElement)
  {
    dump("Failed to get selected element or create a new one!\n");
    window.close();
  } 

  // We insert at caret only when nothing is selected
  insertLinkAtCaret = selection.isCollapsed;
  
  if (insertLinkAtCaret)
  {
    // Note: Use linkTextMessage for normal weight, 
    //       because linkTextCaption is bold (set in EdDialog.css)
    dialog.linkTextMessage.setAttribute("value",GetString("EnterLinkText"));
    // Hide the other string 
    dialog.linkTextCaption.setAttribute("hidden","true");
  }
  else
  {
    if (!imageElement)
    {
      // Don't think we can ever get here!

      // Check if selection has some text - use that first
      selectedText = GetSelectionAsText();
      if (selectedText.length == 0) 
      {
        // No text, look for first image in the selection
        var children = anchorElement.childNodes;
        if (children)
        {
          for(i=0; i < children.length; i++) 
          {
            if (children.item(i) == "IMG")
            {
              imageElement = children.item(i);
              break;
            }
          }
        }
      }
    }
    // Set "caption" for link source and the source text or image URL
    if (imageElement)
    {
      dialog.linkTextCaption.setAttribute("value",GetString("LinkImage"));
      // Link source string is the source URL of image
      // TODO: THIS DOESN'T HANDLE MULTIPLE SELECTED IMAGES!
      dialog.linkTextMessage.setAttribute("value",imageElement.src);
    } else {
      dialog.linkTextCaption.setAttribute("value",GetString("LinkText"));
      if (selectedText.length > 0) {
        // Use just the first 50 characters and add "..."
        dialog.linkTextMessage.setAttribute("value",TruncateStringAtWordEnd(selectedText, 50, true));
      } else {
        dialog.linkTextMessage.setAttribute("value",GetString("MixedSelection"));
      }
    }
  }

  // Make a copy to use for AdvancedEdit and onSaveDefault
  globalElement = anchorElement.cloneNode(false);

  // Get the list of existing named anchors and headings
  FillListboxes();

  // Set data for the dialog controls
  InitDialog();

  // Set initial focus
  if (insertLinkAtCaret) {
    dump("Setting focus to dialog.linkTextInput\n");
    // We will be using the HREF inputbox, so text message
    dialog.linkTextInput.focus();
  } else {
    dump("Setting focus to dialog.linkTextInput\n");
    dialog.hrefInput.focus();

    // We will not insert a new link at caret, so remove link text input field
    dialog.linkTextInput.setAttribute("hidden","true");
    dialog.linkTextInput = null;
  }
  InitMoreFewer();
}

// Set dialog widgets with attribute data
// We get them from globalElement copy so this can be used
//   by AdvancedEdit(), which is shared by all property dialogs
function InitDialog()
{
  dialog.hrefInput.value = globalElement.href;
}

function chooseFile()
{
  // Get a local file, converted into URL format
  fileName = editorShell.GetLocalFileURL(window, "html");
  if (fileName) {
    dialog.hrefInput.value = fileName;
  }
  // Put focus into the input field
  dialog.hrefInput.focus();
}

function RemoveLink()
{
  // Simple clear the input field!
  // BUG: This doesn't clear the input field!
  dialog.hrefInput.value = "";
  dialog.hrefInput.focus();
}

function FillListboxes()
{
  var NamedAnchorNodeList = editorShell.editorDocument.anchors;
  var NamedAnchorCount = NamedAnchorNodeList.length;
  if (NamedAnchorCount > 0) {
    for (var i = 0; i < NamedAnchorCount; i++) {
      AppendStringToList(dialog.NamedAnchorList,NamedAnchorNodeList.item(i).name);
    }
    haveNamedAnchors = true;
  } else {
    // Message to tell user there are none
    AppendStringToList(dialog.NamedAnchorList,GetString("NoNamedAnchors"));
    dialog.NamedAnchorList.setAttribute("disabled", "true");
  }
  var firstHeading = true;
  for (var j = 1; j <= 6; j++) {
    var headingList = editorShell.editorDocument.getElementsByTagName("h"+String(j));
    dump(headingList+" Count= "+headingList.length+"\n");
    if (headingList.length > 0) {
      dump("HELLO\n");
      var heading = headingList.item(0);

      // Skip headings that already have a named anchor as their first child
      //  (this may miss nearby anchors, but at least we don't insert another
      //   under the same heading)
      var child = heading.firstChild;
      if( child && child.name )
        dump(child.name+" = Child.name. Length="+child.name.length+"\n");
      if (child && child.nodeName == "A" && child.name && (child.name.length>0)) {
        continue;
      }

      var range = editorShell.editorDocument.createRange();
      range.setStart(heading,0);
      var lastChildIndex = heading.childNodes.length;
      range.setEnd(heading,lastChildIndex);
      var text = range.toString();
      if (text) {
        // Use just first 40 characters, don't add "...",
        //  and replace whitespace with "_" and strip non-word characters
        text = PrepareStringForURL(TruncateStringAtWordEnd(text, 40, false));
        // Append "_" to any name already in the list
        if (GetExistingHeadingIndex(text) > -1)
          text += "_";
        AppendStringToList(dialog.HeadingsList, text);

        // Save nodes in an array so we can create anchor node under it later
        if (!HNodeArray)
          HNodeArray = new Array(heading)
        else
          HNodeArray[HNodeArray.length] = heading;
      }
    }
  }
  if (HNodeArray) {
    haveHeadings = true;
  } else {
    // Message to tell user there are none
    AppendStringToList(dialog.HeadingsList,GetString("NoHeadings"));
    dialog.HeadingsList.setAttribute("disabled", "true");
  }
}

function GetExistingHeadingIndex(text)
{
  dump("Heading text: "+text+"\n");
  for (var i=0; i < dialog.HeadingsList.length; i++) {
    dump("HeadingListItem"+i+": "+dialog.HeadingsList.options[i].value+"\n");
    if (dialog.HeadingsList.options[i].value == text)
      return i;
  }
  return -1;
}

function SelectNamedAnchor()
{
  if (haveNamedAnchors) {
    dialog.hrefInput.value = "#"+dialog.NamedAnchorList.options[dialog.NamedAnchorList.selectedIndex].value;
  }
}

function SelectHeading()
{
  if (haveHeadings) {
    dialog.hrefInput.value = "#"+dialog.HeadingsList.options[dialog.HeadingsList.selectedIndex].value;
  }
}

// Get and validate data from widgets.
// Set attributes on globalElement so they can be accessed by AdvancedEdit()
function ValidateData()
{
  href = dialog.hrefInput.value.trimString();
  if (href.length > 0) {
    // Set the HREF directly on the editor document's anchor node
    //  or on the newly-created node if insertNew is true
    globalElement.setAttribute("href",href);
    dump("HREF:"+href+"|\n");
  } else if (insertNew) {
    // We must have a URL to insert a new link
    //NOTE: We accept an empty HREF on existing link to indicate removing the link
    dump("Empty HREF error\n");
    ShowInputErrorMessage(GetString("EmptyHREFError"));
    return false;
  }
  if (dialog.linkTextInput) {
    // The text we will insert isn't really an attribute,
    //  but it makes sense to validate it
    newLinkText = TrimString(dialog.linkTextInput.value);
    if (newLinkText.length == 0) {
      ShowInputErrorMessage(GetString("GetInputError"));
      dialog.linkTextInput.focus();
      return false;
    }
  }
  return true;
}


function onOK()
{
  if (ValidateData())
  {
    if (href.length > 0) {
      // Copy attributes to element we are changing or inserting
      editorShell.CloneAttributes(anchorElement, globalElement);

      // Coalesce into one undo transaction
      editorShell.BeginBatchChanges();

      // Get text to use for a new link
      if (insertLinkAtCaret) {
        // Append the link text as the last child node 
        //   of the anchor node
        textNode = editorShell.editorDocument.createTextNode(newLinkText);
        if (textNode) {
          anchorElement.appendChild(textNode);
        }
        try {
          editorShell.InsertElementAtSelection(anchorElement, false);
        } catch (e) {
          dump("Exception occured in InsertElementAtSelection\n");
          return true;
        }
      } else if (insertNew) {
        //  Link source was supplied by the selection,
        //  so insert a link node as parent of this
        //  (may be text, image, or other inline content)
        try {
dump("InsertLink around selection\n");
          editorShell.InsertLinkAroundSelection(anchorElement);
        } catch (e) {
          dump("Exception occured in InsertElementAtSelection\n");
          return true;
        }
      }
      // Check if the link was to a heading 
      if (href[0] == "#") {
        var name = href.substr(1);
        var index = GetExistingHeadingIndex(name);
        dump("Heading name="+name+" Index="+index+"\n");
        if (index >= 0) {
          // We need to create a named anchor 
          //  and insert it as the first child of the heading element
          var headNode = HNodeArray[index];
          var anchorNode = editorShell.editorDocument.createElement("a");
          if (anchorNode) {
            anchorNode.name = name;
            // Remember to use editorShell method so it is undoable!
            editorShell.InsertElement(anchorNode, headNode, 0);
            dump("Anchor node created and inserted under heading\n");
          }
        } else {
          dump("HREF is a heading but is not in the list!\n");
        }
      }
      editorShell.EndBatchChanges();
    } else if (!insertNew) {
      // We already had a link, but empty HREF means remove it
      editorShell.RemoveTextProperty("a", "");
    }
    return true;
  }
  return false;
}
