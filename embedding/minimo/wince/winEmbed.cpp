/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Minimo.
 *
 * The Initial Developer of the Original Code is
 * Doug Turner <dougt@meer.net>.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "MinimoPrivate.h"

#ifdef _BUILD_STATIC_BIN
nsStaticModuleInfo const *const kPStaticModules = nsnull;
PRUint32 const kStaticModuleCount = 0;
#endif

// Global variables
const static char* start_url = "chrome://minimo/content/minimo.xul";
//const static char* start_url = "http://www.meer.net/~dougt/test.html";
//const static char* start_url = "resource://gre/res/start.html";

static NS_DEFINE_CID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

class ApplicationObserver: public nsIObserver 
{
public:  
  ApplicationObserver(nsIAppShell* aAppShell);
  ~ApplicationObserver();  
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsCOMPtr<nsIAppShell> mAppShell;
  int mWindowCount;
};


ApplicationObserver::ApplicationObserver(nsIAppShell* aAppShell)
{
    mAppShell = aAppShell;
    mWindowCount = 0;
    
    nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");


    os->AddObserver(this, "nsIEventQueueActivated", PR_FALSE);
    os->AddObserver(this, "nsIEventQueueDestroyed", PR_FALSE);

    os->AddObserver(this, "xul-window-registered", PR_FALSE);
    os->AddObserver(this, "xul-window-destroyed", PR_FALSE);
    os->AddObserver(this, "xul-window-visible", PR_FALSE);
}

ApplicationObserver::~ApplicationObserver()
{
}

NS_IMPL_ISUPPORTS1(ApplicationObserver, nsIObserver)

NS_IMETHODIMP
ApplicationObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData)
{
    if (!strcmp(aTopic, "nsIEventQueueActivated")) 
    {
        nsCOMPtr<nsIEventQueue> eq(do_QueryInterface(aSubject));
        if (eq)
        {
            PRBool isNative = PR_TRUE;
            // we only add native event queues to the appshell
            eq->IsQueueNative(&isNative);
            if (isNative)
                mAppShell->ListenToEventQueue(eq, PR_TRUE);
        }
    } 
    else if (!strcmp(aTopic, "nsIEventQueueDestroyed")) 
    {
        nsCOMPtr<nsIEventQueue> eq(do_QueryInterface(aSubject));
        if (eq) 
        {
            PRBool isNative = PR_TRUE;
            // we only remove native event queues from the appshell
            eq->IsQueueNative(&isNative);
            if (isNative)
                mAppShell->ListenToEventQueue(eq, PR_FALSE);
        }
    } 
    else if (!strcmp(aTopic, "xul-window-visible"))
    {
        KillSplashScreen();
    }
 
    else if (!strcmp(aTopic, "xul-window-registered"))
    {
        mWindowCount++;
    }
    else if (!strcmp(aTopic, "xul-window-destroyed"))
    {
        mWindowCount--;
        if (mWindowCount == 0)
            mAppShell->Exit();
    }

    return NS_OK;
}

nsresult StartupProfile()
{    
    NS_TIMELINE_MARK_FUNCTION("Profile Startup");

	nsCOMPtr<nsIFile> appDataDir;
	nsresult rv = NS_GetSpecialDirectory(NS_APP_APPLICATION_REGISTRY_DIR, getter_AddRefs(appDataDir));
	if (NS_FAILED(rv))
        return rv;
    
	rv = appDataDir->Append(NS_LITERAL_STRING("minimo"));
	if (NS_FAILED(rv))
        return rv;

	nsCOMPtr<nsILocalFile> localAppDataDir(do_QueryInterface(appDataDir));
    
	nsCOMPtr<nsProfileDirServiceProvider> locProvider;
    NS_NewProfileDirServiceProvider(PR_TRUE, getter_AddRefs(locProvider));
    if (!locProvider)
        return NS_ERROR_FAILURE;
    
	rv = locProvider->Register();
    if (NS_FAILED(rv))
        return rv;
    
	return locProvider->SetProfileDir(localAppDataDir);   
}

void SetPreferences()
{
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefBranch)
        return;

    prefBranch->SetBoolPref("snav.keyCode.modifier", 0);
}


typedef struct FindAppStruct
{
  HWND hwnd;
} FindAppStruct;

BOOL CALLBACK FindApplicationWindowProc(HWND hwnd, LPARAM lParam)
{
  FindAppStruct* findApp = (FindAppStruct*) lParam;
  
  unsigned short windowName[MAX_PATH];
  GetWindowTextW(hwnd, windowName, MAX_PATH);
  
  if (wcsstr(windowName, L"Minimo"))
  {
    findApp->hwnd = hwnd;
    return FALSE;
  }
  return TRUE;
} 

PRBool DoesProcessAlreadyExist()
{
    const HANDLE hMutex = CreateMutexW(0, 0, L"_MINIMO_EXE_MUTEX_");
    
	if(hMutex)
    {
      if(ERROR_ALREADY_EXISTS == GetLastError()) 
      {
        FindAppStruct findApp;
        findApp.hwnd = NULL;
        
        EnumWindows(FindApplicationWindowProc, (LPARAM)&findApp);
        
        if (findApp.hwnd)
        {
          SetForegroundWindow(findApp.hwnd);
          return TRUE;
        }

        MessageBox(0, "Minimo is running, but can't be switched to.", "Unexpected Error", 0);
        return TRUE;
      }
      return FALSE;
    }
    MessageBox(0, "Can not start Minimo", "Unexpected Error", 0);
    return TRUE;
}

int main(int argc, char *argv[])
{
    if (DoesProcessAlreadyExist())
        return 0;

    CreateSplashScreen();

    NS_InitEmbedding(nsnull, nsnull, kPStaticModules, kStaticModuleCount);

    // Choose the new profile
    if (NS_FAILED(StartupProfile()))
        return 1;
    
    SetPreferences();

    NS_TIMELINE_ENTER("appStartup");
    nsCOMPtr<nsIAppShell> appShell = do_CreateInstance(kAppShellCID);
    appShell->Create(nsnull, nsnull);
    
    ApplicationObserver *appObserver = new ApplicationObserver(appShell);
    if (!appObserver)
        return 1;
    NS_ADDREF(appObserver);
    
    WindowCreator *creatorCallback = new WindowCreator(appShell);
    if (!creatorCallback)
        return 1;

    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
    wwatch->SetWindowCreator(creatorCallback);

    nsCOMPtr<nsIDOMWindow> newWindow;
    wwatch->OpenWindow(nsnull, start_url, "_blank", "chrome,dialog=no,all", nsnull, getter_AddRefs(newWindow));

    appShell->Run();

    appShell = nsnull;
    wwatch = nsnull;
    newWindow = nsnull;

    delete appObserver;
    delete creatorCallback;

    // Close down Embedding APIs
    NS_TermEmbedding();

    return NS_OK;
}
