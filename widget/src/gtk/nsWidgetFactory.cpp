/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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
#include "nsIButton.h"
#include "nsITextWidget.h"
#include "nsWidgetsCID.h"

#include "nsToolkit.h"
#include "nsWindow.h"
#include "nsAppShell.h"
#include "nsButton.h"
#include "nsScrollbar.h"
#include "nsCheckButton.h"
#include "nsRadioButton.h"
#include "nsTextWidget.h"
#include "nsTextAreaWidget.h"
#include "nsFileWidget.h"
#include "nsFileSpecWithUIImpl.h"
#include "nsListBox.h"
#include "nsComboBox.h"
#include "nsLookAndFeel.h"
#include "nsLabel.h"
#ifdef LOSER
#include "nsMenuBar.h"
#include "nsMenu.h"
#include "nsMenuItem.h"
#include "nsPopUpMenu.h"
#include "nsContextMenu.h"
#endif
#include "nsFontRetrieverService.h"

// Drag & Drop, Clipboard
#include "nsClipboard.h"
#include "nsTransferable.h"
#include "nsXIFFormatConverter.h"
#include "nsDragService.h"

#include "nsSound.h"

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
#if 0
static NS_DEFINE_IID(kCMenuBar,       NS_MENUBAR_CID);
static NS_DEFINE_IID(kCMenu,          NS_MENU_CID);
static NS_DEFINE_IID(kCMenuItem,      NS_MENUITEM_CID);
static NS_DEFINE_IID(kCPopUpMenu,     NS_POPUPMENU_CID);
static NS_DEFINE_IID(kCContextMenu,   NS_CONTEXTMENU_CID);
#endif
static NS_DEFINE_IID(kCFontRetrieverService,    NS_FONTRETRIEVERSERVICE_CID);

// Drag & Drop, Clipboard
static NS_DEFINE_IID(kCDataObj,       NS_DATAOBJ_CID);
static NS_DEFINE_IID(kCClipboard,     NS_CLIPBOARD_CID);
static NS_DEFINE_IID(kCTransferable,  NS_TRANSFERABLE_CID);
static NS_DEFINE_IID(kCDataFlavor,    NS_DATAFLAVOR_CID);
static NS_DEFINE_IID(kCXIFFormatConverter,  NS_XIFFORMATCONVERTER_CID);
static NS_DEFINE_IID(kCDragService,   NS_DRAGSERVICE_CID);

static NS_DEFINE_IID(kISupportsIID,   NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIFactoryIID,    NS_IFACTORY_IID);

// Sound services (just Beep for now)
static NS_DEFINE_CID(kCSound,   NS_SOUND_CID);
static NS_DEFINE_CID(kCFileSpecWithUI,   NS_FILESPECWITHUI_CID);


class nsWidgetFactory : public nsIFactory
{   
public:   

    NS_DECL_ISUPPORTS

    NS_DECL_NSIFACTORY

    nsWidgetFactory(const nsCID &aClass);   
    virtual ~nsWidgetFactory();   
private:
  nsCID mClassID;

};   



nsWidgetFactory::nsWidgetFactory(const nsCID &aClass)   
{   
    NS_INIT_ISUPPORTS();
    mClassID = aClass;
}   

nsWidgetFactory::~nsWidgetFactory()   
{   
}   

NS_IMPL_ISUPPORTS(nsWidgetFactory, NS_GET_IID(nsIFactory))

nsresult nsWidgetFactory::CreateInstance(nsISupports *aOuter,  
                                          const nsIID &aIID,  
                                          void **aResult)  
{  
    if (aResult == NULL) {  
        return NS_ERROR_NULL_POINTER;  
    }  

    *aResult = NULL;  

    if (nsnull != aOuter)
      return NS_ERROR_NO_AGGREGATION;
  
    nsISupports *inst = nsnull;
    if (mClassID.Equals(kCWindow)) {
      inst = (nsISupports *)new nsWindow();
    }
    else if (mClassID.Equals(kCChild)) {
      inst = (nsISupports *)new ChildWindow();
    }
    else if (mClassID.Equals(kCButton)) {
      inst = (nsISupports*)(nsWidget *)new nsButton();
    }
    else if (mClassID.Equals(kCCheckButton)) {
        inst = (nsISupports*)(nsWidget *)new nsCheckButton();
    }
    else if (mClassID.Equals(kCCombobox)) {
        inst = (nsISupports*)(nsWidget *)new nsComboBox();
    }
    else if (mClassID.Equals(kCRadioButton)) {
        inst = (nsISupports*)(nsWidget *)new nsRadioButton();
    }
    else if (mClassID.Equals(kCFileOpen)) {
        inst = (nsISupports*)new nsFileWidget();
    }
    else if (mClassID.Equals(kCListbox)) {
        inst = (nsISupports*)(nsWidget *)new nsListBox();
    }
    else if (mClassID.Equals(kCHorzScrollbar)) {
        inst = (nsISupports*)(nsWidget *)new nsScrollbar(PR_FALSE);
    }
    else if (mClassID.Equals(kCVertScrollbar)) {
        inst = (nsISupports*)(nsWidget *)new nsScrollbar(PR_TRUE);
    }
    else if (mClassID.Equals(kCTextArea)) {
        inst = (nsISupports*)(nsWidget *)new nsTextAreaWidget();
    }
    else if (mClassID.Equals(kCTextField)) {
        inst = (nsISupports*)(nsWidget *)new nsTextWidget();
    }
    else if (mClassID.Equals(kCAppShell)) {
        inst = (nsISupports*)new nsAppShell();
    }
    else if (mClassID.Equals(kCToolkit)) {
        inst = (nsISupports*)(nsWidget *)new nsToolkit();
    }
    else if (mClassID.Equals(kCLookAndFeel)) {
        inst = (nsISupports*)(nsWidget *)new nsLookAndFeel();
    }
    else if (mClassID.Equals(kCLabel)) {
        inst = (nsISupports*)(nsWidget *)new nsLabel();
    }
#if 0
    else if (mClassID.Equals(kCMenuBar)) {
        inst = (nsISupports*)(nsIMenuBar *)new nsMenuBar();
    }
    else if (mClassID.Equals(kCMenu)) {
        inst = (nsISupports*)(nsIMenu *)new nsMenu();
    }
    else if (mClassID.Equals(kCMenuItem)) {
        inst = (nsISupports*)(nsIMenuItem *)new nsMenuItem();
    }
    else if (mClassID.Equals(kCPopUpMenu)) {
        inst = (nsISupports*)new nsPopUpMenu();
    }
    /*
    else if (mClassID.Equals(kCContextMenu)) {
        inst = (nsISupports*)(nsIContextMenu*)new nsContextMenu();
    }
    */
#endif
    else if (mClassID.Equals(kCSound)) {
    	nsISound* aSound = nsnull;
    	NS_NewSound(&aSound);
        inst = (nsISupports*) aSound;
    }
    else if (mClassID.Equals(kCTransferable)) {
        inst = (nsISupports*)new nsTransferable();
    }
    else if (mClassID.Equals(kCClipboard)) {
        inst = (nsISupports*)new nsClipboard();
    }
    else if (mClassID.Equals(kCXIFFormatConverter))
        inst = (nsISupports*)new nsXIFFormatConverter();
    else if (mClassID.Equals(kCFontRetrieverService))
        inst = (nsISupports*)(nsIFontRetrieverService *) new nsFontRetrieverService();
    else if (mClassID.Equals(kCDragService))
        inst = (nsISupports*) (nsIDragService *) new nsDragService();
    else if (mClassID.Equals(kCFileSpecWithUI))
    	inst = (nsISupports*) (nsIFileSpecWithUI *) new nsFileSpecWithUIImpl;
    else {
        printf("nsWidgetFactory::CreateInstance(), unhandled class.\n");
    }
  
    if (inst == NULL) {  
        return NS_ERROR_OUT_OF_MEMORY;  
    }  

    NS_ADDREF(inst);
    nsresult res = inst->QueryInterface(aIID, aResult);
    NS_RELEASE(inst);

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


