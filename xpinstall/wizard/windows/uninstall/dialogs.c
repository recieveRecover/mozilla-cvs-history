/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code,
 * released March 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *     Sean Su <ssu@netscape.com>
 */

#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"
#include "parser.h"

LRESULT CALLBACK DlgProcUninstall(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  char  szBuf[MAX_BUF];
  char  szFileInstallLog[MAX_BUF];
  char  szKey[MAX_BUF];
  sil   *silFile;
  RECT  rDlg;
  DWORD dwFileFound;

  switch(msg)
  {
    case WM_INITDIALOG:
      SetWindowText(hDlg, diUninstall.szTitle);
      wsprintf(szBuf, diUninstall.szMessage0, ugUninstall.szDescription);
      SetDlgItemText(hDlg, IDC_MESSAGE0, szBuf);

      if(GetClientRect(hDlg, &rDlg))
        SetWindowPos(hDlg, HWND_TOP, (dwScreenX/2)-(rDlg.right/2), (dwScreenY/2)-(rDlg.bottom/2), 0, 0, SWP_NOSIZE);

      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDWIZNEXT:
          dwFileFound = GetLogFile(ugUninstall.szLogPath, ugUninstall.szLogFilename, szFileInstallLog, sizeof(szFileInstallLog));
          while(dwFileFound)
          {
            if((silFile = InitSilNodes(szFileInstallLog)) != NULL)
            {
              FileDelete(szFileInstallLog);
              Uninstall(silFile);
              DeInitSilNodes(&silFile);
            }

            dwFileFound = GetLogFile(ugUninstall.szLogPath, ugUninstall.szLogFilename, szFileInstallLog, sizeof(szFileInstallLog));
          }

          lstrcpy(szFileInstallLog, ugUninstall.szLogPath);
          AppendBackSlash(szFileInstallLog, MAX_BUF);
          lstrcat(szFileInstallLog, ugUninstall.szLogFilename);
          if(FileExists(szFileInstallLog))
          {
            if((silFile = InitSilNodes(szFileInstallLog)) != NULL)
            {
              FileDelete(szFileInstallLog);
              Uninstall(silFile);
              DeInitSilNodes(&silFile);
            }
          }

          /* clean up the uninstall windows registry key */
          lstrcpy(szKey, "Software\\Microsoft\\Windows\\CurrentVersion\\uninstall\\");
          lstrcat(szKey, ugUninstall.szUninstallKeyDescription);
          RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);

          DestroyWindow(hDlg);
          PostQuitMessage(0);
          break;

        case IDCANCEL:
          DestroyWindow(hDlg);
          PostQuitMessage(0);
          break;

        default:
          break;
      }
      break;
  }
  return(0);
}

LRESULT CALLBACK DlgProcMessage(HWND hDlg, UINT msg, WPARAM wParam, LONG lParam)
{
  RECT rDlg;
  HWND hSTMessage = GetDlgItem(hDlg, IDC_MESSAGE); /* handle to the Static Text message window */
  HDC  hdcSTMessage;
  SIZE sizeString;
  int  iLen;
//  int  iCount;
//  int  iCharWidth;
//  UINT uiTotalWidth;

  switch(msg)
  {
    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_MESSAGE:
          hdcSTMessage = GetWindowDC(hSTMessage);
          iLen = lstrlen((LPSTR)lParam);
          GetTextExtentPoint32(hdcSTMessage, (LPSTR)lParam, iLen, &sizeString);

/*          uiTotalWidth = 0;
          for(iCount = 0; iCount < iLen; iCount ++)
          {
            GetCharWidth32(hdcSTMessage, ((LPSTR)lParam)[iCount], ((LPSTR)lParam)[iCount], &iCharWidth);
            uiTotalWidth += iCharWidth;
          }
*/

          ReleaseDC(hSTMessage, hdcSTMessage);

          SetWindowPos(hDlg, HWND_TOP,
                      (dwScreenX/2)-((sizeString.cx - (iLen * 1))/2), (dwScreenY/2)-((sizeString.cy + 50)/2),
                      (sizeString.cx - (iLen * 1)), sizeString.cy + 50,
                       SWP_SHOWWINDOW);

          if(GetClientRect(hDlg, &rDlg))
            SetWindowPos(hSTMessage, HWND_TOP,
                         rDlg.left, rDlg.top,
                         (sizeString.cx - (iLen * 1)), rDlg.bottom,
                         SWP_SHOWWINDOW);

          SetDlgItemText(hDlg, IDC_MESSAGE, (LPSTR)lParam);
          break;
      }
      break;
  }
  return(0);
}

void ProcessWindowsMessages()
{
  MSG msg;

  while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}
void ShowMessage(LPSTR szMessage, BOOL bShow)
{
  char szBuf[MAX_BUF];

  if(ugUninstall.dwMode != SILENT)
  {
    if((bShow) && (hDlgMessage == NULL))
    {
      ZeroMemory(szBuf, sizeof(szBuf));
      NS_LoadString(hInst, IDS_MB_MESSAGE_STR, szBuf, MAX_BUF);
      InstantiateDialog(DLG_MESSAGE, szBuf, DlgProcMessage);
      SendMessage(hDlgMessage, WM_COMMAND, IDC_MESSAGE, (LPARAM)szMessage);
    }
    else if(!bShow && hDlgMessage)
    {
      DestroyWindow(hDlgMessage);
      hDlgMessage = NULL;
    }
  }
}

void InstantiateDialog(DWORD dwDlgID, LPSTR szTitle, WNDPROC wpDlgProc)
{
  char szBuf[MAX_BUF];

  if((hDlg = CreateDialog(hInst, MAKEINTRESOURCE(dwDlgID), hWndMain, wpDlgProc)) == NULL)
  {
    char szEDialogCreate[MAX_BUF];

    if(NS_LoadString(hInst, IDS_ERROR_DIALOG_CREATE, szEDialogCreate, MAX_BUF) == WIZ_OK)
    {
      wsprintf(szBuf, szEDialogCreate, szTitle);
      PrintError(szBuf, ERROR_CODE_SHOW);
    }

    PostQuitMessage(1);
  }
  else if(dwDlgID == DLG_MESSAGE)
  {
    hDlgMessage = hDlg;
  }
}

