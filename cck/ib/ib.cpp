#include "stdafx.h"
#include <Winbase.h>
#include <direct.h>
#include "globals.h"
#include "comp.h"
#include "ib.h"
#include <afxtempl.h>
#include <afxdisp.h>
#include "resource.h"
#include "NewDialog.h"
#define MAX_SIZE 1024

int interpret(char *cmd);

CString rootPath;
CString configName;
CString configPath;
CString workspacePath;
CString cdPath;
CString networkPath;
CString tempPath;
CString iniDstPath;
CString iniSrcPath;
CString scriptPath;
CString nscpxpiPath;
CString cdshellPath;
CString outputPath; 

WIDGET *tempWidget;
int selCount;
char buffer[50000];
XPI	xpiList[100];
int xpiLen = -1;

COMPONENT Components[100];
int		numComponents;

int findXPI(CString xpiname, CString filename)
{
	int found = FALSE;

	for (int i=0; !found && i<=xpiLen; i++)
		if (xpiList[i].xpiname == xpiname && 
			xpiList[i].filename == filename)
				found = TRUE;

	if (!found)
	{
		xpiLen++;
		xpiList[xpiLen].xpiname  = xpiname;
		xpiList[xpiLen].filename = filename;
	}

	return found;
}

int ExtractXPIFile(CString xpiname, CString xpifile)
{
	CString command;

	if (findXPI(xpiname, xpifile))
		return TRUE;

	// Can use -d instead of change CWD???
	CString xpiArchive = nscpxpiPath + "\\" + xpiname;
	command = rootPath + "unzip.exe -o" + " " + xpiArchive + " " + xpifile;
	ExecuteCommand((char *)(LPCTSTR) command, SW_HIDE, INFINITE);

	return TRUE;
}

int ReplaceXPIFiles()
{
	CString command;
	CString xpiArchive;
	CString xpiArcDest;

	// Go through the whole list putting them into the archives
	for (int i=0; i<=xpiLen; i++)
	{
		// This copy preserves the existing archive if it exists - do we
		// need to delete it the first time through?
		xpiArchive = nscpxpiPath + "\\" + xpiList[i].xpiname;
		xpiArcDest = cdPath + "\\" + xpiList[i].xpiname;
		if (!CopyFile(xpiArchive, xpiArcDest, TRUE))
			DWORD e = GetLastError();

		command = rootPath + "zip.exe -m " + xpiArcDest + " " + xpiList[i].filename;
		ExecuteCommand((char *)(LPCTSTR) command, SW_HIDE, INFINITE);
	}

	return TRUE;
}

int ReplaceINIFile()
{
	CString command;
	CString exeName("Seamonkey32e.exe");
	char	olddir[1024];

	GetCurrentDirectory(sizeof(olddir), olddir);

	if(SetCurrentDirectory((char *)(LPCTSTR) cdPath) == FALSE)
		return FALSE;

	CString Src = nscpxpiPath + "\\" +exeName;
	CString Dst = cdPath + "\\" + exeName;
	if (!CopyFile(Src, Dst, FALSE))
		DWORD e = GetLastError();

	command = rootPath + "nszip.exe " + exeName +  " config.ini";
	ExecuteCommand((char *)(LPCTSTR) command, SW_SHOW, INFINITE);

	SetCurrentDirectory(olddir);

	return TRUE;
}

void ModifyPref(char *buffer, CString entity, CString newvalue)
{
	CString buf(buffer);
	
	int i = buf.Find(entity);
	if (i == -1) return;

	i = buf.Find('"', i+1);
	if (i == -1) return;
	i++;

	i = buf.Find('"', i+1);
	if (i == -1) return;
	i++;

	int j = buf.Find('"', i);
	if (j == -1) return;
	j--;

	buf.Delete(i, j-i+1);
	buf.Insert(i, newvalue);

	strcpy(buffer, (char *)(LPCTSTR) buf);
}

int ModifyJS(CString xpifile, CString entity, CString newvalue)
{
	CString newfile = xpifile + ".new";
	int rv = TRUE;
	char *fgetsrv;

	// Read in DTD file and make substitutions
	FILE *srcf = fopen(xpifile, "r");
	FILE *dstf = fopen(newfile, "w");
	CString apost = '"';
	entity.Insert(0,apost);
	entity.Insert(1000,apost);

	if (!srcf)
		rv = FALSE;
	else
	{
		int done = FALSE;
		while (!done)
		{
			fgetsrv = fgets(buffer, sizeof(buffer), srcf);
			done = feof(srcf);
			if (!done)
			{
				if (!fgetsrv || ferror(srcf))
				{
					rv = FALSE;
					break;
				}
				ModifyPref(buffer, entity, newvalue);
				fputs(buffer, dstf);
			}
		}

		fclose(srcf);
		fclose(dstf);
	}

	remove(xpifile);
	rename(newfile, xpifile);

	return TRUE;
}
void ModifyEntity(char *buffer, CString entity, CString newvalue)
{
	CString buf(buffer);
	entity = entity + " ";  // Ensure we don't get false matches

	int i = buf.Find(entity);
	if (i == -1) return;

	i = buf.Find('"', i+1);
	if (i == -1) return;
	i++;

	int j = buf.Find('"', i);
	if (j == -1) return;
	j--;

	buf.Delete(i, j-i+1);
	buf.Insert(i, newvalue);

	strcpy(buffer, (char *)(LPCTSTR) buf);
}

int ModifyDTD(CString xpifile, CString entity, CString newvalue)
{
	CString newfile = xpifile + ".new";
	int rv = TRUE;
	char *fgetsrv;

	// Read in DTD file and make substitutions
	FILE *srcf = fopen(xpifile, "r");
	FILE *dstf = fopen(newfile, "w");
	if (!srcf)
		rv = FALSE;
	else
	{
		int done = FALSE;
		while (!done)
		{
			fgetsrv = fgets(buffer, sizeof(buffer), srcf);
			done = feof(srcf);
			if (!done)
			{
				if (!fgetsrv || ferror(srcf))
				{
					rv = FALSE;
					break;
				}
				ModifyEntity(buffer, entity, newvalue);
				fputs(buffer, dstf);
			}
		}

		fclose(srcf);
		fclose(dstf);
	}

	remove(xpifile);
	rename(newfile, xpifile);

	return TRUE;
}

int interpret(char *cmd)
{
	char *cmdname = strtok(cmd, "(");

	if (strcmp(cmdname, "configure") == 0)
	{
		char temp[1024];
		char *section	= strtok(NULL, ",)");
		char *key 		= strtok(NULL, ",)");
		char *value 	= strtok(NULL, ",)");
		char *newvalue	= value;
		if (value[0] == '%')
		{
			value++;
			char *t = strchr(value, '%');
			if (!t)
				return FALSE;
			*t = '\0';
			newvalue = (char *)(LPCTSTR) GetGlobal(value);
		}
		if (!section || !key || !newvalue)
			return FALSE;
		if (!CopyFile(iniSrcPath, iniDstPath, TRUE))
			DWORD e = GetLastError();
		if (strcmp(key, "Program Folder Name") == 0)
		{
			strcpy(temp, "Netscape Communicator by ");
			strcat(temp, newvalue);
			newvalue = temp;
		}
		WritePrivateProfileString(section, key, newvalue, iniDstPath);
	}
	else if (strcmp(cmdname, "replaceXPI") == 0)
	{
		char *xpiname	= strtok(NULL, ",)");
		char *xpifile	= strtok(NULL, ",)");
		char *value 	= strtok(NULL, ",)");
		char *newvalue	= value;
		if (value[0] == '%')
		{
			value++;
			char *t = strchr(value, '%');
			if (!t)
				return FALSE;
			*t = '\0';
			newvalue = (char *)(LPCTSTR) GetGlobal(value);
		}
		if (!xpiname || !xpifile || !newvalue)
			return FALSE;
		/*This is a hack to prevent the last page from staying up there endlessly;
		  We check to see if the filename is null and if it is so we return true 
		  so that the return value isnt made FALSE */
		CString filename = newvalue;
		if (filename.IsEmpty())
			return TRUE;
		////////////////////////////////
		ExtractXPIFile(xpiname, xpifile);
		if (!CopyFile(newvalue, xpifile, FALSE))
		{
			DWORD e = GetLastError();
			return FALSE;
		}
	}
	else if ((strcmp(cmdname, "modifyDTD") == 0) ||
			(strcmp(cmdname, "modifyJS") == 0))
	{
		char *xpiname	= strtok(NULL, ",)");
		char *xpifile	= strtok(NULL, ",)");
		char *entity	= strtok(NULL, ",)");
		char *value 	= strtok(NULL, ",)");
		char *newvalue	= value;
		if (value[0] == '%')
		{
			value++;
			char *t = strchr(value, '%');
			if (!t)
				return FALSE;
			*t = '\0';
			newvalue = (char *)(LPCTSTR) GetGlobal(value);
		}
		if (!xpiname || !xpifile || !entity || !newvalue)
			return FALSE;
		ExtractXPIFile(xpiname, xpifile);
		if(strcmp(cmdname, "modifyJS") == 0)
			ModifyJS(xpifile,entity,newvalue);
		else
			ModifyDTD(xpifile, entity, newvalue);
	}
	else if (strcmp(cmdname, "wrapXPI") == 0)
	{
	}
	else
		return FALSE;

	return TRUE;
}

void init_components()
{
	int i;
	WIDGET *w = findWidget("SelectedComponents");
	BuildComponentList(Components, &numComponents, iniSrcPath);

	// Turn off components that aren't selected
	for (i=0; i<numComponents; i++)
		if (strstr(w->value, Components[i].name) == NULL)
			Components[i].selected = FALSE;

}

void invisible()
{
	tempWidget = findWidget("SelectedComponents");
	CString component;
	{
		selCount = (((CCheckListBox *)tempWidget->control))->GetCount();
			
		for (int i=0; i < selCount; i++)
		{
			if (((CCheckListBox *)tempWidget->control)->GetCheck(i) != 1)
			{
			component = Components[i].compname;	
			WritePrivateProfileString(Components[i].compname, "Attributes", "INVISIBLE", iniDstPath);
				
			}
		}
	}
}
void AddThirdParty()
{
	CString tpCompPath1 = GetGlobal("CustomComponent1");
	CString tpCompPath2 = GetGlobal("CustomComponent137");
	CString tpComp1		= GetGlobal("CustomComponent2");
	CString tpComp2		= GetGlobal("CustomComponent23");
	CString tpCompSize1	= GetGlobal("ComponentSize");
	CString tpCompSize2	= GetGlobal("ModuleSize");
	CString componentName;
	CString cName;
	CString compSDesc	= "Description Short=";
	CString compLDesc	= "Description Long=";
	CString compArchive = "Archive=";
	CString compISize	= "Install Size=";
	CString compAttrib	= "Attributes=SELECTED|LAUNCHAPP";
	int archiveLen		= tpCompPath1.GetLength();
	int findLen			= tpCompPath1.ReverseFind('\\');
	CString Archive1	= tpCompPath1.Right(archiveLen - findLen -1);
	archiveLen			= tpCompPath2.GetLength();
	findLen				= tpCompPath2.ReverseFind('\\');
	CString Archive2	= tpCompPath2.Right(archiveLen - findLen -1);
	CString tempstr;
	char *cBuffer1 = new char [MAX_SIZE];
	char *p = cBuffer1;
	strcpy(p,LPCTSTR(compSDesc + tpComp1));
	tempstr = compSDesc + tpComp1;
	p += (tempstr.GetLength() +1);
	strcpy(p,LPCTSTR(compLDesc + tpComp1));
	tempstr = compLDesc + tpComp1;
	p += (tempstr.GetLength() +1);
	strcpy(p,LPCTSTR(compArchive + Archive1));
	tempstr = compArchive + Archive1;
	p += (tempstr.GetLength() +1);
	strcpy(p,LPCTSTR(compISize + tpCompSize1));
	tempstr = compISize + tpCompSize1;
	p += (tempstr.GetLength() +1);
	strcpy(p,LPCTSTR(compAttrib));
	p += (compAttrib.GetLength() +1);
	*p = 0;
	char *cBuffer2 = new char [MAX_SIZE];
	char *q = cBuffer2;
	strcpy(q,LPCTSTR(compSDesc + tpComp2));
	tempstr = compSDesc + tpComp2;
	q += (tempstr.GetLength() +1);
	strcpy(q,LPCTSTR(compLDesc + tpComp2));
	tempstr = compLDesc + tpComp2;
	q += (tempstr.GetLength() +1);
	strcpy(q,LPCTSTR(compArchive + Archive2));
	tempstr = compArchive + Archive2;
	q += (tempstr.GetLength() +1);
	strcpy(q,LPCTSTR(compISize + tpCompSize2));
	tempstr = compISize + tpCompSize2;
	q += (tempstr.GetLength() +1);
	strcpy(q,LPCTSTR(compAttrib));
	q += (compAttrib.GetLength() +1);
	*q = 0;


/*
	char cBuffer1[MAX_SIZE][MAX_SIZE]={LPCTSTR(compSDesc + tpComp1),LPCTSTR(compLDesc + tpComp1),
					 LPCTSTR(compArchive + Archive1),LPCTSTR(compISize + tpCompSize1),
					 LPCTSTR(compAttrib)};
	char *cBuffer2[]={LPCTSTR(compSDesc + tpComp2),LPCTSTR(compLDesc + tpComp2),
					 LPCTSTR(compArchive + Archive2),LPCTSTR(compISize + tpCompSize2),
					 LPCTSTR(compAttrib)};
*/

	if (!tpCompPath1.IsEmpty())
	{
		componentName.Format("Component%d", (selCount));
		cName.Format("C%d", (selCount -1));

		WritePrivateProfileString("Setup Type0", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type1", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type2", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type3", cName, componentName, iniDstPath);
		WritePrivateProfileSection(componentName, cBuffer1, iniDstPath);
		selCount++;
		CopyFile(tpCompPath1, cdPath + "\\" + Archive1, FALSE);
		DWORD e1 = GetLastError();

	}
	if (!tpCompPath2.IsEmpty())
	{
		componentName.Format("Component%d", (selCount));
		cName.Format("C%d", (selCount -1));

		WritePrivateProfileString("Setup Type0", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type1", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type2", cName, componentName, iniDstPath);
		WritePrivateProfileString("Setup Type3", cName, componentName, iniDstPath);
		WritePrivateProfileSection(componentName, cBuffer2, iniDstPath);
		CopyFile(tpCompPath2, cdPath + "\\" + Archive2, FALSE);
		DWORD e2 = GetLastError();
	}
	delete [] cBuffer1;
	delete [] cBuffer2;
}
	HRESULT CreateShortcut(const CString Target, const CString Arguments, const CString
                     LinkFileName, const CString WorkingDir, bool IsFolder)
 {
         // Initialize OLE libraries
         if (!AfxOleInit())
                 return FALSE;
         
         HRESULT hres;
         
         CString Desktop=getenv("USERPROFILE");
         Desktop += "\\Desktop\\";
         CString Link = Desktop + LinkFileName;
         
         if (!IsFolder)
         {
                 IShellLink* psl;
                 hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
 (LPVOID*) &psl);
                 if (SUCCEEDED(hres))
                 {
                         IPersistFile* ppf;
                         
                         psl->SetPath(Target);
                         psl->SetArguments(Arguments);
                         psl->SetWorkingDirectory(WorkingDir);
                         
                         hres = psl->QueryInterface( IID_IPersistFile, (LPVOID *) &ppf);
                         
                         if (SUCCEEDED(hres))
                         {
                                 CString Temp = Link;
                                 Temp.MakeLower();
                                 if (Temp.Find(".lnk")==-1)
                                         Link += ".lnk";  // Important !!!
                                 WORD wsz[MAX_PATH];
                                 MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, Link, -1, wsz,MAX_PATH);
                                 
                                 hres = ppf->Save(wsz, TRUE);
                                 
                                 ppf->Release();
                         }
                         psl->Release();
                 }
         }
         else
         {
                 hres = _mkdir(Link);
         }
         return hres;
 }
BOOL GetRegistryKey( HKEY key, char *subkey, char *retdata )
{ 
	long retval;
	HKEY hkey;

	retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

	if(retval == ERROR_SUCCESS)
	{
		long datasize = MAX_SIZE;
		char data[MAX_SIZE];

		RegQueryValue(hkey,NULL,(LPSTR)data,&datasize);

		lstrcpy(retdata,data);
		RegCloseKey(hkey);
	}
	return retval;
}

CString GetBrowser(void)
{
	char key[MAX_SIZE + MAX_SIZE];
	CString retflag = "";

	/* get the .htm regkey and lookup the program */
	if(GetRegistryKey(HKEY_CLASSES_ROOT,".htm",key) == ERROR_SUCCESS)
	{
		lstrcat(key,"\\shell\\open\\command");
		if(GetRegistryKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS)
		{
			char *pos;
			pos = strstr(key,"\"%1\"");
			if(pos == NULL)     /* if no quotes */
			{
				/* now check for %1, without the quotes */
				pos = strstr(key,"%1");
				if(pos == NULL) /* if no parameter */
				pos = key+lstrlen(key)-1;
				else
				*pos = '\0';    /* remove the parameter */
			}
			else
			*pos = '\0';        /* remove the parameter */
			retflag = key;
		}
		
	}

	return retflag;
}
/*BOOL Progress() 
{

	CProgressCtrl progressDlg(this);
	progressDlg.Create(IDD_NEW_DIALOG);
	CProgressCtrl *pProgressDlg = &progressDlg;
				
	//CRuntimeClass *pProgDlgThread = RUNTIME_CLASS(CProgDlgThread);	//This is the multi-threading stuff for the progress dialog
	//AfxBeginThread(pProgDlgThread);

	pProgressDlg->m_ProgressText.SetWindowText("Creating a CD Layout...");
	pProgressDlg->m_ProgressBar.SetPos(0);
	pProgressDlg->m_ProgressBar.SetRange(0,4);
	pProgressDlg->m_ProgressBar.SetStep(1);
				
		pProgressDlg->m_ProgressText.SetWindowText("Loading Globals...");
//		LoadGlobals();
		pProgressDlg->m_ProgressBar.StepIt();
		pProgressDlg->UpdateWindow();

		pProgressDlg->m_ProgressText.SetWindowText("Reading files...");
//		ReadIniFile();
		pProgressDlg->m_ProgressBar.StepIt();
		pProgressDlg->UpdateWindow();

		pProgressDlg->m_ProgressText.SetWindowText("Merging files...");
//		MergeFiles();
		pProgressDlg->m_ProgressBar.StepIt();
		pProgressDlg->UpdateWindow();

		pProgressDlg->m_ProgressText.SetWindowText("Creating CD Layout...");
//		CreateMedia();
		pProgressDlg->m_ProgressBar.StepIt();
		pProgressDlg->UpdateWindow();

		AfxMessageBox("CD Directory created", MB_OK);
	}

	return TRUE;
}
*/
extern "C" __declspec(dllexport)
int StartIB(CString parms, WIDGET *curWidget)
{
	char *fgetsrv;
	int rv = TRUE;
	char	olddir[1024];

	rootPath	= GetGlobal("Root");
	configName	= GetGlobal("CustomizationList");
	configPath  = rootPath + "Configs\\" + configName;
	outputPath	= configPath + "\\Output";
	cdPath 		= configPath + "\\Output\\Core";
	cdshellPath	= configPath + "\\Output\\Shell";
	networkPath = configPath + "\\Network";
	tempPath 	= configPath + "\\Temp";
	iniDstPath	= cdPath + "\\config.ini";
	scriptPath	= rootPath + "\\script.ib";
	workspacePath = configPath + "\\Workspace";
	if (SearchPath(workspacePath, "NSCPXPI", NULL, 0, NULL, NULL))
		nscpxpiPath = workspacePath + "\\NSCPXPI";
	else
		nscpxpiPath = rootPath + "NSCPXPI";
	iniSrcPath	= nscpxpiPath + "\\config.ini";

	init_components();
	
	CNewDialog newprog;
	newprog.Create(IDD_NEW_DIALOG,NULL );
	newprog.ShowWindow(SW_SHOW);
/////////////////////////////
	CWnd * dlg;
	CRect tmpRect = CRect(7,7,173,13);
	dlg = newprog.GetDlgItem(IDC_BASE_TEXT);
	CWnd* pwnd = newprog.GetDlgItem(IDD_NEW_DIALOG);
	dlg->SetWindowText("         Customization is in Progress");
	
/////////////////////////////
	_mkdir((char *)(LPCTSTR) cdPath);
	_mkdir((char *)(LPCTSTR) tempPath);
	_mkdir((char *)(LPCTSTR) workspacePath);
//	_mkdir((char *)(LPCTSTR) cdshellPath);
	GetCurrentDirectory(sizeof(olddir), olddir);

	if(SetCurrentDirectory((char *)(LPCTSTR) tempPath) == FALSE)
		return FALSE;

	// Read in script file and interpret commands from it
	FILE *f = fopen(scriptPath, "r");
	if (!f)
		rv = FALSE;
	else
	{
		int done = FALSE;
		while (!done)
		{
			fgetsrv = fgets(buffer, sizeof(buffer), f);
			done = feof(f);
			if (!done)
			{
				if (!fgetsrv || ferror(f))
				{
					rv = FALSE;
					break;
				}

				buffer[strlen(buffer)] = '\0';  // Eliminate the trailing newline

				if (!interpret(buffer))
				{
					rv = FALSE;
					break;
				}
			}
		}

		fclose(f);
	}

	dlg->SetWindowText("         Replacing XPI Files");

	// Put all the extracted files back into their new XPI homes
	ReplaceXPIFiles();

	dlg->SetWindowText("         Copying default files into the configuration");

	// Copy remaining default installer files into config
	// preserving any existing files that we created already
	// in previous steps
	/* -- Need to be more selective than this
	CopyDir(nscpxpiPath, cdPath, NULL, FALSE);
	*/
	CString cdDir= GetGlobal("CD image");
	CString networkDir = GetGlobal("Network");
	CString ftpLocation = GetGlobal("FTPLocation");

	for (int i=0; i<numComponents; i++)
	{
		if (Components[i].selected)
			CopyFile(nscpxpiPath + "\\" + Components[i].archive, 
					 cdPath + "\\" + Components[i].archive, TRUE);
	}

	if (cdDir.Compare("1") ==0)
	{
		
		CString shellPath = workspacePath + "\\Autorun\\";
		CopyDir(shellPath, outputPath, NULL, TRUE);
		CreateRshell ();

	}
	else
	{
		FILE *infout;
		CString infFile = outputPath + "\\autorun.inf";
		infout = fopen(infFile, "w");
		if (!infout)
			exit( 3 );

		fprintf(infout,"[autorun]\nopen = .\\Core\\Seamonkey32e.exe");
	}
	CString component;
	CString configiniPath = cdPath +"\\config.ini";

	if (ftpLocation.Compare("ftp://") !=0)
	{
		for (int i=0; i<numComponents; i++)
		{
			if (Components[i].selected)
				CopyFile(nscpxpiPath + "\\" + Components[i].archive, 
						 networkPath + "\\" + Components[i].archive, TRUE);
			
			WritePrivateProfileString(Components[i].compname, "url0", ftpLocation, configiniPath);
		}
		
	}
	
	// Didn't work...
	dlg->SetWindowText("         Checking for neccessary components to install");

	invisible();
	
	dlg->SetWindowText("         Checking for Third Party Software");

	AddThirdParty();

	dlg->SetWindowText("         Replacing config.ini");

	ReplaceINIFile();

	dlg->SetWindowText("         Creating Desktop Shortcut for Help");

	SetCurrentDirectory(olddir);
	CString TargetDir = GetGlobal("Root");
	CString TargetFile = TargetDir + "wizardmachine.ini";
	CString MozBrowser = GetBrowser();
	CreateShortcut(MozBrowser, TargetFile, "HelpLink", TargetDir, FALSE);

	dlg->SetWindowText("         Customization is Complete");
	newprog.DestroyWindow();
	return rv;

}

