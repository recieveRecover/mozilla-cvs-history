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
 *   Ben "Count XULula" Goodger
 */

function BuildJSEAttributeNameList()
{
  ClearMenulist(dialog.AddJSEAttributeNameList);
  
  // Get events specific to current element
  var attNames = gJSAttr[gElement.localName.toLowerCase()];
  var i;
  var popup;
  var sep;

  if (attNames && attNames.length)
  {
    for (i = 0; i < attNames.length; i++)
      AppendStringToMenulist(dialog.AddJSEAttributeNameList, attNames[i]);

    popup = dialog.AddJSEAttributeNameList.firstChild;
    if (popup)
    {
      sep = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuseparator");
      if (sep)
        popup.appendChild(sep);
    }        
  }

  // Always add core JS events
  for (i = 0; i < gCoreJSEvents.length; i++)
  {
    if (gCoreJSEvents[i] == "-")
    {
      if (!popup)
        popup = dialog.AddJSEAttributeNameList.firstChild;

      sep = document.createElementNS("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul", "menuseparator");

      if (popup && sep)
        popup.appendChild(sep);
    }
    else
      AppendStringToMenulist(dialog.AddJSEAttributeNameList, gCoreJSEvents[i]);
  }
  
  dialog.AddJSEAttributeNameList.selectedIndex = 0;

  // Use current name and value of first tree item if it exists
  onSelectJSETreeItem();

  dialog.AddJSEAttributeNameList.focus();
}

// build attribute list in tree form from element attributes
function BuildJSEAttributeTable()
{
  var nodeMap = gElement.attributes;
  if (nodeMap.length > 0)
  {
    var added = false;
    for (var i = 0; i < nodeMap.length; i++)
    {
      if( CheckAttributeNameSimilarity( nodeMap[i].nodeName, JSEAttrs ) )
        continue;   // repeated or non-JS handler, ignore this one and go to next
      if( !IsEventHandler( nodeMap[i].nodeName ) )
        continue; // attribute isn't an event handler.
      var name  = nodeMap[i].nodeName.toLowerCase();
      var value = gElement.getAttribute(nodeMap[i].nodeName);
      if (AddTreeItem( name, value, "JSEAList", JSEAttrs )) // add item to tree
        added = true;
    }

    // Select first item
    if (added)
      dialog.AddJSEAttributeTree.selectedIndex = 0;
  }
}

// check to see if given string is an event handler.
function IsEventHandler( which )
{
  var handlerName = which.toLowerCase();
  var firstTwo = handlerName.substring(0,2);
  if (firstTwo == "on")
    return true;
  else
    return false;
}

function onSelectJSEAttribute()
{
  if(!gDoOnSelectTree)
    return;

  dialog.AddJSEAttributeValueInput.value = 
      GetAndSelectExistingAttributeValue(dialog.AddJSEAttributeNameList.label, "JSEAList");
}

function onSelectJSETreeItem()
{
  var tree = dialog.AddJSEAttributeTree;
  if (tree && tree.selectedItems && tree.selectedItems.length)
  {
    var name = GetTreeItemAttributeStr(tree.selectedItems[0]);

    // Select attribute name in list
    if (dialog.AddJSEAttributeNameList.firstChild)
    {
      var arr = dialog.AddJSEAttributeNameList.firstChild.getElementsByAttribute('label', name);
      if (arr && arr.length)
        dialog.AddJSEAttributeNameList.selectedItem = arr[0];

      // Set value input to that in tree (no need to update this in the tree)
      gUpdateTreeValue = false;
      dialog.AddJSEAttributeValueInput.value =  GetTreeItemValueStr(tree.selectedItems[0]);
      gUpdateTreeValue = true;
    }
  }
}

function onInputJSEAttributeValue()
{
  if (gUpdateTreeValue)
  {

    var name = TrimString(dialog.AddJSEAttributeNameList.label);
    var value = TrimString(dialog.AddJSEAttributeValueInput.value);

    // Update value in the tree list
    // Since we have a non-editable menulist, 
    //   we MUST automatically add the event attribute if it doesn't exist
    if (!UpdateExistingAttribute( name, value, "JSEAList" ))
      AddTreeItem( name, value, "JSEAList", JSEAttrs );
  }
}

function editJSEAttributeValue(targetCell)
{
  if (IsNotTreeHeader(targetCell))
    dialog.AddJSEAttributeValueInput.inputField.select();
}

function UpdateJSEAttributes()
{
  var JSEAList = document.getElementById("JSEAList");
  var i;

  // remove removed attributes
  for (i = 0; i < JSERAttrs.length; i++)
  {
    name = JSERAttrs[i];
    if (gElement.getAttribute(name))
      gElement.removeAttribute(name);
  }

  // Add events
  for (i = 0; i < JSEAList.childNodes.length; i++)
  {
    var item = JSEAList.childNodes[i];

    // set the event handler
    gElement.setAttribute( GetTreeItemAttributeStr(item), GetTreeItemValueStr(item) );
  }
}

function RemoveJSEAttribute()
{
  var treechildren = dialog.AddJSEAttributeTree.lastChild;

  // This differs from HTML and CSS panels: 
  // We reselect after removing, because there is not
  //  editable attribute name input, so we can't clear that
  //  like we do in other panels
  var newIndex = dialog.AddJSEAttributeTree.selectedIndex;

  // We only allow 1 selected item
  if (dialog.AddJSEAttributeTree.selectedItems.length)
  {
    var item = dialog.AddJSEAttributeTree.selectedItems[0];

    // Name is the text of the treecell
    var attr = GetTreeItemAttributeStr(item);

    // remove the item from the attribute array
    if (newIndex >= (JSEAttrs.length-1))
      newIndex--;

    // remove the item from the attribute array
    JSERAttrs[JSERAttrs.length] = attr;
    RemoveNameFromAttArray(attr, JSEAttrs);

    // Remove the item from the tree
    treechildren.removeChild (item);

    // Reselect an item
    dialog.AddJSEAttributeTree.selectedIndex = newIndex;
  }
}
