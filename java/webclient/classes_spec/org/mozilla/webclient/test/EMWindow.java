/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is RaptorCanvas.
 *
 * The Initial Developer of the Original Code is Kirk Baker and
 * Ian Wilkinson. Portions created by Kirk Baker and Ian Wilkinson are
 * Copyright (C) 1999 Kirk Baker and Ian Wilkinson. All
 * Rights Reserved.
 *
 * Contributor(s): Kirk Baker <kbaker@eb.com>
 *               Ian Wilkinson <iw@ennoble.com>
 *               Mark Goddard
 *               Ed Burns <edburns@acm.org>
 *               Ashutosh Kulkarni <ashuk@eng.sun.com>
 *               Louis-Philippe Gagnon <louis-philippe@macadamian.com>
 *               Jason Mawdsley <jason@macadamian.com>
 */

package org.mozilla.webclient.test;

/*
 * EMWindow.java
 */

import java.awt.*;
import java.awt.event.*;

import javax.swing.tree.TreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.*;

import java.util.Enumeration;
import java.util.Properties;
import java.util.*;

import org.mozilla.webclient.*;
import org.mozilla.util.Assert;

import org.w3c.dom.Document;

import java.io.File;
import java.io.FileInputStream;

/**
 *

 * This is a test application for using the BrowserControl.

 *
 * @version $Id: EMWindow.java,v 1.26 2001/05/08 04:50:27 edburns%acm.org Exp $
 * 
 * @see	org.mozilla.webclient.BrowserControlFactory

 */

public class EMWindow extends Frame implements DialogClient, ActionListener, DocumentLoadListener, MouseListener, Prompt, PrefChangedCallback {
    static final int defaultWidth = 640;
    static final int defaultHeight = 480;

  private int winNum;

    private TextField		urlField;
	private BrowserControl	browserControl;
    private BrowserControlCanvas browserCanvas;

    private Navigation navigation = null;
	private CurrentPage	    currentPage;
	private History	        history;
    private static Preferences     prefs;
	private Bookmarks	    bookmarks;
    private BookmarksFrame bookmarksFrame = null;
	private TreeModel	    bookmarksTree;
    private DOMViewerFrame  domViewer;
	private Panel			controlPanel;
	private Panel			statusPanel;
	private Panel			buttonsPanel;
    private FindDialog           findDialog = null;
private PasswordDialog           passDialog = null;
private UniversalDialog           uniDialog = null;
    private MenuBar             menuBar;
    private Menu                historyMenu;
    private MenuItem backMenuItem;
    private MenuItem forwardMenuItem;
    private HistoryActionListener historyActionListener = null;
    private Menu                bookmarksMenu;
    private Label          statusLabel;
    private String currentURL;

  private Document       currentDocument = null;

  private EmbeddedMozilla creator;

    private Component forwardButton;
    private Component backButton;
    private Component stopButton;
    private Component refreshButton;

    private PopupMenu popup;
    private MenuItem popup_ViewSource, popup_SelectAll;
    private PopupActionListener contextListener;


  public static void main(String [] arg)
    {
    }


    public EMWindow (String title, String binDir, String url, int winnum, EmbeddedMozilla Creator) 
    {
	super(title);
    popup = new PopupMenu();
	creator = Creator;
    currentURL = url;
	winNum = winnum;
        System.out.println("constructed with binDir: " + binDir + " url: " + 
                           url);
		setSize(defaultWidth, defaultHeight);
	
		// Create the Menu Bar
		menuBar = new MenuBar();
		this.setMenuBar(menuBar);
       		Menu fileMenu = new Menu("File");
		Menu viewMenu = new Menu("View");
       		Menu searchMenu = new Menu("Search");
		Menu editMenu = new Menu("Edit");
       		MenuItem newItem = new MenuItem("New Window");
       		MenuItem closeItem = new MenuItem("Close");
       		MenuItem findItem = new MenuItem("Find");
       		MenuItem findNextItem = new MenuItem("Find Next");
		MenuItem sourceItem = new MenuItem("View Page Source as String");
		MenuItem pageInfoItem = new MenuItem("View Page Info");
		MenuItem selectAllItem = new MenuItem("Select All");
        MenuItem copyItem = new MenuItem("Copy");
       		menuBar.add(fileMenu);
		menuBar.add(viewMenu);
       		menuBar.add(searchMenu);
		menuBar.add(editMenu);
       		fileMenu.add(newItem);
       		newItem.addActionListener(this);
       		fileMenu.add(closeItem);
       		closeItem.addActionListener(this);
       		searchMenu.add(findItem);
       		findItem.addActionListener(this);
       		searchMenu.add(findNextItem);
       		findNextItem.addActionListener(this);

        historyMenu = new Menu("History");
          backMenuItem = new MenuItem("Back");
          backMenuItem.addActionListener(this);
          historyMenu.add(backMenuItem);
          forwardMenuItem = new MenuItem("Forward");
          forwardMenuItem.addActionListener(this);
          historyMenu.add(forwardMenuItem);
          menuBar.add(historyMenu);

        bookmarksMenu = new Menu("Bookmarks");
          MenuItem addBookmark = new MenuItem("Add Current Page");
          addBookmark.addActionListener(this);
          bookmarksMenu.add(addBookmark);

          addBookmark = new MenuItem("Add Current Page In New Folder");
          addBookmark.addActionListener(this);
          bookmarksMenu.add(addBookmark);

          MenuItem manageBookmarks = new MenuItem("Manage Bookmarks...");
          manageBookmarks.addActionListener(this);
          bookmarksMenu.add(manageBookmarks);
          menuBar.add(bookmarksMenu);

        Menu streamMenu = new Menu("Stream");
          MenuItem streamFromFile = new MenuItem("Load Stream From File...");
          streamFromFile.addActionListener(this);
          streamMenu.add(streamFromFile);
          MenuItem randomStream = new MenuItem("Load Random HTML InputStream");
          randomStream.addActionListener(this);
          streamMenu.add(randomStream);
          menuBar.add(streamMenu);

		viewMenu.add(sourceItem);
		sourceItem.addActionListener(this);
       		viewMenu.add(pageInfoItem);
       		pageInfoItem.addActionListener(this);
		editMenu.add(selectAllItem);
		selectAllItem.addActionListener(this);
        editMenu.add(copyItem);
        copyItem.addActionListener(this);

		// Create the URL field
		urlField = new TextField("", 30);
        urlField.addActionListener(this);        	
        urlField.setText(url);


		// Create the buttons sub panel
		buttonsPanel = new Panel();
        buttonsPanel.setLayout(new GridBagLayout());

		// Add the buttons
		backButton = makeItem(buttonsPanel, "Back",    0, 0, 1, 1, 0.0, 0.0);
        backButton.setEnabled(false);
		forwardButton = makeItem(buttonsPanel, "Forward", 1, 0, 1, 1, 0.0, 0.0);
        forwardButton.setEnabled(false);
		stopButton = makeItem(buttonsPanel, "Stop",    2, 0, 1, 1, 0.0, 0.0);
        stopButton.setEnabled(false);
		refreshButton = makeItem(buttonsPanel, "Refresh", 3, 0, 1, 1, 0.0, 0.0);
        refreshButton.setEnabled(false);
        makeItem(buttonsPanel, "DOMViewer",    4, 0, 1, 1, 0.0, 0.0);

		// Create the control panel
		controlPanel = new Panel();
        controlPanel.setLayout(new BorderLayout());
        
        // Add the URL field, and the buttons panel
        Panel centerPanel = new Panel();
        centerPanel.setLayout(new BorderLayout());
        centerPanel.add(urlField, BorderLayout.NORTH);

        //		controlPanel.add(urlField,     BorderLayout.CENTER);
		controlPanel.add(centerPanel,     BorderLayout.CENTER);
		controlPanel.add(buttonsPanel, BorderLayout.WEST);

        // create the status panel
        statusPanel = new Panel();
        statusPanel.setLayout(new BorderLayout());

        // create and add the statusLabel
        statusLabel = new Label("", Label.LEFT);
        statusLabel.setBackground(Color.lightGray);
        statusPanel.add(statusLabel, BorderLayout.CENTER);

		// Create the browser
        try {
            BrowserControlFactory.setAppData(binDir);
			browserControl = BrowserControlFactory.newBrowserControl();
            browserCanvas = 
                (BrowserControlCanvas) 
                browserControl.queryInterface(BrowserControl.BROWSER_CONTROL_CANVAS_NAME);

        }
        catch(Exception e) {
            System.out.println("Can't create BrowserControl: " + 
                               e.getMessage());
        }
        Assert.assert(null != browserCanvas);
		browserCanvas.setSize(defaultWidth, defaultHeight);
	
		// Add the control panel and the browserCanvas
		add(controlPanel, BorderLayout.NORTH);
		add(browserCanvas,      BorderLayout.CENTER);
		add(statusPanel,      BorderLayout.SOUTH);

		addWindowListener(new WindowAdapter() {
		    public void windowClosing(WindowEvent e) {
                System.out.println("Got windowClosing");
		System.out.println("destroying the BrowserControl");
		EMWindow.this.delete();
				// should close the BrowserControlCanvas
		creator.DestroyEMWindow(winNum);
		    }
		    
		    public void windowClosed(WindowEvent e) { 
                System.out.println("Got windowClosed");
		    }
		});
	 
        // Create the Context Menus
        add(popup);

        popup.add(popup_ViewSource = new MenuItem("View Source as ByteArray"));
        popup.add(popup_SelectAll = new MenuItem("Select All"));
        
        contextListener = new PopupActionListener();
       
        popup_ViewSource.addActionListener (contextListener);
        popup_SelectAll.addActionListener (contextListener);

		show();
		toFront();

		try {
            navigation = (Navigation)
                browserControl.queryInterface(BrowserControl.NAVIGATION_NAME);
            navigation.setPrompt(this);
            currentPage = (CurrentPage)
                browserControl.queryInterface(BrowserControl.CURRENT_PAGE_NAME);
            history = (History)
                browserControl.queryInterface(BrowserControl.HISTORY_NAME);
            prefs = (Preferences)
                browserControl.queryInterface(BrowserControl.PREFERENCES_NAME);
            prefs.registerPrefChangedCallback(this, 
                                              "network.cookie.warnAboutCookies",
                                              "This IS the Closure!");
            prefs.setPref("network.cookie.warnAboutCookies", "true");
            prefs.setPref("browser.cache.disk_cache_size", "0");
     
            // pull out the proxies, and make java aware of them
            Properties prefsProps = prefs.getPrefs();
            String proxyHost = (String) prefsProps.get("network.proxy.http");
            String proxyPort = (String) prefsProps.get("network.proxy.http_port");
            if (null != proxyHost && null != proxyPort) {
                System.setProperty("http.proxyHost", proxyHost);
                System.setProperty("http.proxyPort", proxyPort);
            }
             
            //prefsProps = prefs.getPrefs();
            //prefsProps.list(System.out);  // This works, try it!
        }
		catch (Exception e) {
		    System.out.println(e.toString());
		}
        
        try {
            EventRegistration eventRegistration = 
                (EventRegistration)
                browserControl.queryInterface(BrowserControl.EVENT_REGISTRATION_NAME);
            eventRegistration.addDocumentLoadListener(this);
            eventRegistration.addMouseListener(this);

            // PENDING(edburns): test code, replace with production code
            bookmarks = 
                (Bookmarks)
                browserControl.queryInterface(BrowserControl.BOOKMARKS_NAME);
            System.out.println("debug: edburns: got Bookmarks instance");

            bookmarksTree = bookmarks.getBookmarks();

            /*********

            TreeNode bookmarksRoot = (TreeNode) bookmarksTree.getRoot();

            System.out.println("debug: edburns: testing the Enumeration");
            int childCount = bookmarksRoot.getChildCount();
            System.out.println("debug: edburns: root has " + childCount + 
                               " children.");

            Enumeration rootChildren = bookmarksRoot.children();
            TreeNode currentChild;

            int i = 0, childIndex;
            while (rootChildren.hasMoreElements()) {
                currentChild = (TreeNode) rootChildren.nextElement();
                System.out.println("debug: edburns: bookmarks root has children! child: " + i + " name: " + currentChild.toString());
                i++;
            }

            System.out.println("debug: edburns: testing getChildAt(" + --i + "): ");
            currentChild = bookmarksRoot.getChildAt(i);
            System.out.println("debug: edburns: testing getIndex(Child " + 
                               i + "): index should be " + i + ".");
            childIndex = bookmarksRoot.getIndex(currentChild);
            System.out.println("debug: edburns: index is: " + childIndex);
            *****/

            /**********


            System.out.println("debug: edburns: got new entry");

            Properties entryProps = entry.getProperties();

            System.out.println("debug: edburns: entry url: " + 
                               entryProps.getProperty(BookmarkEntry.URL));
            bookmarks.addBookmark(folder, entry);
            **********/
        }
		catch (Exception e) {
		    System.out.println(e.toString());
		}
        
        if (null != navigation) {
            navigation.loadURL(url);
        }
    } // EMWindow() ctor

public void delete()
{
    browserCanvas.setVisible(false);
    if (null != bookmarksFrame) {
	bookmarksFrame.setVisible(false);
	bookmarksFrame.dispose();
	bookmarksFrame = null;
    }
    if (null != domViewer) {
	domViewer.setVisible(false);
	domViewer.dispose();
	domViewer = null;
    } 
    BrowserControlFactory.deleteBrowserControl(browserControl);
    browserControl = null;
    this.hide();
    this.dispose();
    urlField = null;
    browserCanvas = null;
    currentPage = null;
    bookmarks = null;
    bookmarksTree = null;
    controlPanel = null;
    buttonsPanel = null;
    currentDocument = null;
}


public void actionPerformed (ActionEvent evt) 
{
    String command = evt.getActionCommand();
    
    try {
        
        if (command.equals("New Window")) {
            creator.CreateEMWindow();
        }
        else if (command.equals("Close")) {
            System.out.println("Got windowClosing");
            System.out.println("destroying the BrowserControl");
            EMWindow.this.delete();
            // should close the BrowserControlCanvas
            creator.DestroyEMWindow(winNum);
        }
        else if (command.equals("Find")) {
            if (null == findDialog) {
                Frame f = new Frame();
                f.setSize(350,150);
                findDialog = new FindDialog(this, this, 
                                            "Find in Page", "Find  ", 
                                            "", 20, false);
                findDialog.setModal(false);
            }
            findDialog.setVisible(true);
            //		currentPage.findInPage("Sun", true, true);
        }
        else if (command.equals("Find Next")) {
            currentPage.findNextInPage();
        }
        else if (command.equals("View Page Source as String")) {
            String sou = currentPage.getSource();
            System.out.println("+++++++++++ Page Source is +++++++++++\n\n" + sou);
        }
        else if (command.equals("View Page Info")) {
            currentPage.getPageInfo();
        }
        else if (command.equals("Select All")) {
            currentPage.selectAll();
        }
        else if (command.equals("Copy")) {
            currentPage.copyCurrentSelectionToSystemClipboard();
        }
	    else if(command.equals("Stop")) {
            navigation.stop();
        }
        else if (command.equals("Refresh")) {
            navigation.refresh(Navigation.LOAD_NORMAL);
        }
        else if (command.equals("Add Current Page")) {
            if (null == bookmarksTree) {
                bookmarksTree = bookmarks.getBookmarks();
            }
            BookmarkEntry entry = 
                bookmarks.newBookmarkEntry(urlField.getText());
            bookmarks.addBookmark(null, entry);
        }        
        else if (command.equals("Add Current Page In New Folder")) {
            if (null == bookmarksTree) {
                bookmarksTree = bookmarks.getBookmarks();
            }
            BookmarkEntry folder = bookmarks.newBookmarkFolder("newFolder");
            bookmarks.addBookmark(null, folder);
            BookmarkEntry entry = 
                bookmarks.newBookmarkEntry(urlField.getText());
            bookmarks.addBookmark(folder, entry);
        }        
        else if (command.equals("Manage Bookmarks...")) {
            if (null == bookmarksTree) {
                bookmarksTree = bookmarks.getBookmarks();
            }
            
            if (null == bookmarksFrame) {
                bookmarksFrame = new BookmarksFrame(bookmarksTree, 
                                                    browserControl);
                bookmarksFrame.setSize(new Dimension(320,480));
                bookmarksFrame.setLocation(defaultWidth + 5, 0);
            }
            bookmarksFrame.setVisible(true);
        }
        else if (command.equals("Load Stream From File...")) {
            FileDialog fileDialog = new FileDialog(this, "Pick an HTML file",
                                                   FileDialog.LOAD);
            fileDialog.show();
            String file = fileDialog.getFile();
            String directory = fileDialog.getDirectory();
            
            if ((null != file) && (null != directory) &&
                (0 < file.length()) && (0 < directory.length())) {
                String absPath = directory + file;
                
                FileInputStream fis = new FileInputStream(absPath);
                File tFile = new File(absPath);
                
                System.out.println("debug: edburns: file: " + absPath);
                
                navigation.loadFromStream(fis, "file:///hello.html",
                                          "text/html", (int) tFile.length(), 
                                          null);
            }
        }        
        else if (command.equals("Load Random HTML InputStream")) {
            RandomHTMLInputStream rhis = new RandomHTMLInputStream(3);
            System.out.println("debug: edburns: created RandomHTMLInputStream");
            navigation.loadFromStream(rhis, "http://randomstream.com/",
                                      "text/html", -1, null);
        }        
        else if (command.equals("DOMViewer")) {
            if (null == domViewer) {
                domViewer = new DOMViewerFrame("DOM Viewer", creator);
                domViewer.setSize(new Dimension(300, 600));
                domViewer.setLocation(defaultWidth + 5, 0);
            }
            if (null != currentDocument) {
                domViewer.setDocument(currentDocument);
                domViewer.setVisible(true);
            }
        }
        else if (command.equals("Back")) {
            if (history.canBack()) {
                history.back();
            }
        }
        else if (command.equals("Forward")) {
            if (history.canForward()) {
                history.forward();
            }
        }
        else if (command.equals(" ")) {
        }
        else {
            navigation.loadURL(urlField.getText());
        }
    }
    catch (Exception e) {
        System.out.println(e.getMessage());
    }
} // actionPerformed()


public void dialogDismissed(Dialog d) {
    if (d == passDialog || d == uniDialog) {
        return;
    }
  if(findDialog.wasClosed()) {
    System.out.println("Find Dialog Closed");
  }
  else {
    String searchString = findDialog.getTextField().getText();
    if(searchString == null) {
      System.out.println("Java ERROR - SearchString not received from Dialog Box" + 
			 searchString);
    }
    else if(searchString.equals("")) {
      System.out.println("Clear button selected");
      try {
          CurrentPage currentPage = (CurrentPage)
              browserControl.queryInterface(BrowserControl.CURRENT_PAGE_NAME);
          currentPage.resetFind();
      }
      catch (Exception e) {
          System.out.println(e.getMessage());
      }
    }
    else {
      System.out.println("Tring to Find String   -  " + searchString);
      System.out.println("Parameters are    - Backwrads = " + findDialog.backwards + " and Matchcase = " + findDialog.matchcase);
      try {
	CurrentPage currentPage = (CurrentPage)
	  browserControl.queryInterface(BrowserControl.CURRENT_PAGE_NAME);
	currentPage.findInPage(searchString, !findDialog.backwards, findDialog.matchcase);
      }
      catch (Exception e) {
	System.out.println(e.getMessage());
      }
    }
  }
}

public void dialogCancelled(Dialog d) {
  System.out.println("Find Dialog Closed");
}
      



private Component makeItem (Panel p, Object arg, int x, int y, int w, int h, double weightx, double weighty)
{
     GridBagLayout gbl = (GridBagLayout) p.getLayout();
     GridBagConstraints c = new GridBagConstraints();
     Component comp = null;
     
     c.fill = GridBagConstraints.BOTH;
     c.gridx = x;
     c.gridy = y;
     c.gridwidth = w;
     c.gridheight = h;
     c.weightx = weightx;
     c.weighty = weighty;
     if (arg instanceof String) {
         Button b;
         
         comp = b = new Button((String) arg);
         b.addActionListener(this);
         
         p.add(comp);
         gbl.setConstraints(comp, c);

         if (((String)arg).equals(" ")) {
             b.setEnabled(false);
         }
     }
     return comp;
} // makeItem()


//
// From WebclientEventListener sub-interfaces
//

/**

 * Important: do not call any webclient methods during this callback.
 * It may caus your app to deadlock.

 */

public void eventDispatched(WebclientEvent event)
{
    boolean enabledState;
    String status;

    if (event instanceof DocumentLoadEvent) {
        switch ((int) event.getType()) {
        case ((int) DocumentLoadEvent.START_DOCUMENT_LOAD_EVENT_MASK):
            stopButton.setEnabled(true);
            refreshButton.setEnabled(true);
            currentURL = (String) event.getEventData();
            System.out.println("debug: edburns: Currently Viewing: " + 
                               currentURL);
            statusLabel.setText("Starting to load " + currentURL);
            urlField.setText(currentURL);
            currentDocument = null;
            break;
        case ((int) DocumentLoadEvent.END_DOCUMENT_LOAD_EVENT_MASK):
            stopButton.setEnabled(false);
            backButton.setEnabled(history.canBack());
            backMenuItem.setEnabled(history.canBack());
            forwardButton.setEnabled(history.canForward());
            forwardMenuItem.setEnabled(history.canForward());
            populateHistoryMenu();
            statusLabel.setText("Done.");
            //            currentDocument = currentPage.getDOM();
            // add the new document to the domViewer
            //            if (null != currentDocument && null != domViewer) {
            //                domViewer.setDocument(currentDocument);
            //            }

            break;
        case ((int) DocumentLoadEvent.PROGRESS_URL_LOAD_EVENT_MASK):
            status = "Status: " + (String) event.getEventData();
            statusLabel.setText(status);
            break;
        case ((int) DocumentLoadEvent.STATUS_URL_LOAD_EVENT_MASK):
            status = "Status: " + (String) event.getEventData();
            statusLabel.setText(status);
            break;
        }
    }
}

/**

 * This method exercises the rest of the history API that isn't
 * exercised elsewhere in the browser.

 */

private void populateHistoryMenu()
{
    int i = 0;
    int histLen = 0;
    int curIndex = 0;
    String curUrl;
    MenuItem curItem;
    historyMenu.removeAll();

    if (null == historyActionListener) {
        historyActionListener = new HistoryActionListener();
        if (null == historyActionListener) {
            return;
        }
    }

    // add back these MenuItems
    historyMenu.add(backMenuItem);
    historyMenu.add(forwardMenuItem);

    // now populate the menu with history items
    histLen = history.getHistoryLength();
    curIndex = history.getCurrentHistoryIndex();
    for (i = 0; i < histLen; i++) {
        // PENDING(put in code to truncate unruly long URLs)
        curUrl = history.getURLForIndex(i);

        // It's important that we prepend the index.  This is used in
        // the actionListener to load by index.

        if (i == curIndex) {
            curUrl = Integer.toString(i) + " * " + curUrl;
        }
        else {
            curUrl = Integer.toString(i) + " " + curUrl;
        }
        curItem = new MenuItem(curUrl);
        curItem.addActionListener(historyActionListener);
        historyMenu.add(curItem);
    }
}

//
// From MouseListener
// 

public void mouseClicked(java.awt.event.MouseEvent e)
{
    int modifiers = e.getModifiers();
     if (0 != (modifiers & InputEvent.BUTTON1_MASK)) {
        System.out.println("Button1 ");
    }
    if (0 != (modifiers & InputEvent.BUTTON2_MASK)) {
        System.out.println("Button2 ");
    }
    if (0 != (modifiers & InputEvent.BUTTON3_MASK)) {
        System.out.println("Button3 ");
	popup.show(this, e.getX(), e.getY());
    }
}

public void mouseEntered(java.awt.event.MouseEvent e)
{
    if (e instanceof WCMouseEvent) {
        WCMouseEvent wcMouseEvent = (WCMouseEvent) e;
        Properties eventProps = 
            (Properties) wcMouseEvent.getWebclientEvent().getEventData();
        if (null == eventProps) {
            return;
        }
        if (e.isAltDown()) {
            System.out.println("Alt ");
        }
        if (e.isControlDown()) {
            System.out.println("Ctrl ");
        }
        if (e.isShiftDown()) {
            System.out.println("Shift ");
        }
        if (e.isMetaDown()) {
            System.out.println("Meta ");
        }
        String href = eventProps.getProperty("href");
        if (null != href) {
            // if it's a relative URL
            if (null != currentURL && -1 == href.indexOf("://")) {
                int lastSlashIndex = currentURL.lastIndexOf('/');
                if (-1 == lastSlashIndex) {
                    href = currentURL + "/" + href;
                }
                else {
                    href = currentURL.substring(0, lastSlashIndex) + "/"+ href;
                }
            }
            statusLabel.setText(href);
        }
    }
}

public void mouseExited(java.awt.event.MouseEvent e)
{
    statusLabel.setText("");
}

public void mousePressed(java.awt.event.MouseEvent e)
{
}

public void mouseReleased(java.awt.event.MouseEvent e)
{
}

//
// Prompt methods
// 

public boolean promptUsernameAndPassword(String dialogTitle,
                                  String text,
                                  String passwordRealm,
                                  int savePassword,
                                  Properties fillThis)
{
    if (null == fillThis) {
        return false;
    }
    if (null == passDialog) {
        if (dialogTitle.equals("")) {
            dialogTitle = "Basic Authentication Test";
        }
        passDialog = new PasswordDialog(this, this, 
                                        dialogTitle, text, passwordRealm, 
                                        20, true, fillThis);
        if (null == passDialog) {
            return false;
        }
        passDialog.setModal(true);
    }
    passDialog.setVisible(true);
    
    return passDialog.wasOk();
}

public boolean universalDialog(String titleMessage,
                               String dialogTitle,
                               String text,
                               String checkboxMsg,
                               String button0Text,
                               String button1Text,
                               String button2Text,
                               String button3Text,
                               String editfield1Msg,
                               String editfield2Msg,
                               int numButtons,
                               int numEditfields,
                               boolean editfield1Password,
                               Properties fillThis)
{
    System.out.println("titleMessage " + titleMessage);
    System.out.println("dialogTitle " + dialogTitle);
    System.out.println("text " + text);
    System.out.println("checkboxMsg " + checkboxMsg);
    System.out.println("button0Text " + button0Text);
    System.out.println("button1Text " + button1Text);
    System.out.println("button2Text " + button2Text);
    System.out.println("button3Text " + button3Text);
    System.out.println("editfield1Msg " + editfield1Msg);
    System.out.println("editfield2Msg " + editfield2Msg);
    System.out.println("numButtons " + numButtons);
    System.out.println("numEditfields " + numEditfields);
    System.out.println("editfield1Password " + editfield1Password);
        
    fillThis.put("editfield1Value", "edit1");
    fillThis.put("editfield2Value", "edit2");
    fillThis.put("checkboxState", "true");
    if (null == fillThis) {
        return false;
    }
    if (null == uniDialog) {
        if (dialogTitle.equals("")) {
            dialogTitle = "Universal Dialog";
        }
        uniDialog = new UniversalDialog(this, this, dialogTitle);
        if (null == uniDialog) {
            return false;
        }
        uniDialog.setParms(titleMessage, dialogTitle, text, checkboxMsg, 
                           button0Text, button1Text, button2Text, 
                           editfield1Msg, editfield2Msg, numButtons, 
                           numEditfields, editfield1Password, fillThis);
        uniDialog.setModal(true);
    }
    
    uniDialog.setVisible(true);

    return true;
}

//
// PrefChangedCallback
//
public int prefChanged(String prefName, Object closure)
{
    System.out.println("prefChanged: " + prefName + " closure: " + closure);
    return 0;
}

class HistoryActionListener implements ActionListener
{

public void actionPerformed(ActionEvent event)
{
    String command = event.getActionCommand();

    if (null == command) {
        return;
    }

    // pull out the leading integer
    Integer index;
    int space = command.indexOf((int)' ');
    if (-1 == space) {
        return;
    }

    index = new Integer(command.substring(0, space));

    EMWindow.this.history.setCurrentHistoryIndex(index.intValue());
}

}

class PopupActionListener implements ActionListener {
public void actionPerformed(ActionEvent event) {
    String command = event.getActionCommand();
    if (command.equals("View Source as ByteArray"))
        {
            System.out.println("I will now View Source");
            byte source[] = EMWindow.this.currentPage.getSourceBytes();
            String sou = new String(source);
            System.out.println("+++++++++++ Page Source is +++++++++++\n\n" + sou);
        }
    else if (command.equals("Select All"))
        {
            System.out.println("I will now Select All");
            EMWindow.this.currentPage.selectAll();
        }
}
}


//
// Package methods
//

Navigation getNavigation()
{
    return navigation;
}

}

// EOF
