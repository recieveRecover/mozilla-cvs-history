/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ
//	BrowserClasses.cp
// ΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡΡ

#include "BrowserClasses.h"

// ₯₯₯ PowerPlant Classes
	#include <URegistrar.h>
	#include <LActiveScroller.h>
	#include <LButton.h>
	#include <LCaption.h>
	#include <LDialogBox.h>
	#include <LEditField.h>
	#include <LGroupBox.h>
	#include <LIconPane.h>
	#include <LListBox.h>
	#include <LPicture.h>
	#include <LPlaceHolder.h>
	#include <LPrintout.h>
	#include <LScroller.h>
	#include <LStdControl.h>
	#include <LTable.h>
	#include <LTextEdit.h>
	#include <LWindow.h>
	#include <LRadioGroup.h>
	#include <LTabGroup.h>
	#include <LTextColumn.h>
	#include <LSubOverlapView.h>
	
// ₯₯₯ AM wrappers classes and their AM implementations
	#include <LStaticText.h>
	#include <LAMStaticTextImp.h>
	#include <LIconControl.h>
	#include <LProgressBar.h>
	#include <LPushButton.h>
	#include <LAMPushButtonImp.h>
	#include <LBevelButton.h>
	#include <LCmdBevelButton.h>
	#include <LCheckbox.h>
	#include <LRadioButton.h>
	#include <LTextGroupBox.h>
	#include <LEditText.h>
	#include <LAMEditTextImp.h>
	#include <LPopupButton.h>
	#include <LAMPopupButtonImp.h>
	
// ₯₯₯ Things that will go away after appearance conversion complete
	#include "CProgressBar.h"
	#include "CPatternBevelView.h"
	#include "CBevelButton.h"
	#include "CPatternButtonPopup.h"
	#include "CPatternProgressBar.h"
	#include "CPatternPane.h"
	#include "CGABorderPane.h"
		
// ₯₯₯ PowerPlant Grayscale Classes
	#include <UGALibRegistry.h>

// ₯₯₯ General Purpose UI Classes
	#include "CToolbarBevelButton.h"
	#include "CPatternButton.h"	
	#include "CAMSavvyBevelView.h"

	#include "CNavigationButtonPopup.h"
	#include "CCloseAllAttachment.h"
	#include "CColorEraseAttachment.h"
				
	#include "CIncludeView.h"
	#include "CPlaceHolderView.h"
	#include "COffscreenCaption.h"
	#include "CClusterView.h"
	#include "CTabSwitcher.h"
	#include "CPatternTabControl.h"	
	#include "CTaskBarView.h"
	#include "LTableHeader.h"
	#include "LTableViewHeader.h"
	#include "CSimpleDividedView.h"
	#include "CKeyScrollAttachment.h"	
	#include "CToolTipAttachment.h"	
	#include "CDynamicTooltips.h"
	#include "CPaneEnabler.h"
	#include "CStringListToolTipPane.h"	
	#include "CSaveProgress.h"	
	#include "CScrollerWithArrows.h"
	#include "CProgressCaption.h"
	
#ifdef MOZ_MAIL_NEWS
	#include "CBiffButtonAttachment.h"
	#include "CSingleLineEditField.h"
#endif
	
// ₯₯₯ Browser Specific UI Classes
	
	#include "CDragBar.h"
	#include "CDragBarContainer.h"
	#include "CDragBarDockControl.h"
	#include "CPatternedGrippyPane.h"
	#include "CDividerGrippyPane.h"
	#include "CSwatchBrokerView.h"
	#include "CToolbarDragBar.h"
	#include "CToolbarPatternBevelView.h"
	#include "CProxyPane.h"
	#include "CProxyCaption.h"
	#include "PopupBox.h"
	#include "CPersonalToolbarTable.h"
	#include "CNavCenterWindow.h"
	#include "CNavCenterSelectorPane.h"
	#include "CNavCenterContextMenuAtt.h"
	#include "CNavCenterTitle.h"
	#include "CInlineEditField.h"

	#include "CConfigActiveScroller.h"
	#include "CTSMEditField.h"
//	#include "VEditField.h"
//	#include "CSimpleTextView.h"
	
	#include "CDownloadProgressWindow.h"
	#include "CURLEditField.h"
	#include "CAutoCompleteURLEditField.h"
	#include "CURLCaption.h"
	#include "CHyperScroller.h"
	#include "CButtonEnablerReloadStop.h"

	#include "CBrowserWindow.h"
	#include "CHTMLView.h"
	#include "CBrowserView.h"

	#include "CSpinningN.h"
	#include "CBrowserSecurityButton.h"
	#include "CMiniSecurityButton.h"

	#include "mprint.h"
	#include "macgui.h"
	#include "findw.h"
//	#include "prefw.h"

	#include "BookmarksDialogs.h"
	#include "mplugin.h"
	#include "divview.h"
//	#include "mattach.h"
	#include "UFormElementFactory.h"
	
	#include "CMenuTable.h"
	#include "CPrefsMediator.h"
	#include "CAssortedMediators.h"

#if defined (JAVA)
	#include "mjava.h"
#endif
	
	#include "CEditorWindow.h"

//	#include "mhistory.h"
	#include "CContextMenuAttachment.h"
	
	#include "CHyperTreeFlexTable.h"
	#include "CRDFCoordinator.h"
	#include "CHyperTreeHeader.h"

//-----------------------------------
void RegisterAllBrowserClasses(void)
//-----------------------------------
{
	// AutoRegister classes
	
	RegisterClass_(CProxyPane);
	RegisterClass_(CProxyCaption);
	RegisterClass_(CCloseAllAttachment);
	RegisterClass_(CColorEraseAttachment);
	
	RegisterClass_(LSubOverlapView);

	// ₯₯₯ PowerPlant Classes
	RegisterClass_(LButton);
	RegisterClass_(LCaption);
	RegisterClass_(LDialogBox);
	RegisterClass_(LEditField);
	RegisterClass_(LListBox);
	RegisterClass_(LPane);
	RegisterClass_(LPicture);
	RegisterClass_(LPlaceHolder);
	RegisterClass_(LPrintout);
	RegisterClass_(LScroller);
	RegisterClass_(LStdControl);
	RegisterClass_(LStdButton);
	RegisterClass_(LStdCheckBox);
	RegisterClass_(LStdRadioButton);
	RegisterClass_(LStdPopupMenu);
	RegisterClass_(LTextEdit);
	RegisterClass_(LView);
	RegisterClass_(LWindow);
	RegisterClass_(LRadioGroup);
	RegisterClass_(LTabGroup);
	RegisterClass_(LActiveScroller);


#ifdef PP_NewClasses
	#include <LCicnButton.h>
	#include <LOffscreenView.h>
	#include <LTextButton.h>

	RegisterClass_(LCicnButton);
	RegisterClass_(LOffscreenView);
	RegisterClass_(LTextButton);
#endif
	RegisterClass_(LTable);
	RegisterClass_(LIconPane);
	RegisterClass_(LGroupBox);
	RegisterClass_(LTextColumn);
	
	RegisterClass_(CGAPopupMenu);
	
	// ₯₯₯ PowerPlant Grayscale Classes
	RegisterGALibraryClasses();

	// ₯₯₯ PowerPlant appearance manager classes and their implementations
	RegisterClass_(LStaticText);
	RegisterClassID_(LAMStaticTextImp, LStaticText::imp_class_ID);
	RegisterClass_(LIconControl);
	RegisterClassID_(LAMControlImp, LIconControl::imp_class_ID);
	RegisterClass_(LProgressBar);
	RegisterClassID_(LAMControlImp, LProgressBar::imp_class_ID);
	RegisterClass_(LPushButton);
	RegisterClassID_(LAMPushButtonImp, LPushButton::imp_class_ID);
//	RegisterClass_(LBevelButton);
//	RegisterClassID_(LAMControlImp, LBevelButton::imp_class_ID);
	RegisterClass_(LCmdBevelButton);
	RegisterClassID_(LAMControlImp, LCmdBevelButton::imp_class_ID);
	RegisterClass_(LCheckBox);
	RegisterClassID_(LAMControlImp, LCheckBox::imp_class_ID);
	RegisterClass_(LRadioButton);
	RegisterClassID_(LAMControlImp, LRadioButton::imp_class_ID);
	RegisterClass_(LTextGroupBox);
	RegisterClassID_(LAMControlImp, LTextGroupBox::imp_class_ID);
	RegisterClass_(LPopupButton);
	RegisterClassID_(LAMPopupButtonImp, LPopupButton::imp_class_ID);
//	RegisterClass_(LEditText);
//	RegisterClassID_(LAMEditTextImp, LEditText::imp_class_ID);
	
	// ₯₯₯ Things that will go away after appearance port complete
	RegisterClass_(CProgressBar);
	RegisterClass_(CPatternBevelView);
	RegisterClass_(CBevelButton);
	RegisterClass_(CDeluxeBevelButton);
	RegisterClass_(CPatternButton);
	RegisterClass_(CPatternButtonPopup);
	RegisterClass_(CPatternProgressBar);
	RegisterClass_(CPatternProgressCaption);
	RegisterClass_(CPatternPane);
	RegisterClass_(CGABorderPane);

	// ₯₯₯ General Purpose UI Classes
	RegisterClass_(CToolbarBevelButton);	
	RegisterClass_(CAMSavvyBevelView);	
		
	RegisterClass_(CIncludeView);
	RegisterClass_(CPlaceHolderView);
	RegisterClass_(COffscreenCaption);
	RegisterClass_(CClusterView);
	RegisterClass_(CPatternTabControl);
	RegisterClass_(CTabSwitcher);
	RegisterClass_(CKeyScrollAttachment);
	RegisterClass_(CToolTipAttachment);
	RegisterClass_(CDynamicTooltipPane);
	RegisterClass_(CSharedToolTipAttachment);
	RegisterClass_(CMenuTable);
	RegisterClass_(CPaneEnabler);
	RegisterClass_(CSlaveEnabler);
	RegisterClass_(CScrollerWithArrows);
	
#ifdef MOZ_MAIL_NEWS
	RegisterClass_(CSingleLineEditField);
	RegisterClass_(CSelectFolderMenu);
//#else
//	RegisterClass_(CBiffButtonAttachment);
#endif // MOZ_MAIL_NEWS
	RegisterClass_(CSimpleDividedView);
	RegisterClass_(CProgressCaption);
	RegisterClass_(CTaskBarView);

	RegisterClass_(CToolTipPane);
	RegisterClass_(CStringListToolTipPane);

	RegisterClass_(LTableHeader);
	RegisterClass_(LTableViewHeader);


	RegisterClass_(CTextEdit);
	RegisterClass_(CEditBroadcaster);

	RegisterClass_(CNavigationButtonPopup);
	
	// *** Browser Specific UI Classes

	RegisterClass_(CDragBar);
	RegisterClass_(CDragBarContainer);
	RegisterClass_(CDragBarDockControl);
	RegisterClass_(CBrokeredView);
	RegisterClass_(CSwatchBrokerView);
	RegisterClass_(CToolbarDragBar);
	RegisterClass_(CToolbarPatternBevelView);
	RegisterClass_(CPersonalToolbarTable);
	
	RegisterClass_(CConfigActiveScroller);
	RegisterClass_(CTSMEditField);
//	REGISTERV(EditField);
	
	RegisterClass_(CDownloadProgressWindow);
	RegisterClass_(CBrowserWindow);
	RegisterClass_(CHTMLView);
	RegisterClass_(CURLEditField);
	RegisterClass_(CAutoCompleteURLEditField);
	RegisterClass_(CURLCaption);
	RegisterClass_(CSaveProgress);
	RegisterClass_(CHyperScroller);
	RegisterClass_(CButtonEnablerReloadStop);
	RegisterClass_(CBrowserView);

	RegisterClass_(CPatternedGrippyPane);
	RegisterClass_(CDividerGrippyPane);
	RegisterClass_(CSpinningN);
	RegisterClass_(CBrowserSecurityButton);
	RegisterClass_(CMiniSecurityButton);
	
	RegisterClass_(CHyperTreeFlexTable);
	RegisterClass_(CNavCenterSelectorPane);
	RegisterClass_(CRDFCoordinator);
	RegisterClass_(CHyperTreeHeader);
	RegisterClass_(CInlineEditField);

	RegisterClass_(CNavCenterWindow);
	RegisterClass_(CBookmarksFindDialog);
	RegisterClass_(CPluginView);
	RegisterClass_(LDividedView);
	RegisterClass_(CNavCenterContextMenuAttachment);
	RegisterClass_(CNavCenterSelectorContextMenuAttachment);
	RegisterClass_(CNavCenterTitle);
	
#ifdef EDITOR
	CEditorWindow::RegisterViewTypes();
#endif // EDITOR
	CFindWindow::RegisterViewTypes();
	UFormElementFactory::RegisterFormTypes();
	UHTMLPrinting::RegisterHTMLPrintClasses();
	CPrefsMediator::RegisterViewClasses();
	CPrefsDialog::RegisterViewClasses();
	UAssortedPrefMediators::RegisterViewClasses();

	RegisterClass_(CContextMenuAttachment);
#ifdef JAVA
	RegisterClass_(CJavaView);
#endif

} // RegisterAllBrowserClasses
