/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */

#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsWidgetsCID.h"

// includes for our specific widgets
#include "nsWindow.h"
#include "nsButton.h"
#include "nsCheckButton.h"
#include "nsComboBox.h"
#include "nsRadioButton.h"
#include "nsFileWidget.h"
#include "nsListBox.h"
#include "nsScrollBar.h"
#include "nsTextAreaWidget.h"
#include "nsTextWidget.h"
#include "nsAppShell.h"
#include "nsToolkit.h"
#include "nsLookAndFeel.h"
#include "nsLabel.h"
#ifdef LOSER
#include "nsMenuBar.h"
#include "nsMenu.h"
#include "nsMenuItem.h"
#include "nsPopUpMenu.h"
#endif

static NS_DEFINE_IID(kCWindow,        NS_WINDOW_CID);
static NS_DEFINE_IID(kCChild,         NS_CHILD_CID);
static NS_DEFINE_IID(kCButton,        NS_BUTTON_CID);
static NS_DEFINE_IID(kCCheckButton,   NS_CHECKBUTTON_CID);
static NS_DEFINE_IID(kCCombobox,      NS_COMBOBOX_CID);
static NS_DEFINE_IID(kCFileOpen,      NS_FILEWIDGET_CID);
static NS_DEFINE_IID(kCListbox,       NS_LISTBOX_CID);
static NS_DEFINE_IID(kCRadioButton,   NS_RADIOBUTTON_CID);
static NS_DEFINE_IID(kCHorzScrollbar, NS_HORZSCROLLBAR_CID);
static NS_DEFINE_IID(kCVertScrollbar, NS_VERTSCROLLBAR_CID);
static NS_DEFINE_IID(kCTextArea,      NS_TEXTAREA_CID);
static NS_DEFINE_IID(kCTextField,     NS_TEXTFIELD_CID);
static NS_DEFINE_IID(kCAppShell,      NS_APPSHELL_CID);
static NS_DEFINE_IID(kCToolkit,       NS_TOOLKIT_CID);
static NS_DEFINE_IID(kCLookAndFeel,   NS_LOOKANDFEEL_CID);
static NS_DEFINE_IID(kCLabel,         NS_LABEL_CID);
#ifdef LOSER
static NS_DEFINE_IID(kCMenuBar,       NS_MENUBAR_CID);
static NS_DEFINE_IID(kCMenu,          NS_MENU_CID);
static NS_DEFINE_IID(kCMenuItem,      NS_MENUITEM_CID);
static NS_DEFINE_IID(kCPopUpMenu,     NS_POPUPMENU_CID);
#endif

static NS_DEFINE_IID(kCImageButton,   NS_IMAGEBUTTON_CID);
static NS_DEFINE_IID(kISupportsIID,   NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID,    NS_IFACTORY_IID);

class nsWidgetFactory : public nsIFactory
{   
public:   
  // nsISupports methods   
  NS_DECL_ISUPPORTS
  
  // nsIFactory methods   
  NS_IMETHOD CreateInstance(nsISupports *aOuter,   
			    const nsIID &aIID,   
			    void **aResult);   
  
  NS_IMETHOD LockFactory(PRBool aLock);   
  
  nsWidgetFactory(const nsCID &aClass);   
  virtual ~nsWidgetFactory();   
  
private:   
  nsCID mClassID;
}; 

NS_IMPL_ADDREF(nsWidgetFactory)
NS_IMPL_RELEASE(nsWidgetFactory)

nsWidgetFactory::nsWidgetFactory(const nsCID &aClass)
{
  NS_INIT_REFCNT();
  mClassID = aClass;
}

nsWidgetFactory::~nsWidgetFactory()
{
}

nsresult nsWidgetFactory::QueryInterface(const nsIID &aIID,
                                         void **aResult)
{
  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;   
  }

  // Always NULL result, in case of failure
  *aResult = NULL;

  if (aIID.Equals(kISupportsIID)) {
    *aResult = (void *)(nsISupports*)this;
  } else if (aIID.Equals(kIFactoryIID)) {
    *aResult = (void *)(nsIFactory*)this;
  }

  if (*aResult == NULL) {
    return NS_NOINTERFACE;
  }

  NS_ADDREF_THIS(); // Increase reference count for caller
  return NS_OK;
}  


nsresult nsWidgetFactory::CreateInstance(nsISupports* aOuter,
                                          const nsIID &aIID,  
                                          void **aResult)  
{
  if (aResult == NULL) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = NULL;
  if (nsnull != aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsISupports *inst = nsnull;
  if (mClassID.Equals(kCWindow)) {
    inst = (nsISupports*)new nsWindow();
  }
  else if (mClassID.Equals(kCChild)) {
    inst = (nsISupports*)new ChildWindow();
  }
  else if (mClassID.Equals(kCButton)) {
    inst = (nsISupports*)(nsWindow*)new nsButton();
  }
  else if (mClassID.Equals(kCCheckButton)) {
    inst = (nsISupports*)(nsWindow*)new nsCheckButton();
  }
  else if (mClassID.Equals(kCCombobox)) {
    inst = (nsISupports*)(nsWindow*)new nsComboBox();
  }
  else if (mClassID.Equals(kCRadioButton)) {
    inst = (nsISupports*)(nsWindow*)new nsRadioButton();
  }
  else if (mClassID.Equals(kCFileOpen)) {
    inst = (nsISupports*)new nsFileWidget();
  }
  else if (mClassID.Equals(kCListbox)) {
    inst = (nsISupports*)(nsWindow*)new nsListBox();
  }
  else if (mClassID.Equals(kCHorzScrollbar)) {
    inst = (nsISupports*)(nsWindow*)new nsScrollbar(PR_FALSE);
  }
  else if (mClassID.Equals(kCVertScrollbar)) {
    inst = (nsISupports*)(nsWindow*)new nsScrollbar(PR_TRUE);
  }
  else if (mClassID.Equals(kCTextArea)) {
    inst = (nsISupports*)(nsWindow*)new nsTextAreaWidget();
  }
  else if (mClassID.Equals(kCTextField)) {
    inst = (nsISupports*)(nsWindow*)new nsTextWidget();
  }
  else if (mClassID.Equals(kCAppShell)) {
    inst = (nsISupports*)new nsAppShell();
  }
  else if (mClassID.Equals(kCToolkit)) {
    inst = (nsISupports*)new nsToolkit();
  }
  else if (mClassID.Equals(kCLookAndFeel)) {
    inst = (nsISupports*)new nsLookAndFeel();
  }
  else if (mClassID.Equals(kCLabel)) {
    inst = (nsISupports*)(nsWindow*)new nsLabel();
  }
#ifdef LOSER
  else if (mClassID.Equals(kCMenuBar)) {
    inst = (nsISupports*)(nsIMenuBar*) new nsMenuBar();
  }
  else if (mClassID.Equals(kCMenu)) {
    inst = (nsISupports*)(nsIMenu*) new nsMenu();
  }
  else if (mClassID.Equals(kCMenuItem)) {
    inst = (nsISupports*)(nsIMenuItem*) new nsMenuItem();
  }
  else if (mClassID.Equals(kCPopUpMenu)) {
    inst = (nsISupports*)new nsPopUpMenu();
  }
#endif
  if (inst == NULL) {  
    return NS_ERROR_OUT_OF_MEMORY;  
  }  
  
  nsresult res = inst->QueryInterface(aIID, aResult);
  
  if (res != NS_OK) {  
    // We didn't get the right interface, so clean up  
    delete inst;  
  }
  
  return res;  
}

nsresult nsWidgetFactory::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.
  return NS_OK;
} 

// return the proper factory to the caller
extern "C" NS_WIDGET nsresult
NSGetFactory(nsISupports* serviceMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aFactory = new nsWidgetFactory(aClass);
  
  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  return (*aFactory)->QueryInterface(kIFactoryIID, (void**)aFactory);
}
