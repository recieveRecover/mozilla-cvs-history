//get prefInt services 

var availCharsetList     = [];
var activeCharsetList    = [];
var availCharsetDict     = [];
var ccm                  = null; //Charset Coverter Mgr.
var prefInt              = null; //Preferences Interface
var pref_string_title    = "";
var pref_string_content  = "";

function Init()
{
  doSetOKCancel(Save);
  
  var applicationArea = "";

  if ("arguments" in window && window.arguments[0])
    applicationArea = window.arguments[0];

  prefInt = Components.classes["@mozilla.org/preferences;1"];

  if (prefInt) {
    prefInt = prefInt.getService(Components.interfaces.nsIPref);

    if (applicationArea.indexOf("mail") != -1) {
      pref_string_title = "intl.charsetmenu.mailedit";
    } else {
    //default is the browser
      pref_string_title = "intl.charsetmenu.browser.static";
    }

    pref_string_content = prefInt.getLocalizedUnicharPref(pref_string_title);
    
    AddRemoveLatin1('add');
  }

  ccm = Components.classes['@mozilla.org/charset-converter-manager;1'];

  if (ccm) {
    ccm = ccm.getService(Components.interfaces.nsICharsetConverterManager2);
    availCharsetList = ccm.GetDecoderList();
    availCharsetList = availCharsetList.QueryInterface(Components.interfaces.nsISupportsArray);
    availCharsetList.sort;
  }

  LoadAvailableCharSets();
  LoadActiveCharSets();
}


function LoadAvailableCharSets()
{
  var available_charsets = document.getElementById('available_charsets'); 
  var available_charsets_treeroot = document.getElementById('available_charsets_root'); 
  var atom;
  var i;
  var str;
  var tit;
  var visible;

  if (availCharsetList) {
    for (i = 0; i < availCharsetList.Count(); i++) {
      atom = availCharsetList.GetElementAt(i);
      atom = atom.QueryInterface(Components.interfaces.nsIAtom);

      if (atom) {
        str = atom.GetUnicode();
        try {
          tit = ccm.GetCharsetTitle(atom);
        } catch (ex) {
          //don't ignore charset detectors without a title
          tit = str;
        }

        try {                                  
          visible = ccm.GetCharsetData(atom,'.notForBrowser');
          visible = false;
        } catch (ex) {
          visible = true;
        }
      } //atom

      if (str && tit) {
        availCharsetDict[i] = new Array(2);
        availCharsetDict[i][0] = tit;
        availCharsetDict[i][1] = str;
        availCharsetDict[i][2] = visible;
      }
    }
  }
  availCharsetDict.sort();

  if (availCharsetDict) {
    for (i = 0; i < availCharsetDict.length; i++) {
      if (availCharsetDict[i][2]) {
        AddTreeItem(document, 
                    available_charsets_treeroot, 
                    availCharsetDict[i][1], 
                    availCharsetDict[i][0]);
      } //if visible
    }
  }
}


function GetCharSetTitle(id) 
{
  if (availCharsetDict) {
    for (var j = 0; j < availCharsetDict.length; j++) {
      if (availCharsetDict[j][1] == id) {
        return availCharsetDict[j][0];
      }
    }
  }
  return '';
}

function GetCharSetVisibility(id) 
{
  if (availCharsetDict) {
    for (var j = 0; j < availCharsetDict.length; j++) {
      if (availCharsetDict[j][1] == id)
      return availCharsetDict[j][2];
    }
  }
  return false;
}


function AddRemoveLatin1(action) 
{
  var arrayOfPrefs = [];
  arrayOfPrefs = pref_string_content.split(', ');

  if (arrayOfPrefs.length > 0) {
    for (var i = 0; i < arrayOfPrefs.length; i++) {
      if (arrayOfPrefs[i] == 'ISO-8859-1') {
        if (action == 'remove') {
          arrayOfPrefs[i] = arrayOfPrefs[arrayOfPrefs.length-1];
          arrayOfPrefs.length = arrayOfPrefs.length - 1;
        }

        pref_string_content = arrayOfPrefs.join(', ');
        return;
      }
    }

    if (action == 'add') {
      arrayOfPrefs[arrayOfPrefs.length] = 'ISO-8859-1';
      pref_string_content = arrayOfPrefs.join(', ');
    }
  }
}


function LoadActiveCharSets()
{
  var active_charsets = document.getElementById('active_charsets'); 
  var active_charsets_treeroot = document.getElementById('active_charsets_root'); 
  var arrayOfPrefs = [];
  var str;
  var tit;
  var visible;

  arrayOfPrefs = pref_string_content.split(', ');

  if (arrayOfPrefs.length > 0) {
    for (var i = 0; i < arrayOfPrefs.length; i++) {
      str = arrayOfPrefs[i];

      tit = GetCharSetTitle(str);
      visible = GetCharSetVisibility(str);

      if (!tit)
        tit = str; 

      if (str && tit && visible) {
        AddTreeItem(document, active_charsets_treeroot, str, tit);
      } //if 
    } //for
  }
}


function SelectAvailableCharset()
{ 
  //Remove the selection in the active charsets list
  var active_charsets = document.getElementById('active_charsets');

  if (active_charsets.selectedItems.length > 0)
    active_charsets.clearItemSelection();

  enable_add_button();
} //SelectAvailableCharset



function SelectActiveCharset()
{ 
  //Remove the selection in the available charsets list
  var available_charsets = document.getElementById('available_charsets');

  if (available_charsets.selectedItems.length > 0)
      available_charsets.clearItemSelection();

  enable_remove_button();
} //SelectActiveCharset



function enable_remove_button()
{
  var remove_button = document.getElementById('remove_button');

  remove_button.setAttribute('disabled','false');
}

  
function enable_save()
{
  var save_button = document.getElementById('ok');
  save_button.removeAttribute('disabled');
}



function enable_add_button()
{
  var add_button = document.getElementById('add_button');
  add_button.setAttribute('disabled','false');
}



function AddAvailableCharset()
{
  var active_charsets = document.getElementById('active_charsets'); 
  var active_charsets_treeroot = document.getElementById('active_charsets_root'); 
  var available_charsets = document.getElementById('available_charsets'); 
  
  for (var nodeIndex=0; nodeIndex < available_charsets.selectedItems.length;  nodeIndex++) 
  {
    var selItem =  available_charsets.selectedItems[nodeIndex];
    var selRow  =  selItem.firstChild;
    var selCell =  selRow.firstChild;

    var charsetname  = selCell.getAttribute('value');
    var charsetid = selCell.getAttribute('id');
    var already_active = false;

    for (var item = active_charsets_treeroot.firstChild; item != null; item = item.nextSibling) {

      var row  =  item.firstChild;
      var cell =  row.firstChild;
      var active_charsetid = cell.getAttribute('id');

      if (active_charsetid == charsetid) 
      {
        already_active = true;
        break;
      }//if

    }//for

    if (already_active == false) {
      AddTreeItem(document, active_charsets_treeroot, charsetid, charsetname);
    }//if

  }//for

  available_charsets.clearItemSelection();
  enable_save();

} //AddAvailableCharset



function RemoveActiveCharset()
{
  var active_charsets_treeroot = document.getElementById('active_charsets_root'); 
  var tree = document.getElementById('active_charsets');
  var nextNode = null;
  var numSelected = tree.selectedItems.length;
  var deleted_all = false;

  while (tree.selectedItems.length > 0) {
    var selectedNode = tree.selectedItems[0];
    nextNode = selectedNode.nextSibling;
    
    if (!nextNode) {
      if (selectedNode.previousSibling) 
        nextNode = selectedNode.previousSibling;
    }

    var row  =  selectedNode.firstChild;
    var cell =  row.firstChild;

    row.removeChild(cell);
    selectedNode.removeChild(row);
    active_charsets_treeroot.removeChild(selectedNode);
  } //while
  
  if (nextNode) {
    tree.selectItem(nextNode)
  } else {
    //tree.clearItemSelection();
  }
  
  enable_save();
} //RemoveActiveCharset



function Save()
{
  // Iterate through the 'active charsets  tree to collect the charsets
  // that the user has chosen. 

  var active_charsets = document.getElementById('active_charsets'); 
  var active_charsets_treeroot = document.getElementById('active_charsets_root'); 
  
  var row          = null;
  var cell         = null;
  var charsetid    = "";
  var num_charsets = 0;
  var pref_string_content = '';

  for (var item = active_charsets_treeroot.firstChild; item != null; item = item.nextSibling) {

    row  =  item.firstChild;
    cell =  row.firstChild;
    charsetid = cell.getAttribute('id');

    if (charsetid.length > 1) {
      num_charsets++;

      //separate >1 charsets by commas
      if (num_charsets > 1)
        pref_string_content = pref_string_content + ", " + charsetid;
      else
        pref_string_content = charsetid;
    }
  }

  try
  {
    if (prefInt) {
      prefInt.SetCharPref(pref_string_title, pref_string_content);
      window.close();
    }
  }
  catch(ex) {
    confirm('exception' + ex);
  }
} //Save


function MoveUp() {
  var tree = document.getElementById('active_charsets'); 
  if (tree.selectedItems.length == 1) {
    var selected = tree.selectedItems[0];
    var before = selected.previousSibling
    if (before) {
      before.parentNode.insertBefore(selected, before);
      tree.selectItem(selected);
      tree.ensureElementIsVisible(selected);
    }
  }

  enable_save();
} //MoveUp


   
function MoveDown() {
  var tree = document.getElementById('active_charsets');
  if (tree.selectedItems.length == 1) {
    var selected = tree.selectedItems[0];
    if (selected.nextSibling) {
      if (selected.nextSibling.nextSibling) {
        selected.parentNode.insertBefore(selected, selected.nextSibling.nextSibling);
      }
      else {
        selected.parentNode.appendChild(selected);
      }
      tree.selectItem(selected);
    }
  }

  enable_save();
} //MoveDown

function AddTreeItem(doc, treeRoot, ID, UIstring)
{
  // Create a treerow for the new item
  var item = doc.createElement('treeitem');
  var row  = doc.createElement('treerow');
  var cell = doc.createElement('treecell');

  // Copy over the attributes
  cell.setAttribute('value', UIstring);
  cell.setAttribute('id', ID);

  // Add it to the tree
  item.appendChild(row);
  row.appendChild(cell);

  treeRoot.appendChild(item);
}
