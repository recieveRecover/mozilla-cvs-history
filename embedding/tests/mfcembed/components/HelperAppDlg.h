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
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chak Nanga <chak@netscape.com> 
 */

#ifndef __HelperAppDlg_h
#define __HelperAppDlg_h

#include "nsError.h"
#include "resource.h"
#include "nsIFactory.h"
#include "nsIExternalHelperAppService.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIWebProgressListener.h"
#include "nsWeakReference.h"
#include "nsIWindowWatcher.h"

// this component is for an MFC app; it's Windows. make sure this is defined.
#ifndef XP_WIN
#define XP_WIN
#endif

class CHelperAppLauncherDialogFactory : public nsIFactory {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIFACTORY

    CHelperAppLauncherDialogFactory();
    virtual ~CHelperAppLauncherDialogFactory();
};

enum {
    CONTENT_SAVE_TO_DISK,
    CONTENT_LAUNCH_WITH_APP
};

// This class implements the nsIHelperAppLauncherDialog interface
//
class CHelperAppLauncherDialog : public nsIHelperAppLauncherDialog {
public:
    CHelperAppLauncherDialog();
    virtual ~CHelperAppLauncherDialog();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIHELPERAPPLAUNCHERDIALOG

    int m_HandleContentOp;
    CString m_FileName;

private:
  nsCOMPtr<nsIWindowWatcher> mWWatch;
  CWnd* GetParentFromContext(nsISupports *aWindowContext);
};

// This class implements a download progress dialog
// which displays the progress of a file download
//
class CProgressDlg : public CDialog, 
                     public nsIWebProgressListener,
                     public nsSupportsWeakReference
{
public:
    enum { IDD = IDD_PROGRESS_DIALOG };

    CProgressDlg(nsIHelperAppLauncher *aLauncher, int aHandleContentOp,
                CString& aFileName, CWnd* pParent = NULL);
    virtual ~CProgressDlg();

    int m_HandleContentOp;
    CString m_FileName;
    CStatic	m_SavingFrom;
    CStatic	m_Action;
    CProgressCtrl m_ProgressCtrl;

    nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();

	DECLARE_MESSAGE_MAP()
};

// This class implements a dialog box which gives the user
// a choice of saving the content being downloaded to disk
// or to open with an external application
//
class CChooseActionDlg : public CDialog
{
public:
    CChooseActionDlg(nsIHelperAppLauncher* aLauncher, CWnd* pParent = NULL);

    enum { IDD = IDD_CHOOSE_ACTION_DIALOG };
    CStatic	m_ContentType;

    int m_ActionChosen;
    CString m_OpenWithAppName;
    nsCOMPtr<nsIHelperAppLauncher> m_HelperAppLauncher;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
    virtual BOOL OnInitDialog();
    afx_msg void OnChooseAppClicked();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnOpenUsingRadioBtnClicked();
    afx_msg void OnSaveToDiskRadioBtnClicked();

    void EnableAppName(BOOL bEnable);
    void EnableChooseBtn(BOOL bEnable);
    void InitWithPreferredAction(nsIMIMEInfo* aMimeInfo);
    void SelectSaveToDiskOption();
    void SelectOpenUsingAppOption();
    void UpdateAppNameField(CString& appName);

    DECLARE_MESSAGE_MAP()
};

#endif
