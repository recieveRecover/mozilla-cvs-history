// XMLTerm Page Commands

// CONVENTION: All pre-defined XMLTerm Javascript functions
//             begin with an upper letter. This allows
//             an easy distinction with user defined functions,
//             which should begin with a lower case letter.

// Global variables
var altwin;                   // Alternate (browser) window

var tips = new Array();       // Usage tip strings
var tipnames = new Array();   // Usage tip names
var tipCount = 0;             // No. of tips
var selectedTip = 0;          // Selected random tip

// Set prompt using form entry
function DefineTip(tip, name) {
  tips[tipCount] = tip;
  tipnames[tipCount] = name;
  tipCount++;
  return;
}

DefineTip('Click the new tip link to the left to get a new tip!',
          'new-tip');

DefineTip('User level setting (at the top) controls amount of help information',
         'level-setting');

DefineTip('Mode setting controls if double clicking a directory/executable opens a new window',
         'mode-setting');

DefineTip('Icons setting controls whether directory listings use icons',
          'icons-setting');

DefineTip('Single click an explicit (underlined) hyperlink; double click implicit (usually blue) hyperlinks',
         'double-click');

DefineTip('Click the SetPrompt button to use a cool Mozilla prompt from dmoz.org!',
         'set-prompt-moz');

DefineTip('"js:SetPrompt(HTMLstring);" sets prompt to HTML string.',
          'set-prompt');

DefineTip('Type "js:func(arg);" to execute inline Javascript function func.',
          'inline-js');

DefineTip('Inline Javascript ("js:...") can be used to produce HTML output.',
          'js-html');

DefineTip('XMLterm supports full screen commands like "vi", "emacs -nw", and "less".',
         'full-screen');

DefineTip('Use "xls -i" for iconic display of directory contents.',
          'xls-i');

DefineTip('Use "xcat file" to display file.',
          'xcat');

// Display random usage tip
// (should cease to operate after a few cycles;
// need to use Prefs to keep track of that)
function NewTip() {
  var ranval = Math.random();
  selectedTip = Math.floor(ranval * tipCount);
  if (selectedTip >= tipCount) selectedTip = 0;

  dump("xmlterm: NewTip "+selectedTip+","+ranval+"\n");

  var tipdata = document.getElementById('tipdata');
  tipdata.firstChild.data = tips[selectedTip];

  ShowHelp("",0);

  return false;
}

// Explain tip
function ExplainTip(tipName) {
  dump("xmlterm: ExplainTip("+tipName+")\n");

  if (tipName) {
    var tipdata = document.getElementById('tipdata');
    tipdata.firstChild.data = "";
  } else {
    tipName = tipnames[selectedTip];
  }

  ShowHelp('xmltermTips.html#'+tipName,0,120);

  return false;
}

// F1 key - Hide all output
function F1Key(isShift, isControl) {
  return DisplayAllOutput(false);
}

// F2 key - Show all output
function F2Key(isShift, isControl) {
  return DisplayAllOutput(true);
}

// F7 - Explain tip
function F7Key(isShift, isControl) {
  return ExplainTip();
}

// F8 - New tip
function F8Key(isShift, isControl) {
  return NewTip();
}

// F9 key
function F9Key(isShift, isControl) {
  return NewXMLTerm('');
}

// Scroll Home key
function ScrollHomeKey(isShift, isControl) {
  dump("ScrollHomeKey("+isShift+","+isControl+")\n");
  if (isShift && window.xmltbrowser) {
     window.xmltbrowser.scroll(0,0);
  } else {
     window.scroll(0,0);
  }
  return false;
}

// Scroll End key
function ScrollEndKey(isShift, isControl) {
  dump("ScrollEndKey("+isShift+","+isControl+")\n");
  if (isShift && window.xmltbrowser) {
     window.xmltbrowser.scroll(0,9999);
  } else {
     window.scroll(0,9999);
  }
  return false;
}

// Scroll PageUp key
function ScrollPageUpKey(isShift, isControl) {
  dump("ScrollPageUpKey("+isShift+","+isControl+")\n");

  if (isShift && window.xmltbrowser) {
     window.xmltbrowser.scrollBy(0,-300);
  } else {
     window.scrollBy(0,-300);
  }
  return false;
}

// Scroll PageDown key
function ScrollPageDownKey(isShift, isControl) {
  dump("ScrollPageDownKey("+isShift+","+isControl+")\n");
  if (isShift && window.xmltbrowser) {
    window.xmltbrowser.scrollBy(0,300);
  } else {
    window.scrollBy(0,300);
  }
  return false;
}

// Set history buffer size
function SetHistory(value) {
  dump("SetHistory("+value+")\n");
  window.xmlterm.SetHistory(value, document.cookie);
  return (false);
}

// Set prompt
function SetPrompt(value) {
  dump("SetPrompt("+value+")\n");
  window.xmlterm.SetPrompt(value, document.cookie);
  return (false);
}

// Create new XMLTerm window
function NewXMLTerm(firstcommand) {
  newwin = window.openDialog( "chrome://xmlterm/content/xmlterm.xul",
                              "xmlterm", "chrome,dialog=no,resizable",
                              firstcommand);
  //newwin = window.xmlterm.NewXMLTermWindow(firstcommand);
  dump("NewXMLTerm: "+newwin+"\n")
  return (false);
}

// Handle resize events
function Resize(event) {
  dump("Resize()\n");
  window.xmlterm.Resize();
  return (false);
}

// Form Focus (workaround for form input being transmitted to xmlterm)
function FormFocus() {
  dump("FormFocus()\n");
  window.xmlterm.IgnoreKeyPress(true, document.cookie);
  return false;
}

// Form Blur (workaround for form input being transmitted to xmlterm)
function FormBlur() {
  dump("FormBlur()\n");
  window.xmlterm.IgnoreKeyPress(false, document.cookie);
  return false;
}

// Set user level
function UpdateSettings() {

  var oldUserLevel = window.userLevel;
  window.userLevel = document.xmltform1.level.options[document.xmltform1.level.selectedIndex].value;

  var oldShowIcons = window.showIcons;
  window.showIcons = document.xmltform1.icons.options[document.xmltform1.icons.selectedIndex].value;

  window.commandMode = document.xmltform1.mode.options[document.xmltform1.mode.selectedIndex].value;

  dump("UpdateSettings: userLevel="+window.userLevel+"\n");
  dump("UpdateSettings: commandMode="+window.commandMode+"\n");
  dump("UpdateSettings: showIcons="+window.showIcons+"\n");

  if (window.userLevel != oldUserLevel) {
    // Change icon display style in the style sheet

    if (window.userLevel == "advanced") {
      AlterStyle("DIV.beginner",     "display", "none");
      AlterStyle("DIV.intermediate", "display", "none");

    } else if (window.userLevel == "intermediate") {
      AlterStyle("DIV.intermediate", "display", "block");
      AlterStyle("DIV.beginner",     "display", "none");

    } else {
      AlterStyle("DIV.beginner",     "display", "block");
      AlterStyle("DIV.intermediate", "display", "block");
    }
  }

  if (window.showIcons != oldShowIcons) {
    // Change icon display style in the style sheet
    if (window.showIcons == "on") {
      AlterStyle("SPAN.noicons", "display", "none");
      AlterStyle("TR.icons",     "display", "table-row");
      AlterStyle("IMG.icons",    "display", "inline");
    } else {
      AlterStyle("SPAN.noicons", "display", "inline");
      AlterStyle("TR.icons",     "display", "none");
      AlterStyle("IMG.icons",    "display", "none");
    }
  }

  return false;
}

// Alter style in stylesheet of specified document doc, or current document
// if doc is omitted.
function AlterStyle(ruleName, propertyName, propertyValue, doc) {

  dump("AlterStyle("+ruleName+"{"+propertyName+":"+propertyValue+"})\n");

  if (!doc) doc = window.document;

  var sheet = doc.styleSheets[0];
  var r;
  for (r = 0; r < sheet.cssRules.length; r++) {
    //dump(sheet.cssRules[r].selectorText+"\n");

    if (sheet.cssRules[r].selectorText == ruleName) {
      var style = sheet.cssRules[r].style;
      //dump("style="+style.getPropertyValue(propertyName)+"\n");

      style.setProperty(propertyName,propertyValue,"");
    }
  }
  return false;
}

// Handle default meta command
function MetaDefault(arg1) {
  return Load("http://"+arg1);
}

// Load URL in XMLTermBrowser window
function Load(url) {
  var succeeded = false;
  if (window.xmltbrowser) {
     dump("Load:xmltbrowser.location.href="+window.xmltbrowser.location.href+"\n");
     if (window.xmltbrowser.location.href.length) {
        window.xmltbrowser.location = url;
        succeeded = true;
     }
  }
  if (!succeeded) {
     window.xmltbrowser = window.open(url, "xmltbrowser");
  }

  // Save browser window object in global variable
  altwin = window.xmltbrowser;

  return (false);
}

// Control display of all output elements
function DisplayAllOutput(flag) {
  var outputElements = document.getElementsByName("output");
  for (i=0; i<outputElements.length-1; i++) {
    var outputElement = outputElements[i];
    outputElement.style.display = (flag) ? "block" : "none";
  }

  var promptElements = document.getElementsByName("prompt");
  for (i=0; i<promptElements.length-1; i++) {
    promptElement = promptElements[i];
    promptElement.style.setProperty("text-decoration",
                                     (flag) ? "none" : "underline", "")
  }

  if (!flag)
    ScrollHomeKey(0,0);

  return (false);
}

// Centralized event handler
// eventType values:
//   click
//
// targetType: 
//   textlink  - hyperlink
//   prompt    - command prompt
//   command   - command line
//   exec      - execute command with sendln/createln
//               (depending upon window.commandMode)
//   send      - transmit arg to LineTerm
//   sendln    - transmit arg+newline to LineTerm
//   createln  - transmit arg+newline as first command to new XMLTerm
//
// entryNumber: >=0 means process only if current entry
//              <0  means process anytime
//
// arg1:    command/pathname string (without newline)
// arg2:    alternate command line (for use in current entry only;
//            uses relative pathnames assuming current working directory)
//
function HandleEvent(eventObj, eventType, targetType, entryNumber,
                     arg1, arg2) {
  dump("HandleEvent("+eventObj+","+eventType+","+targetType+","+
                   entryNumber+","+arg1+","+arg2+")\n");

  // Entry independent targets
  if (action === "textlink") {
    // Single click opens hyperlink
    // Browser-style
    dump("textlink = "+arg1+"\n");
    Load(arg1);

  } else if (targetType === "prompt") {
    // Single click on prompt expands/collapses command output
    var outputElement = document.getElementById("output"+entryNumber);
    var helpElement = document.getElementById("help"+entryNumber);
    var promptElement = document.getElementById("prompt"+entryNumber);
    //dump(promptElement.style.getPropertyValue("text-decoration"));

    if (outputElement.style.display == "none") {
      outputElement.style.display = "block";
      promptElement.style.setProperty("text-decoration","none","");

    } else {
      outputElement.style.display = "none";
      promptElement.style.setProperty("text-decoration","underline","");
      if (helpElement) {
        ShowHelp("",entryNumber);
      }
    }

  } else if (eventType === "click") {

     dump("clickCount="+eventObj.clickCount+"\n");

     var dblClick = (eventObj.clickCount > 1);

     // Execute shell commands only on double-click for safety
     // Use single click for "selection" and prompt expansion only
     // Windows-style
     if (!dblClick)
        return false;

     if (targetType === "command") {
       var commandElement = document.getElementById(targetType+entryNumber);
       var command = commandElement.firstChild.data;
       window.xmlterm.SendText("\025"+command+"\n", document.cookie);

     } else {
       // Targets which may be qualified only for current entry

       if ((entryNumber >= 0) &&
         (window.xmlterm.currentEntryNumber != entryNumber)) {
         dump("NOT CURRENT COMMAND\n");
         return (false);
       }

       var action = targetType;

       if (action === "exec") {
          if (window.commandMode === "window") {
             action = "createln";
          } else {
             action = "sendln";
         }
       }

       if (action === "send") {
         dump("send = "+arg1+"\n");
         window.xmlterm.SendText(arg1, document.cookie);

       } else if (action === "sendln") {

         if ((Math.abs(entryNumber)+1 == window.xmlterm.currentEntryNumber) &&
          (arg2 != null)) {
            // Current command
            dump("sendln = "+arg2+"\n\n");
            window.xmlterm.SendText("\025"+arg2+"\n", document.cookie);
         } else {
            // Not current command
            dump("sendln = "+arg1+"\n\n");
            window.xmlterm.SendText("\025"+arg1+"\n", document.cookie);
         }

       } else if (action === "createln") {
         dump("createln = "+arg1+"\n\n");
         newwin = NewXMLTerm(arg1+"\n");
       }
    }
  }

  return (false);
}

// Set history buffer count using form entry
function SetHistoryValue() {
  var field = document.getElementById('InputValue');
  return SetHistory(field.value);
}

// Set prompt using form entry
function SetPromptValue() {
  var field = document.getElementById('InputValue');
  return SetPrompt(field.value);
}

// Insert help element displaying URL in an IFRAME before output element
// of entryNumber, or before the SESSION element if entryNumber is zero.
// Height is the height of the IFRAME in pixels.
// If URL is the null string, simply delete the help element

function ShowHelp(url, entryNumber, height) {

  if (!height) height = 120;

  dump("xmlterm: ShowHelp("+url+","+entryNumber+","+height+")\n");

  if (entryNumber) {
    beforeID = "output"+entryNumber;
    helpID = "help"+entryNumber;
  } else {
    beforeID = "session";
    helpID = "help";
  }

  var beforeElement = document.getElementById(beforeID);

  if (!beforeElement) {
    dump("InsertIFrame: beforeElement ID="+beforeID+"not found\n");
    return false;
  }

  var parentNode = beforeElement.parentNode;

  var helpElement = document.getElementById(helpID);
  if (helpElement) {
    // Delete help element
    parentNode.removeChild(helpElement);
    helpElement = null;
    // *NOTE* Need to flush display here to avoid black flash?
  }

  if (url.length > 0) {
    // Create new help element
    helpElement = document.createElement("div");
    helpElement.setAttribute('id', helpID);
    helpElement.setAttribute('class', 'help');

    var closeElement = document.createElement("span");
    closeElement.setAttribute('class', 'helplink');
    closeElement.appendChild(document.createTextNode("Close help frame"));
    //closeElement.appendChild(document.createElement("p"));

    var iframe = document.createElement("iframe");
    iframe.setAttribute('id', helpID+'frame');
    iframe.setAttribute('class', 'helpframe');
    iframe.setAttribute('width', '100%');
    iframe.setAttribute('height', height);
    iframe.setAttribute('frameborder', '0');
    iframe.setAttribute('src', url);

    helpElement.appendChild(iframe);
    helpElement.appendChild(closeElement);

    dump(helpElement);

    // Insert help element
    parentNode.insertBefore(helpElement, beforeElement);

    // NOTE: Need to do this *after* node is inserted into document
    closeElement.setAttribute('onClick', 'return ShowHelp("",'+entryNumber
                                          +');');
  }

  return false;
}

// About XMLTerm
function AboutXMLTerm() {
  dump("xmlterm: AboutXMLTerm\n");

  var tipdata = document.getElementById('tipdata');
  tipdata.firstChild.data = "";

  ShowHelp('xmltermAbout.html',0,120);

  return false;
}

// onLoad event handler
function LoadHandler() {
  dump("xmlterm: LoadHandler ... "+window.xmlterm+"\n");

  // Update settings
  UpdateSettings();

  NewTip();

  if (window.xmlterm) {
     // XMLTerm already initialized
     return (false);
  }

  dump("LoadHandler: WINDOW.ARGUMENTS="+window.arguments+"\n");

  dump("Trying to make an XMLTerm Shell through the component manager...\n");

  var xmltshell = Components.classes["component://mozilla/xmlterm/xmltermshell"].createInstance();

  dump("Interface xmltshell1 = " + xmltshell + "\n");

  xmltshell = xmltshell.QueryInterface(Components.interfaces.mozIXMLTermShell);
  dump("Interface xmltshell2 = " + xmltshell + "\n");

  if (!xmltshell) {
    dump("Failed to create XMLTerm shell\n");
    window.close();
    return;
  }

  // Store the XMLTerm shell in the window
  window.xmlterm = xmltshell;

  // Content window same as current window
  var contentWindow = window;

  // Initialize XMLTerm shell in content window with argvals
  window.xmlterm.Init(contentWindow, "", "");

  //dump("LoadHandler:"+document.cookie+"\n");

  dump("contentWindow="+contentWindow+"\n");
  dump("document="+document+"\n");
  dump("documentElement="+document.documentElement+"\n");

  // Handle resize events
  //contentWindow.addEventListener("onresize", Resize);
  contentWindow.onresize = Resize;

  // Set focus to appropriate frame
  contentWindow.focus();

  //contentWindow.xmlterm = xmlterm;

  //dump(contentWindow.xmlterm);

  // The following code is for testing IFRAMEs only
  dump("[Main] "+window+"\n");
  dump(window.screenX+", "+window.screenY+"\n");
  dump(window.scrollX+", "+window.scrollY+"\n");
  dump(window.pageXOffset+", "+window.pageYOffset+"\n");

  dump("IFRAME checks\n");
  var iframe = document.getElementById('iframe1');

  dump("iframe="+iframe+"\n");

  frames=document.frames;
  dump("frames="+frames+"\n");
  dump("frames.length="+frames.length+"\n");

  framewin = frames[0];

  dump("framewin="+framewin+"\n");
  dump("framewin.document="+framewin.document+"\n");

  dump(framewin.screenX+", "+framewin.screenY+"\n");
  dump(framewin.scrollX+", "+framewin.scrollY+"\n");
  dump(framewin.pageXOffset+", "+framewin.pageYOffset+"\n");

  var body = framewin.document.getElementsByTagName("BODY")[0];
  dump("body="+body+"\n");

  var height= body.scrollHeight;
  dump("height="+height+"\n");

//        iframe.height = 800;
//        iframe.width = 700;

//        framewin.sizeToContent();

  framewin.xmltshell = xmltshell;
  dump(framewin.xmltshell+"\n");

  dump("xmlterm: LoadHandler completed\n");
  return (false);
}

