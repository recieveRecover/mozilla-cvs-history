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

// All the code-resource handling stuff has been adopted from:
// Multi-Seg CR example in CW
// Classes:
// CPluginInstance - one per plugin instance. Needs to call the functions
// CPluginHandler - one per plugin MIME type

#include "CHTMLView.h"

#include "macutil.h"
#include "npapi.h"
#include "mplugin.h"
#include "uprefd.h"
#include "ufilemgr.h"
#include "uapp.h"	// might be unnecessary when we get rid of CFrontApp
#include "resgui.h"
#include "miconutils.h"
#include "uerrmgr.h"
#include "macgui.h"
#include "resae.h"
#include "edt.h"	// for EDT_RegisterPlugin()
#include "CApplicationEventAttachment.h"

#if defined(JAVA)
#include "java.h"	// for LJ_AddToClassPath
#endif

#include "np.h"
#include "nppg.h"
#include "prlink.h"

#include "npglue.h"
#include "nsIEventHandler.h"

#ifdef LAYERS
#include "layers.h"
#include "mkutils.h"
#endif // LAYERS

#include "MixedMode.h"
#include "MoreMixedMode.h"
#include <CodeFragments.h>

#include "LMenuSharing.h"

// On a powerPC, plugins depend on being able to find qd in our symbol table
#ifdef powerc
// #pragma export qd
// beard:  this seems to be an illegal pragma
#endif

const ResIDT nsPluginSig = 128;	// STR#, MIME type is #1

struct _np_handle;


// *************************************************************************************
//
// class CPluginHandler
//
// *************************************************************************************

class CPluginHandler
{
		friend void FE_UnloadPlugin(void* plugin, struct _np_handle* handle);
		friend NPPluginFuncs* FE_LoadPlugin(void* plugin, NPNetscapeFuncs *, struct _np_handle* handle);

public:

// �� constructors
								CPluginHandler(FSSpec& plugFile);
								~CPluginHandler();

// �� xp->macfe
	static	Boolean				CheckExistingHandlers(FSSpec& file);
			NPPluginFuncs* 		LoadPlugin(NPNetscapeFuncs * funcs, _np_handle* handle);
			void				UnloadPlugin();

// plugin -> netscape
	static	void				Status(const char * message);

	static	CPluginHandler*		FindHandlerForPlugin(const CStr255& pluginName);
	
private:

// �� funcs
			OSErr				InitCodeResource(NPNetscapeFuncs * funcs, _np_handle* handle);
			void				CloseCodeResource(_np_handle* handle = NULL);
			void				ResetPlugFuncTable();
			void				OpenPluginResourceFork(Int16 inPrivileges);
			
// �� vars
			short				fRefCount;
			PRLibrary*			fLibrary;			// For PPC
			Handle				fCode;				// For 68K
			NPPluginFuncs		fPluginFuncs;		// xp->plugin
			NPP_MainEntryUPP	fMainEntryFunc;
			NPP_ShutdownUPP		fUnloadFunc;
			LFile* 				fFile;				// File containing the plugin
			CStr255				fPluginName;		// Name of plug-in
			
			CPluginHandler*		fNextHandler;		// Link to next handler in the global list
	static	CPluginHandler*		sHandlerList;		// Global list of all handlers
};

CPluginHandler* CPluginHandler::sHandlerList = NULL;

void* GetPluginDesc(const CStr255& pluginName)
{
	return CPluginHandler::FindHandlerForPlugin(pluginName);
}
	
//
// Find a handler given a plug-in name (currently only
// called from CFrontApp:RegisterMimeType).
//
CPluginHandler*	CPluginHandler::FindHandlerForPlugin(const CStr255& pluginName)
{
	CPluginHandler* handler = CPluginHandler::sHandlerList;
	while (handler != NULL)
	{
		if (handler->fPluginName == pluginName)
			return handler;

		handler = handler->fNextHandler;
	}
	
	return nil;
}


//
// Check the handlers we already have registered against
// the given file spec.  We don�t want to redundantly
// create handlers for the same plugin file.
//
Boolean CPluginHandler::CheckExistingHandlers(FSSpec& file)
{
	CPluginHandler* handler = CPluginHandler::sHandlerList;
	while (handler != NULL)
	{
		if (handler->fFile)
		{
			FSSpec existingFile;
			handler->fFile->GetSpecifier(existingFile);
			if (existingFile.vRefNum == file.vRefNum && existingFile.parID == file.parID)
			{
				if (EqualString(existingFile.name, file.name, true, true))
					return false;
			}
		}
		handler = handler->fNextHandler;
	}
	return true;
}


// �� constructors

CPluginHandler::CPluginHandler(FSSpec& plugFile)
{
	// Variables
	fRefCount = 0;
	fCode = NULL;
	fLibrary = NULL;
	fFile = NULL;
	fMainEntryFunc = NULL;
	fUnloadFunc = NULL;
	fNextHandler = NULL;
	fPluginName = "";

	Try_
	{
		fFile = new LFile(plugFile);
		fFile->OpenResourceFork(fsRdPerm);

		// Look for the plug-in description resource
		CStringListRsrc descRsrc(126);
		CStr255 pluginDescription;
		
		// Read plug-in description if available; if not, use blank description
		short descCount = descRsrc.CountStrings();
		if (descCount > 0)
			descRsrc.GetString(1, pluginDescription);
		if (descCount <= 0 || ResError())
			pluginDescription = "";
		
		// Read plug-in name if available; if not, use file name
		if (descCount > 1)
			descRsrc.GetString(2, fPluginName);
		if (descCount <= 1 || ResError())
			fPluginName = plugFile.name;

		// cstring fileName((const unsigned char*) plugFile.name);
		char* fileName = CFileMgr::EncodedPathNameFromFSSpec(plugFile, TRUE);
		ThrowIfNil_(fileName);
		fileName = NET_UnEscape(fileName);
		NPError err = NPL_RegisterPluginFile(fPluginName, fileName, pluginDescription, this);
		XP_FREE(fileName);
		ThrowIfOSErr_(err);

		CStringListRsrc mimeList(128);
		CStringListRsrc descList(127);
		
		UInt32 mimeTotal = mimeList.CountStrings() >> 1;		// 2 strings per entry
		UInt32 descTotal = descList.CountStrings();
		UInt32 count, mimeIndex, descIndex;
		ThrowIf_(mimeTotal == 0);
		
		for (count = mimeIndex = descIndex = 1; count <= mimeTotal; count++, mimeIndex += 2, descIndex++)
		{
			CStr255 mimeType;
			CStr255 extensions;
			CStr255 description;
			
			mimeList.GetString(mimeIndex, mimeType);
			if (ResError())
				continue;
				
			mimeList.GetString(mimeIndex + 1, extensions);
			if (ResError())
				continue;
				
			//
			// Get the description string from the plug-in.
			// If they didn't specify one, use a pre-existing
			// description if it exists (e.g. we know by default
			// that video/quicktime is "Quicktime Video"). If
			// we still didn't find a description, just use the
			// MIME type.
			//
			Boolean gotDescription = false;
			if (descIndex <= descTotal)
			{
				descList.GetString(descIndex, description);
				gotDescription = (ResError() == noErr);
			}
			if (!gotDescription)
			{
				NET_cdataStruct temp;
				NET_cdataStruct* cdata;
				char* mimetype = (char*) mimeType;
				
				memset(&temp, 0, sizeof(temp));
				temp.ci.type = mimetype;
				cdata = NET_cdataExist(&temp);
				if (cdata && cdata->ci.desc)
					description = cdata->ci.desc;
				else
					description = mimeType;
			}

			//
			// Look up this type in the MIME table.  If it's not found, then
			// the user hasn't seen this plug-in type before, so make a new
			// mapper for the table with the type defaulted to be handled by
			// the plug-in.  If it was found, but was from an old prefs file
			// (that didn't contain plug-in information), make sure the plug-
			// in still gets priority so users don't freak out the first time
			// they run a new Navigator and the plug-ins stop working.  If it
			// was found but was latent (disabled because the plug-in was
			// missing), re-enable it now because the plug-in is present again.
			//
			cstring type((const unsigned char*) mimeType);
			CMimeMapper* mapper = CPrefs::sMimeTypes.FindMimeType(type);
			if (mapper == NULL)
			{
				mapper = new CMimeMapper(CMimeMapper::Plugin, mimeType, "", extensions, '????', '????');
				ThrowIfNil_(mapper);
				mapper->SetPluginName(fPluginName);
				mapper->SetDescription(description);
				CPrefs::sMimeTypes.InsertItemsAt(1, LArray::index_Last, &mapper);
				CPrefs::SetModified();
				mapper->WriteMimePrefs();			// convert mapper to xp prefs
			}
			else if (mapper->FromOldPrefs())
			{
				mapper->SetPluginName(fPluginName);
				mapper->SetExtensions(extensions);
				mapper->SetDescription(description);
				mapper->SetLoadAction(CMimeMapper::Plugin);
				CPrefs::SetModified();
				mapper->WriteMimePrefs();			// convert mapper to xp prefs
			}
			else if (mapper->LatentPlugin())
			{
				XP_ASSERT(mapper->GetLoadAction() == CMimeMapper::Unknown);
				mapper->SetLoadAction(CMimeMapper::Plugin);
				CPrefs::SetModified();
			}
			else
			{
				//
				// Only update the description if (a) there�s a description
				// from the plug-in itself (as opposed to a default description),
				// and (b) the current description is blank (we don�t want to
				// overwrite a user-edited description).
				//
				if (gotDescription && mapper->GetDescription() == "")
				{
					mapper->SetDescription(description);
					CPrefs::SetModified();
				}
			}
			
			// Now we know whether the plug-in should be enabled or not
			Boolean enabled = ((mapper->GetLoadAction() == CMimeMapper::Plugin) &&
							   (fPluginName == mapper->GetPluginName()));
			NPL_RegisterPluginType(type,  (char*) mapper->GetExtensions(), (char*) mapper->GetDescription(), NULL, this, enabled);
		}
		
		fFile->CloseResourceFork();
		
	}
	Catch_(inErr)
	{
		if (fFile != NULL)
			fFile->CloseResourceFork();
			
	}		
	EndCatch_;
	
	ResetPlugFuncTable();

	// Link us into the global list of handlers
	fNextHandler = CPluginHandler::sHandlerList;
	CPluginHandler::sHandlerList = this;
}


CPluginHandler::~CPluginHandler()			// This code never gets called
{
	// CloseCodeResource();
	
	if (fFile)
		delete fFile;
		
	if (this == CPluginHandler::sHandlerList)
		CPluginHandler::sHandlerList = NULL;
	else
	{
		CPluginHandler* handler = CPluginHandler::sHandlerList;
		while (handler != NULL)
		{
			if (this == handler->fNextHandler)
			{
				handler->fNextHandler = this->fNextHandler;
				break;
			}
			handler = handler->fNextHandler;
		}
	}
}


void CPluginHandler::ResetPlugFuncTable()
{
	fMainEntryFunc = NULL;
	fUnloadFunc = NULL;
	memset(&fPluginFuncs, 0, sizeof(NPPluginFuncs));
}


NPPluginFuncs * CPluginHandler::LoadPlugin(NPNetscapeFuncs * funcs, _np_handle* handle)
{
    // XXX Needs to be fixed for C++ Plugin API
	OSErr err = InitCodeResource(funcs, handle);
	if (err == noErr)
		return &fPluginFuncs;
	
	return NULL;
}

OSErr CPluginHandler::InitCodeResource(NPNetscapeFuncs* funcs, _np_handle* handle)
{
	fRefCount++;
	
	if (fCode != NULL || fLibrary != 0)		// Already loaded
		return noErr;
		
	OSErr err = noErr;
#ifdef DEBUG
	EDebugAction oldThrow = gDebugThrow;
	EDebugAction oldSignal = gDebugSignal;
	gDebugThrow = debugAction_Nothing;
	gDebugSignal = debugAction_Nothing;
#endif
	Try_	
	{
		OpenPluginResourceFork(fsRdWrPerm);
		Try_
		{	// PPC initialization

			if (!UEnvironment::HasGestaltAttribute(gestaltCFMAttr, gestaltCFMPresent))
				Throw_((OSErr)cfragNoLibraryErr);	// No CFM

			FSSpec fileSpec;
			fFile->GetSpecifier(fileSpec);
			
			char* cFullPath = CFileMgr::PathNameFromFSSpec(fileSpec, TRUE);
			ThrowIfNil_(cFullPath);
			
			fLibrary = PR_LoadLibrary(cFullPath);
			ThrowIfNil_(fLibrary);

			// PCB:  Let's factor this into an XP function, please!
			nsFactoryProc nsGetFactory = (nsFactoryProc) PR_FindSymbol(fLibrary, "NSGetFactory");
			if (nsGetFactory != NULL) {
				nsresult res = NS_OK;
			    if (thePluginManager == NULL) {
			    	// For now, create the plugin manager on demand.
			        static NS_DEFINE_IID(kIPluginManagerIID, NS_IPLUGINMANAGER_IID);
			        res = nsPluginManager::Create(NULL, kIPluginManagerIID, (void**)&thePluginManager);
			    	ThrowIf_(res != NS_OK || thePluginManager == NULL);
			    }
				static NS_DEFINE_IID(kIPluginIID, NS_IPLUGIN_IID);
				nsIPlugin* plugin = NULL;
				res = nsGetFactory(kIPluginIID, (nsIFactory**)&plugin);
			    ThrowIf_(res != NS_OK || plugin == NULL);
			    // beard: establish the primary reference.
			    plugin->AddRef();
			    res = plugin->Initialize((nsIPluginManager2*)thePluginManager);
			    ThrowIf_(res != NS_OK);
				handle->userPlugin = plugin;
			} else {
				fMainEntryFunc = (NPP_MainEntryUPP) PR_FindSymbol(fLibrary, "mainRD");
				ThrowIfNil_(fMainEntryFunc);
			}
		}
		Catch_(inErr)
		{	// 68K if PPC fails
			fLibrary = NULL;	// No PPC
			fCode = Get1Resource(emPluginFile, 128);
			ThrowIfNil_(fCode);
			::MoveHHi(fCode);
			::HNoPurge(fCode);
			::HLock(fCode);

			//get the address of main and typecast it into a MainEntryProcUPP
			fMainEntryFunc = (NPP_MainEntryUPP)(*fCode);
		} EndCatch_
		
		if (fMainEntryFunc != NULL) {
			fPluginFuncs.version = funcs->version;
			fPluginFuncs.size = sizeof(NPPluginFuncs);
			err = CallNPP_MainEntryProc(fMainEntryFunc, funcs, &fPluginFuncs, &fUnloadFunc);		
			ThrowIfOSErr_(err);
			if ((fPluginFuncs.version >> 8) < NP_VERSION_MAJOR)
				Throw_((OSErr)NPERR_INCOMPATIBLE_VERSION_ERROR);
		}
	}
	Catch_(inErr)
	{
		CloseCodeResource();
		err = inErr;
	}
	EndCatch_
	CPrefs::UseApplicationResFile();	// Revert the resource chain
#ifdef DEBUG
	gDebugThrow = oldThrow;
	gDebugSignal = oldSignal;
#endif
	return err;
}

// Dispose of the code
void CPluginHandler::CloseCodeResource(_np_handle* handle)
{
	// Already unloaded?
	if (fRefCount == 0)
		return;
		
	// If there are still outstanding references, don't unload yet
	fRefCount--;
	if (fRefCount > 0)
		return;

	if (handle != NULL) {
		nsIPlugin* userPlugin = handle->userPlugin;
		if (userPlugin != NULL) {
			userPlugin->Release();
			handle->userPlugin = NULL;
		}
	} else
	if (fUnloadFunc != NULL)
		CallNPP_ShutdownProc(fUnloadFunc);

// Dispose of the code
// 68K, just release the resource
	if (fCode != NULL)	
	{
		::HUnlock(fCode);
		::HPurge(fCode);
		::ReleaseResource(fCode);
		fCode = NULL;
	}
// PPC, close the connections
	if (fLibrary != NULL)
	{
		int err = PR_UnloadLibrary(fLibrary);
		Assert_(err == 0);
		fLibrary = NULL;
	}

	ResetPlugFuncTable();
	
	Try_
	{
		fFile->CloseResourceFork();
	}
	Catch_(inErr){} EndCatch_
	Try_
	{
		fFile->CloseDataFork();
	}
	Catch_(inErr){} EndCatch_
}


typedef struct ResourceMapEntry ResourceMapEntry, **ResourceMapHandle;

// See More Macintosh Toolbox, pp. 1-122, 1-123.
#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif
struct ResourceMapEntry
{
	UInt32				dataOffset;
	UInt32				mapOffset;
	UInt32				dataLength;
	UInt32				mapLength;
	ResourceMapHandle	nextMap;
	UInt16				refNum;
};
#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

//
// Open the plug-in�s resource fork BELOW the the application�s
// resource fork.  If we just opened the plug-in above the
// application but set the current res file to the app, the
// plug-in will be made current by the Resource Manager if the
// current res file is closed, which can happen when we close
// our prefs file after writing something to it.  Once the plug-
// in is the current res file, if there are any resource conflicts
// between the two we�ll get the plug-in�s resource instead of
// ours, causing woe.
//
void CPluginHandler::OpenPluginResourceFork(Int16 inPrivileges)
{
	Try_
	{
		Int16 plugRef = fFile->OpenResourceFork(inPrivileges);		// Open normally
		
		// The plug-in should now be at the top of the chain
		ResourceMapHandle plugMap = (ResourceMapHandle) LMGetTopMapHndl();
		ThrowIf_((**plugMap).refNum != plugRef);
		
		// Search down the chain to find the app�s map
		short appRef = LMGetCurApRefNum();
		ResourceMapHandle appMap = (ResourceMapHandle) LMGetTopMapHndl();
		while (appMap && (**appMap).refNum != appRef)
			appMap = (**appMap).nextMap;
		ThrowIfNil_(appMap);
		
		// Remove the plug-in from the top of the chain
		ResourceMapHandle newTop = (**plugMap).nextMap;
		ThrowIfNil_(newTop);
		LMSetTopMapHndl((Handle)newTop);
		
		// Re-insert the plug-in below the application
		(**plugMap).nextMap = (**appMap).nextMap;
		(**appMap).nextMap = plugMap;
	}
	Catch_(inErr) 
	{
	}
	EndCatch_
}





void RegisterPluginsInFolder(FSSpec folder);
// Recursive registration of all plugins
void RegisterPluginsInFolder(FSSpec folder)
{
	// find all the files that are plugins
	CFileIter iter(folder);
	FSSpec plugFile;
	FInfo finderInfo;
	Boolean isFolder;
	while (iter.Next(plugFile, finderInfo, isFolder))
	{
		if (isFolder)
			RegisterPluginsInFolder(plugFile);
		else if (finderInfo.fdType == emPluginFile)
		{
			//
			// Check the handlers we already have registered against
			// the given file spec.  We don�t want to redundantly
			// create handlers for the same plugin file.
			//
			if (CPluginHandler::CheckExistingHandlers(plugFile))
			{
				Try_
				{
					CPluginHandler* plug = new CPluginHandler(plugFile);
				}
				Catch_(inErr){}
				EndCatch_
			}
		}
#ifdef EDITOR
		else {
			unsigned char *name = &plugFile.name[0];
			int	len = name[0];
			
			/* This checks to see if the name starts with cp and ends in '.zip' or '.jar'.
			*/
			if (len >= 5) {
				if ((name[1] == 'c' || name[1] == 'C')
				&& (name[2] == 'p' || name[2] == 'P')
				&& (name[len - 3] == '.')
				&& ((name[len - 2] == 'z' || name[len - 2] == 'Z')
				&& (name[len - 1] == 'i' || name[len - 1] == 'I')
				&& (name[len - 0] == 'p' || name[len - 0] == 'P'))
				|| ((name[len - 2] == 'j' || name[len - 2] == 'J')
				&& (name[len - 1] == 'a' || name[len - 1] == 'A')
				&& (name[len - 0] == 'r' || name[len - 0] == 'R'))) {
				char *cFullPath = CFileMgr::PathNameFromFSSpec( plugFile, TRUE );
				ThrowIfNil_(cFullPath);
				char* cUnixFullPath = CFileMgr::EncodeMacPath(cFullPath);	// Frees cFullPath
				ThrowIfNil_(cUnixFullPath);
				(void) NET_UnEscape(cUnixFullPath);

				EDT_RegisterPlugin(cUnixFullPath);
				XP_FREE(cUnixFullPath);
				}
			}
		}
#endif // EDITOR
	}
	
	//
	// Tell Java to look in this directory for class files
	// so LiveConnect-enabled plug-ins can store their classes
	// with the plug-in.
	//
	Try_
	{
		char* cFullPath = CFileMgr::PathNameFromFSSpec(folder, TRUE);
		ThrowIfNil_(cFullPath);
		char* cUnixFullPath = CFileMgr::EncodeMacPath(cFullPath);	// Frees cFullPath
		ThrowIfNil_(cUnixFullPath);
		(void) NET_UnEscape(cUnixFullPath);

#if defined(JAVA)
		// Tell Java about this path name
		LJ_AddToClassPath(cUnixFullPath);
#elif defined(OJI)
		// What, tell the current Java plugin about this class path?
#endif
		XP_FREE(cUnixFullPath);
	}
	Catch_(inErr) {}
	EndCatch_
}

//
// Default handler registered below for formats FO_CACHE_AND_EMBED and
// FO_CACHE, across all types.  This handler recovers the pointer to
// our CPluginView and tells it that it does not have a valid plugin
// instance.
//
NET_StreamClass* EmbedDefault(
	int format_out, void* registration, URL_Struct* request, MWContext* context);
NET_StreamClass* EmbedDefault(
	int /*format_out*/, void* /*registration*/, URL_Struct* request, MWContext*)
{
	if (request != NULL)
	{
		NPEmbeddedApp* fe_data = (NPEmbeddedApp*) request->fe_data;
		if (fe_data != NULL)
		{
			CPluginView* plugin = (CPluginView*) fe_data->fe_data;
			if (plugin != NULL)
				plugin->SetBrokenPlugin();
		}
	}
	return NULL;
}

/* Returns the plugin folder spec */
OSErr FindPluginFolder(FSSpec * plugFolder, Boolean create);
OSErr FindPluginFolder(FSSpec * plugFolder, Boolean create)
{
	FSSpec	netscapeSpec = CPrefs::GetFolderSpec(CPrefs::NetscapeFolder);
	long		netscape_dirID;


	OSErr err;
	if ( (err=CFileMgr::GetFolderID(netscapeSpec, netscape_dirID)) == noErr )
		{
#ifndef PLUGIN_FOLDER_NAME_MUST_BE_MOVED_TO_A_RESOURCE
				/*
					Yes! It's a bugus preprocessor symbol, but it got your attention, didn't it?

					For localization, the plugin folder name should be moved to a resource, however there is no reported
					bug yet (and we're currently under change control).

					***Move only the preferred name into a resource.  Leave the old name as a constant string in the source
					(since the old name will not have been localized on any users disk).***

					In the meantime...
				*/

			Str255 pluginFolderName = "\pPlug-ins";
#else
				// The preferred (localized) name of the plugins folder is in a resource...
			Str255 pluginFolderName;
			GetIndString(pluginFolderName, ..., ...);
#endif


			FSSpec	oldNameSpec;
			Boolean	isFolder, wasAliased;

				// first look for the preferences folder with the preferred name
			if ( ((err=FSMakeFSSpec(netscapeSpec.vRefNum, netscape_dirID, pluginFolderName, plugFolder)) == noErr)		// ...if we made the FSSpec ok
				&& ((err=ResolveAliasFile(plugFolder, true, &isFolder, &wasAliased)) == noErr) )												// ...and it leads to something
				{
					// the plugins folder already exists as "Plug-ins", nothing else to do
				}

				// else, try the old name (using |oldNameSpec|, so that if not found, I can create a folder from the preferred spec created above)
			else if ( ((err=FSMakeFSSpec(netscapeSpec.vRefNum, netscape_dirID, "\pPlugins", &oldNameSpec)) == noErr)	// ...if we made the FSSpec ok
						 && ((err=ResolveAliasFile(&oldNameSpec, true, &isFolder, &wasAliased)) == noErr) )									// ...and it leads to something
				{
						// the plugins folder already exists as [deprecated] "Plugins", copy the spec we used to learn that to the caller
					FSMakeFSSpec(oldNameSpec.vRefNum, oldNameSpec.parID, oldNameSpec.name, plugFolder);
				}

				// otherwise, we may need to create it
			else if ( create )
				{
					long created_dirID;
					err = FSpDirCreate(plugFolder, smSystemScript, &created_dirID);
				}
		}

	return err;
}

// *************************************************************************************
//
// XP interface methods
//
// *************************************************************************************

void FE_RegisterPlugins();
void FE_RegisterPlugins()
{
	//
	// Register a wildcard handler for "embed" types before we scan the 
	// plugins folder -- that way, if a "null plugin" is found that
	// also registers "*"/FO_EMBED, it will be registered later than this
	// handler and thus take precendence.
	//
    NET_RegisterContentTypeConverter("*", FO_EMBED, NULL, EmbedDefault);

	OSErr err;
	FSSpec plugFolder;

	if (!IsThisKeyDown(kOptionKey))			// Skip loading plugins if option key is down
		err = FindPluginFolder(&plugFolder, false);
	else
		err = -1;

	if (err != noErr)
	{
		CFrontApp::SplashProgress(GetPString(MAC_NO_PLUGIN));
		return;
	}
	else
	{
		CFrontApp::SplashProgress(GetPString(MAC_REGISTER_PLUGINS));
		RegisterPluginsInFolder(plugFolder);
	}
	
	//
	// Now that all plug-ins have been registered, look through the MIME table
	// for types that are assigned to non-existant plug-ins or plug-in types,
	// and change all these types to use "Unknown".
	//
	LArrayIterator iterator(CPrefs::sMimeTypes);
	CMimeMapper* mapper;
	while (iterator.Next(&mapper))
	{
		if (mapper->GetLoadAction() == CMimeMapper::Plugin)				// Handled by plug-in?
		{
			Boolean foundType = false;
			char* otherPluginName = NULL;
			
			// Look for the desired plug-in
			CStr255 mapperName = mapper->GetPluginName();
			NPReference plugin = NPRefFromStart;
			char* name;
			while (NPL_IteratePluginFiles(&plugin, &name, NULL, NULL))
			{
				// Look for the desired type
				CStr255 mapperMime = mapper->GetMimeName();
				NPReference mimetype = NPRefFromStart;
				char* type;
				while (NPL_IteratePluginTypes(&mimetype, plugin, &type, NULL, NULL, NULL))
				{
					if (mapperMime == type)
					{
						if (mapperName != name)
							otherPluginName = name;
						else
							foundType = true;
						break;
					}
				}
				
			}

			if (!foundType)										// No plug-in, or no type?
			{
				if (otherPluginName)
				{
					void* pdesc = GetPluginDesc(otherPluginName);
					if (pdesc)
						NPL_EnablePluginType((char*)mapper->GetMimeName(), pdesc, TRUE);
					mapper->SetPluginName(otherPluginName);
					CPrefs::SetModified();
				}
				else
				{
					mapper->SetLatentPlugin();						// Mark the plug-in as �disabled because missing�
					CPrefs::SetModified();							// The prefs have changed
				}
			}
		}
	}
}



NPPluginFuncs * FE_LoadPlugin(void *plugin, NPNetscapeFuncs * funcs, struct _np_handle* handle)
{
	return ((CPluginHandler*)plugin)->LoadPlugin(funcs, handle);
}

void FE_UnloadPlugin(void *plugin, struct _np_handle* handle)
{
	((CPluginHandler*)plugin)->CloseCodeResource(handle);
}

//
// This exit routine is called when an embed stream completes.
// We use that opportunity to see if something when wrong when
// creating the initial stream for the plug-in so we can 
// display some helpful UI.
//
void FE_EmbedURLExit(URL_Struct* /*urls*/, int /*status*/, MWContext* /*cx*/)
{
// bing: Don't do anything here until we figure out a 
}


//
// This code is called by npn_status, which is called by the plug-in.
// The plug-in may have manipulated the port before calling us, so
// we have to assume that we're out of focus so we'll draw in the
// right place.  We also need to save and restore the entire port
// state so the plug-in's port settings are trashed.
//
void FE_PluginProgress(MWContext* cx, const char* message)
{
	GrafPtr currentPort;
	GetPort(&currentPort);
	StColorPortState portState(currentPort);
	portState.Save(currentPort);
	StUseResFile resFile(LMGetCurApRefNum());
		
	LView::OutOfFocus(nil);		// Make sure FE_Progress focuses the caption
	FE_Progress(cx, message);
	LView::OutOfFocus(nil);		// portState.Restore will screw up the focus again
	
	portState.Restore();
}


//
// See comments in npglue.c in np_geturlinternal.
//
void FE_ResetRefreshURLTimer(MWContext* )
{
Assert_(false);
//	if (NETSCAPEVIEW(context))
//		NETSCAPEVIEW(context)->ResetRefreshURLTimer();
}

// A generic way to query information. Nothing to do now.
NPError FE_PluginGetValue(MWContext *, NPEmbeddedApp *, NPNVariable ,
						  void */*r_value*/)
{
	return NPERR_NO_ERROR;
}

//
// Rather than having to scan an explicit list of windows each time an event
// comes in, why not use a sub-class of LWindow, and let PowerPlant do the work
// for us?
//

#pragma mark ### CPluginWindow ###

class CPluginWindow : public LWindow, public LPeriodical {
public:
	CPluginWindow(nsIEventHandler* eventHandler, WindowRef window);
	virtual ~CPluginWindow();

	virtual void		HandleClick(const EventRecord& inMacEvent, Int16 inPart);
	virtual void		EventMouseUp(const EventRecord &inMacEvent);
	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam);
	virtual Boolean		HandleKeyPress(const EventRecord& inKeyEvent);
	virtual void		DrawSelf();
	virtual void		SpendTime(const EventRecord& inMacEvent);
	virtual void		ActivateSelf();
	virtual void		DeactivateSelf();
	virtual void 		AdjustCursorSelf(Point inPortPt, const EventRecord& inMacEvent);

	Boolean				IsPluginCommand(CommandT inCommand);
	Boolean				PassPluginEvent(EventRecord& event);
	
private:
	nsIEventHandler*	mEventHandler;
	SInt16				mKind;
};

void FE_RegisterWindow(nsIEventHandler* handler, void* window)
{
#if 1
	LCommander::LCommander::SetDefaultCommander(LCommander::GetTopCommander());
	CPluginWindow* pluginWindow = new CPluginWindow(handler, WindowRef(window));
    XP_ASSERT(pluginWindow != NULL);
    if (pluginWindow != NULL) {
    	pluginWindow->Show();
    	pluginWindow->Select();
    }
#else
	((CPluginView*)plugin)->RegisterWindow(window);
#endif
}

void FE_UnregisterWindow(nsIEventHandler* handler, void* window)
{
#if 1
	// Toss the pluginWindow itself.
	LWindow* pluginWindow = LWindow::FetchWindowObject(WindowPtr(window));
	if (pluginWindow != NULL) {
		// Hide the window.
		pluginWindow->Hide();
		// Notify PowerPlant that the window is no longer active.
		// pluginWindow->Deactivate();
		delete pluginWindow;
	}
#else
	((CPluginView*)plugin)->UnregisterWindow(window);
#endif
}

#if 0
SInt16 FE_AllocateMenuID(void *plugin, XP_Bool isSubmenu)
{
	return ((CPluginView*)plugin)->AllocateMenuID(isSubmenu);
}
#endif

CPluginWindow::CPluginWindow(nsIEventHandler* eventHandler, WindowRef window)
{
	mEventHandler = eventHandler;
	mMacWindowP = window;

	// The following is cribbed from one of the LWindow constructors.

	// "bless" the plugin window so it will be considered to be a first-class PowerPlant window.
	mKind = ::GetWindowKind(window);
	::SetWindowKind(window, PP_Window_Kind);
	::SetWRefCon(window, long(this));

	// Set some default attributes.
	SetAttribute(windAttr_CloseBox | windAttr_TitleBar | windAttr_Resizable
				| windAttr_SizeBox | windAttr_Targetable | windAttr_Enabled);
	
	// Window Frame and Image are the same as its portRect.
	short width = mMacWindowP->portRect.right - mMacWindowP->portRect.left;
	short height = mMacWindowP->portRect.bottom - mMacWindowP->portRect.top;

	ResizeFrameTo(width, height, false);
	ResizeImageTo(width, height, false);
	
	CalcRevealedRect();
	
	// Initial size and location are the "user" state for zooming.
	CalcPortFrameRect(mUserBounds);	
	PortToGlobalPoint(topLeft(mUserBounds));
	PortToGlobalPoint(botRight(mUserBounds));

	mVisible = triState_Off;
	mActive = triState_Off;
	mEnabled = triState_Off;
	if (HasAttribute(windAttr_Enabled)) {
		mEnabled = triState_On;
	}
	
	FocusDraw();
	::GetForeColor(&mForeColor);
	::GetBackColor(&mBackColor);
	
	StartIdling();
}

CPluginWindow::~CPluginWindow()
{
	// prevent the LWindow destructor from clobbering the window.
	if (mMacWindowP != NULL) {
		WindowPtr window = mMacWindowP;
		if (IsWindowVisible(window))
			HideSelf();
	
		// Restore kind and refCon to original values.
		::SetWindowKind(window, mKind);
		::SetWRefCon(window, 0);

		mMacWindowP = NULL;
	}
}

void CPluginWindow::HandleClick(const EventRecord& inMacEvent, Int16 inPart)
{
	EventRecord mouseEvent = inMacEvent;
	if (!PassPluginEvent(mouseEvent))
		LWindow::HandleClick(inMacEvent, inPart);
}

void CPluginWindow::EventMouseUp(const EventRecord& inMouseUp)
{
	EventRecord mouseEvent = inMouseUp;
	PassPluginEvent(mouseEvent);
}

Boolean	CPluginWindow::ObeyCommand(CommandT inCommand, void *ioParam)
{
	if (IsPluginCommand(inCommand)) {
		EventRecord menuEvent;
		::OSEventAvail(0, &menuEvent);
		menuEvent.what = nsPluginEventType_MenuCommandEvent;
		menuEvent.message = -inCommand;			// PowerPlant encodes a raw menu selection as the negation of the selection.
		return PassPluginEvent(menuEvent);
	}
	return LWindow::ObeyCommand(inCommand, ioParam);
}

Boolean	CPluginWindow::HandleKeyPress(const EventRecord& inKeyEvent)
{
	EventRecord keyEvent = inKeyEvent;
	return PassPluginEvent(keyEvent);
}

void CPluginWindow::DrawSelf()
{
	EventRecord updateEvent;
	::OSEventAvail(0, &updateEvent);
	updateEvent.what = updateEvt;
	updateEvent.message = UInt32(mMacWindowP);
	PassPluginEvent(updateEvent);
}

void CPluginWindow::SpendTime(const EventRecord& inMacEvent)
{
	// Need to poll various states, because the plugin can change things like
	// the window's visibility, size, etc.

	// Put the visible state in synch with the window's visibility.
	mVisible = (IsWindowVisible(mMacWindowP) ? triState_On : triState_Off);

	// Make sure the window's size hasn't changed (what about location?)
	short width = mMacWindowP->portRect.right - mMacWindowP->portRect.left;
	short height = mMacWindowP->portRect.bottom - mMacWindowP->portRect.top;
	if (mFrameSize.width != width || mFrameSize.height != height) {
		ResizeFrameTo(width, height, false);
		ResizeImageTo(width, height, false);
		CalcRevealedRect();
	}

	EventRecord macEvent = inMacEvent;
	PassPluginEvent(macEvent);
	
	//
	// If we�re in SpendTime because of a non-null event, send a
	// null event too.  Some plug-ins (e.g. Shockwave) rely on null
	// events for animation, so it doesn�t matter if we�re giving
	// them lots of time with mouseMoved or other events -- if they
	// don�t get null events, they don�t animate fast.
	//
	if (macEvent.what != nullEvent) {
		macEvent.what = nullEvent;
		PassPluginEvent(macEvent);
	}
}

void CPluginWindow::ActivateSelf()
{
	LWindow::ActivateSelf();
	
	EventRecord activateEvent;
	::OSEventAvail(0, &activateEvent);
	activateEvent.what = activateEvt;
	activateEvent.modifiers |= activeFlag;
	activateEvent.message = UInt32(mMacWindowP);
	PassPluginEvent(activateEvent);
}


void CPluginWindow::DeactivateSelf()
{
	LWindow::DeactivateSelf();

	EventRecord activateEvent;
	::OSEventAvail(0, &activateEvent);
	activateEvent.what = activateEvt;
	activateEvent.modifiers &= ~activeFlag;
	activateEvent.message = UInt32(mMacWindowP);
	PassPluginEvent(activateEvent);
}

void CPluginWindow::AdjustCursorSelf(Point inPortPt, const EventRecord& inMacEvent)
{
	EventRecord cursorEvent = inMacEvent;
	cursorEvent.what = nsPluginEventType_AdjustCursorEvent;
	if (!PassPluginEvent(cursorEvent))
		LWindow::AdjustCursorSelf(inPortPt, inMacEvent);
}

Boolean CPluginWindow::IsPluginCommand(CommandT inCommand)
{
#if 1
	// Since only one plugin can have menus in the menu bar at a time,
	// the test only checks to see if this plugin has any menus, and
	// whether the command is synthetic and is from one of the plugin's menus.
	if (thePluginManager != NULL) {
		short menuID, menuItem;
		if (LCommander::IsSyntheticCommand(inCommand, menuID, menuItem)) {
			PRBool hasAllocated = PR_FALSE;
			if (thePluginManager->HasAllocatedMenuID(mEventHandler, menuID, &hasAllocated) == NS_OK)
				return hasAllocated;
		}
	}
	return false;
#else
	return mPlugin->IsPluginCommand(inCommand);
#endif
}

Boolean CPluginWindow::PassPluginEvent(EventRecord& event)
{
#if 1
	nsPluginEvent pluginEvent = { &event, mMacWindowP };
	PRBool eventHandled = PR_FALSE;
	mEventHandler->HandleEvent(&pluginEvent, &eventHandled);
	return eventHandled;
#else
	return mPlugin->PassWindowEvent(event, mMacWindowP))
#endif
}


// *************************************************************************************
//
// CPluginView methods
//
// *************************************************************************************

#pragma mark ### CPluginView ###

CPluginView* CPluginView::sPluginTarget = NULL;
LArray* CPluginView::sPluginList = NULL;

//
// This static method lets the caller broadcast a mac event to
// all existing plug-ins (regardless of context).  It�s used by
// CFrontApp::EventSuspendResume to broadcast suspend and resume
// events to plug-ins.
//
void CPluginView::BroadcastPluginEvent(const EventRecord& event)
{
	if (sPluginList != NULL)
	{
		EventRecord eventCopy = event;		// event is const, but PassEvent isn�t
		LArrayIterator iterator(*sPluginList);
		CPluginView* plugin = NULL;
		while (iterator.Next(&plugin))
			(void) plugin->PassEvent(eventCopy);
	}
}

// XXX	The following two methods are obsolete -- CPluginWindow now does the work.

// This gets called for every event - a performance analysis/review would be good
Boolean CPluginView::PluginWindowEvent(const EventRecord& event)
{
	WindowPtr hitWindow;
	
	switch(event.what)
	{
		case mouseDown:
		case mouseUp:
			::FindWindow(event.where, &hitWindow);
			break;
			
		case autoKey:
		case keyDown:
		case keyUp:
			hitWindow = ::FrontWindow();
			break;

		case activateEvt:
		case updateEvt:
			hitWindow = (WindowPtr) event.message;
			break;

		case nullEvent:
		case diskEvt:
		case osEvt:
		case kHighLevelEvent: 		
		default:
			return false;
	}

#if 1	
	// Determine which plugin owns this window.
	Boolean isActivateEvent = (event.what == activateEvt && (event.modifiers & activeFlag));
	CPluginView* owningPlugin = FindPlugin(hitWindow);
	if (owningPlugin != NULL) {
		EventRecord eventCopy = event;
		return owningPlugin->PassWindowEvent(eventCopy, hitWindow);
	}
#else
	if (sPluginList != NULL) {
		// iterate through each plugin instance
		LArrayIterator pluginIterator(*sPluginList);
		CPluginView* plugin = NULL;
		while (pluginIterator.Next(&plugin)) {
			if (plugin->fWindowList != NULL) {
				// then iterate through each window for each instance
				LArrayIterator windowIterator(*(plugin->fWindowList));
				WindowPtr window = NULL;
				while (windowIterator.Next(&window)) {
					if (window == hitWindow) {
						EventRecord eventCopy = event;
						return plugin->PassWindowEvent(eventCopy, window);
					}
				}
			}
		}
	}
#endif

	// we didn't find a match
	return false;
}

// Determines which plugin owns the specified window.

CPluginView* CPluginView::FindPlugin(WindowPtr window)
{
	// Make sure the window isn't a PowerPlant window.
	LWindow* windowObj = LWindow::FetchWindowObject(window);
	if (windowObj == NULL && sPluginList != NULL) {
		// iterate through each plugin instance
		LArrayIterator pluginIterator(*sPluginList);
		CPluginView* plugin = NULL;
		while (pluginIterator.Next(&plugin)) {
			if (plugin->fWindowList != NULL) {
				// then iterate through each window for each instance
				LArrayIterator windowIterator(*(plugin->fWindowList));
				WindowPtr pluginWindow = NULL;
				while (windowIterator.Next(&pluginWindow)) {
					if (window == pluginWindow)
						return plugin;
				}
			}
		}
	}
	return NULL;
}

void CPluginView::RegisterWindow(void* window)
{
	// Register the pluginWindow with PowerPlant.
	
	// Set the default commander to the application, to limit the depth of the chain of command.
	// I've seen this get ridiculously deep.
	LCommander::LCommander::SetDefaultCommander(LCommander::GetTopCommander());

#if 0
	CPluginWindow* pluginWindow = new CPluginWindow(this, WindowPtr(window));
	
	if (fWindowList == NULL)
		fWindowList = new LArray;
	if (fWindowList != NULL)
		fWindowList->InsertItemsAt(1, 0, &window);
#endif
}

void CPluginView::UnregisterWindow(void* window)
{
	// Toss the pluginWindow itself.
	LWindow* pluginWindow = LWindow::FetchWindowObject(WindowPtr(window));
	if (pluginWindow != NULL) {
		// Notify PowerPlant that the window is no longer active.
		pluginWindow->Deactivate();
		delete pluginWindow;
	}

#if 0
	if (fWindowList != NULL) {
		Int32 index = fWindowList->FetchIndexOf(&window);
		if (index > 0)
			fWindowList->RemoveItemsAt(1, index);
				
		if (fWindowList->GetCount() == 0) {
			delete fWindowList;
			fWindowList = NULL;
		}
	}
#endif
}


SInt16 CPluginView::AllocateMenuID(Boolean isSubmenu)
{
	SInt16 menuID = LMenuSharingAttachment::AllocatePluginMenuID(isSubmenu);

	if (fMenuList == NULL)
		fMenuList = new TArray<SInt16>;
	if (fMenuList != NULL)
		fMenuList->AddItem(menuID);
	
	return menuID;
}

Boolean CPluginView::IsPluginCommand(CommandT inCommand)
{
	// Since only one plugin can have menus in the menu bar at a time,
	// the test only checks to see if this plugin has any menus, and
	// whether the command is synthetic and is from one of the plugin's menus.
	if (fMenuList != NULL) {
		short menuId, menuItem;
		if (LCommander::IsSyntheticCommand(inCommand, menuId, menuItem)) {
			TArray<SInt16>& menus = *fMenuList;
			UInt32 count = menus.GetCount();
			for (UInt32 i = count; i > 0; --i)
				if (menus[i] == menuId)
					return true;
		}
	}
	return false;
}

Boolean CPluginView::PassWindowEvent(EventRecord& inEvent, WindowPtr window)
{
	Boolean eventHandled = false;
	if (fApp) {
		eventHandled = NPL_HandleEvent(fApp, (NPEvent*)&inEvent, (void*) window);
	}
	return eventHandled;
}

CPluginView::CPluginView(LStream *inStream) : LView(inStream), LDragAndDrop(nil, this)
{
	fApp = NULL;
	fOriginalView = NULL;
	fBrokenPlugin = false;
	fPositioned = false;
	fHidden = false;
	fWindowed = true;
	fBrokenIcon = NULL;
	fIsPrinting = false;
	fWindowList = NULL;
	fMenuList = NULL;
		
	//
	// Add the new plug-in to a global list of all plug-ins.
	//
	if (sPluginList == NULL)
		sPluginList = new LArray;
	if (sPluginList != NULL)
		sPluginList->InsertItemsAt(1, 0, &this);


	//
	// Can�t call ResetDrawRect yet because it needs a port
	// from our superview, but we don�t have a superview yet!
	//
}


CPluginView::~CPluginView()
{
	if (fBrokenIcon != NULL)
		CIconList::ReturnIcon(fBrokenIcon);
		
	//
	// This static is parallel to LCommander::sTarget, but only for plug-ins.
	// The LCommander destructor takes care of resetting the LCommander::sTarget,
	// and so the CPluginView destructor should take care of the special plug-in target. 
	//
	if (CPluginView::sPluginTarget == this)
		CPluginView::sPluginTarget = NULL;

	// we're assuming the plugin has already killed the windows themselves
	if (fWindowList != NULL)
		delete fWindowList;
	
	// release the menu IDs used by this plugin?
	if (fMenuList != NULL)
		delete fMenuList;
	
	//
	// Remove the deleted plug-in from the global list of all plug-ins.
	//
	if (sPluginList != NULL) {
		Int32 index = sPluginList->FetchIndexOf(&this);
		if (index > 0)
			sPluginList->RemoveItemsAt(1, index);
				
		if (sPluginList->GetCount() == 0)
		{
			delete sPluginList;
			sPluginList = NULL;
		}
	}
};


void CPluginView::EmbedSize(LO_EmbedStruct* embed_struct, SDimension16 hyperSize)
{
	//
	// If the plug-in is hidden, set the width and height to zero and
	// set a flag indicating that we are hidden.
	//
	if (embed_struct->objTag.ele_attrmask & LO_ELE_HIDDEN)
	{
		embed_struct->objTag.width = 0;
		embed_struct->objTag.height = 0;
		fHidden = true;
		Hide();
		StartIdling();		// Visible plug-ins start idling in EmbedDisplay
	}
	
	//
	// If the embed src is internal-external-plugin, the plugin is
	// full-screen, and we should set up the plugin�s real width
	// for layout since it doesn�t know how big to make it.  Since a full-
	// screen plugin should resize when its enclosing view (the hyperview)
	// resizes, we bind the plugin view on all sides to its superview.
	//
	Boolean fullPage = false;
    if (embed_struct->embed_src)
    {
		char* theURL;
	    PA_LOCK(theURL, char*, embed_struct->embed_src);
	    XP_ASSERT(theURL);
	    if (XP_STRCMP(theURL, "internal-external-plugin") == 0)
	    	fullPage = true;
	    PA_UNLOCK(embed_struct->embed_src);
	}
	
	if (fullPage)
	{
		SBooleanRect binding = {true, true, true, true};
		SetFrameBinding(binding);
		
		embed_struct->objTag.width = hyperSize.width;
		embed_struct->objTag.height = hyperSize.height;
		
		//
		// Remember an offset for the view to
		// compensate for layout's default margins.
		//
		const short kLeftMargin = 8;				// ��� These should be defined in mhyper.h!!!
		const short kTopMargin = 8;
		fHorizontalOffset = -kLeftMargin;
		fVerticalOffset = -kTopMargin;
	}
	else
	{
		fHorizontalOffset = 0;
		fVerticalOffset = 0;
	}

	ResizeImageTo(embed_struct->objTag.width, embed_struct->objTag.height, false);
	ResizeFrameTo(embed_struct->objTag.width, embed_struct->objTag.height, false);

	//
	// NOTE: The position set here is not really valid because the x and y in 
	// embed_struct are not valid yet (we have to wait until DisplayEmbed
	// (see below) is called the first time to get the real location).
	// We go ahead and position ourselves anyway just so we have a superview
	// and location initially.
	//
	Int32 x = embed_struct->objTag.x + embed_struct->objTag.x_offset + fHorizontalOffset;
	Int32 y = embed_struct->objTag.y + embed_struct->objTag.y_offset + fVerticalOffset;
	PlaceInSuperImageAt(x, y, false);
	
	//
	// Set up the NPWindow and NPPort for the first time.  No one should
	// have called NPL_EmbedSize before this point, or bogus values will
	// get passed to the plugin!
	//	
	ResetDrawRect();
}


void CPluginView::EmbedDisplay(LO_EmbedStruct* embed_struct, Boolean isPrinting)
{
	fWindowed = (Boolean)NPL_IsEmbedWindowed(fApp);
	fIsPrinting = isPrinting;

	//
	// If we�re not positioned yet, place our instance at the
	// location specified by layout and make our view visible.
	//
	if (fPositioned == false)
	{
		Int32 x = embed_struct->objTag.x + embed_struct->objTag.x_offset + fHorizontalOffset;
		Int32 y = embed_struct->objTag.y + embed_struct->objTag.y_offset + fVerticalOffset;
		PlaceInSuperImageAt(x, y, false);
		if (fWindowed)
			Show();
		else if (!fBrokenPlugin) {
			fHidden = true;
			Hide();
		}
		StartIdling();
		fPositioned = true;

		XP_ASSERT(fApp);
		if (fApp != NULL)
			NPL_EmbedSize(fApp);	// Tell the plugin about its dimensions and location
		
		Refresh();
	}
	
	// If this is a windowed plug-in, this call indicates that we've moved or our visibility
	// changed.
	if (fWindowed || fBrokenPlugin)
	{
		Rect frameRect;
		SPoint32 imagePoint;
		Int32 x, y;
		
		if (IsVisible() && (embed_struct->objTag.ele_attrmask & LO_ELE_INVISIBLE))
			Hide();
			
		CalcPortFrameRect(frameRect);
		GetSuperView()->PortToLocalPoint(topLeft(frameRect));
		GetSuperView()->LocalToImagePoint(topLeft(frameRect), imagePoint);
		
		x = embed_struct->objTag.x + embed_struct->objTag.x_offset + fHorizontalOffset;
		y = embed_struct->objTag.y + embed_struct->objTag.y_offset + fVerticalOffset;
		if ((imagePoint.h != x) || (imagePoint.v != y))
			PlaceInSuperImageAt(x, y, true);
		
		if (!IsVisible() && !(embed_struct->objTag.ele_attrmask & LO_ELE_INVISIBLE))
			Show();
	}
	// For a windowless plug-in, this is where the plug-in actually draws.
	else 
	{
		EventRecord updateEvent;
		//
		// Always reset the drawing info before calling the plugin to draw.
		//
		ResetDrawRect();

		::OSEventAvail(0, &updateEvent);
		updateEvent.what = updateEvt;
		(void) PassEvent(updateEvent);
	}
}


void CPluginView::EmbedCreate(MWContext* context, LO_EmbedStruct* embed_struct)
{
	fEmbedStruct = embed_struct;
	fApp = (NPEmbeddedApp*) embed_struct->objTag.FE_Data;
	Boolean printing = (context->type == MWContextPrint);
	
	//
	// Set the references between the view, the app, and the layout
	// structure.  Also set the pointer to the new instance�s
	// window data (NPWindow), which is stored as part of our object
	// (unless we are printing, in which case it already has a NPWindow).
	// Then start up the stream for the plug-in (if applicable).
	// If NPL_EmbedStart fails, it will delete the NPEmbeddedApp and
	// associated XP data structures, so be sure to remove the
	// embed_struct's reference to it.
	//
	if (fApp != NULL)
	{	
		if (printing)
			fOriginalView = (CPluginView*) fApp->fe_data;
		fApp->fe_data = this;
		embed_struct->objTag.FE_Data = fApp;
		if (!printing)
			fApp->wdata = GetNPWindow();
		NPError err = NPL_EmbedStart(context, embed_struct, fApp);
		if (err != NPERR_NO_ERROR)
			fApp = NULL;
	}
	
	//
	// Something went wrong? If we have no app, then layout can never
	// tell us to delete the view (since the app is in the FE_Data of
	// the layout structure), so we need to delete it here.
	//
	if (fApp == NULL)
	{
		delete this;
		embed_struct->objTag.FE_Data = NULL;
	}
}


void CPluginView::EmbedFree(MWContext* context, LO_EmbedStruct* embed_struct)
{
	//
	// Set our reference to the NPEmbeddedApp to NULL now, before
	// calling NPL_EmbedDelete.  NPL_EmbedDelete may re-call FE code
	// after deleting the NPEmbeddedApp and we don�t want to have
	// a reference to a deleted app!
	//
	// XXX This is no longer necessary, as this is called in _response_
	// to NPL_EmbedDelete.
	NPEmbeddedApp* app = fApp;
	fApp = NULL;
	
	//
	// If our original view field was set above in EmbedCreate, the
	// the NPEmbeddedApp associated with this view was associated
	// only temporarily for printing, so we should switch its 
	// references to its view and window back to their original values.
	// If there is no original view, then we must be the original
	// creator of the NPEmbeddedApp, so we should delete it.
	//
	if (fOriginalView != NULL)
	{
		app->fe_data = fOriginalView;
		XP_ASSERT(app->wdata == fOriginalView->GetNPWindow());
		app->wdata = fOriginalView->GetNPWindow();
	}

	ThrowIfNil_(context);
}



// �� event processing

void CPluginView::ClickSelf(const SMouseDownEvent& inMouseDown)
{
	XP_ASSERT(fWindowed);
	LView::ClickSelf(inMouseDown);
	
	if (!fBrokenPlugin)
	{
		EventRecord mouseEvent = inMouseDown.macEvent;		// inMouseDown is const, but PassEvent isn�t
		(void) PassEvent(mouseEvent);
		LCommander::SwitchTarget(this);						// Once clicked, we can get keystrokes
	}
}

void CPluginView::EventMouseUp(const EventRecord& inMouseUp)
{
	XP_ASSERT(fWindowed);
	LView::EventMouseUp(inMouseUp);
	
	EventRecord mouseEvent = inMouseUp;						// inMouseUp is const, but PassEvent isn�t
	(void) PassEvent(mouseEvent);
}


Boolean	CPluginView::ObeyCommand(CommandT inCommand, void *ioParam)
{
	if (IsPluginCommand(inCommand)) {
		// assume this is a plugin menu item, since menusharing didn't handle it.
		EventRecord menuEvent;
		::OSEventAvail(0, &menuEvent);
		menuEvent.what = nsPluginEventType_MenuCommandEvent;
		menuEvent.message = -inCommand;			// PowerPlant encodes a raw menu selection as the negation of the selection.
		return PassEvent(menuEvent);
	}
	return LCommander::ObeyCommand(inCommand, ioParam);
}

Boolean	CPluginView::HandleKeyPress(const EventRecord& inKeyEvent)
{
	if (fWindowed) {
		EventRecord keyEvent = inKeyEvent;						// inKeyEvent is const, but PassEvent isn�t
		return PassEvent(keyEvent);
	}
	
	return false;
}


void CPluginView::DrawSelf()
{
	XP_ASSERT(!fHidden);

	if (fPositioned == false)		// Bail if layout hasn�t positioned us yet
		return;
		
	if (fBrokenPlugin)
		DrawBroken(false);
	else
	{
		//
		// Always reset the drawing info before calling the plugin to draw.
		// Although the MoveBy and ResizeFrameBy methods ensure that the
		// drawing info is updated if the plugin is scrolled or its window
		// is resized, the clip still needs to be set up every time before
		// drawing because the area to draw will be different.
		//
		ResetDrawRect();


		if (fIsPrinting)
			{
			this->PrintEmbedded();
			fIsPrinting = false;
			}
		else
			{
			EventRecord updateEvent;
			::OSEventAvail(0, &updateEvent);
			updateEvent.what = updateEvt;
			(void) PassEvent(updateEvent);
			}
	}
}



void CPluginView::SpendTime(const EventRecord& inMacEvent)
{
	EventRecord macEvent = inMacEvent;		// inMacEvent is const, but PassEvent isn�t
	(void) PassEvent(macEvent);
	
	//
	// If we�re in SpendTime because of a non-null event, send a
	// null event too.  Some plug-ins (e.g. Shockwave) rely on null
	// events for animation, so it doesn�t matter if we�re giving
	// them lots of time with mouseMoved or other events -- if they
	// don�t get null events, they don�t animate fast.
	//
	if (macEvent.what != nullEvent)
	{
		macEvent.what = nullEvent;
		(void) PassEvent(macEvent);
	}
}


void CPluginView::ActivateSelf()
{
	EventRecord activateEvent;
	::OSEventAvail(0, &activateEvent);
	activateEvent.what = activateEvt;
	activateEvent.modifiers = activeFlag;
	(void) PassEvent(activateEvent);
}


void CPluginView::DeactivateSelf()
{
	EventRecord activateEvent;
	::OSEventAvail(0, &activateEvent);
	activateEvent.what = activateEvt;
	(void) PassEvent(activateEvent);
}

//
// These two methods are called when we are targeted or
// untargeted.  Since plugins want to know when they have
// the key focus, we create a new event type to pass to
// them.
//

void CPluginView::BeTarget()
{
	XP_ASSERT(!fHidden);
	CPluginView::sPluginTarget = this;					// Parallel to LCommander::sTarget, except only for plug-ins
	EventRecord focusEvent;
	::OSEventAvail(0, &focusEvent);
	focusEvent.what = nsPluginEventType_GetFocusEvent;
	Boolean handled = PassEvent(focusEvent);
	if (!handled)										// If the plugin doesn�t want the focus,
		SwitchTarget(GetSuperCommander());				//		switch the focus back to our super
}

void CPluginView::DontBeTarget()
{
	CPluginView::sPluginTarget = NULL;					// Parallel to LCommander::sTarget, except only for plug-ins
	EventRecord focusEvent;
	::OSEventAvail(0, &focusEvent);
	focusEvent.what = nsPluginEventType_LoseFocusEvent;
	(void) PassEvent(focusEvent);
}


void CPluginView::AdjustCursorSelf(Point inPortPt, const EventRecord& inMacEvent)
{
	XP_ASSERT(!fHidden);
	EventRecord cursorEvent = inMacEvent;
	cursorEvent.what = nsPluginEventType_AdjustCursorEvent;
	
	Boolean handled = PassEvent(cursorEvent);
	if (!handled)
		LPane::AdjustCursorSelf(inPortPt, inMacEvent);
}



// �� positioning

void  CPluginView::AdaptToSuperFrameSize(Int32 inSurrWidthDelta, Int32 inSurrHeightDelta, Boolean inRefresh)
{
	LView::AdaptToSuperFrameSize(inSurrWidthDelta, inSurrHeightDelta, inRefresh);

	//
	// Our superview has changed size, so we could have changed size
	// (if we�re bound to the superview) or changed clip (if we now
	// overlap the edges of the superview).
	//
	if (fWindowed)
		ResetDrawRect();
	
	//
	// If we are bound to our superview, we will change size as it
	// changes size.  Since the superview (the hyperview) is in turn
	// bound to the window, we should adjust our NPWindow structure
	// (done above in ResetDrawRect) and tell the plugin about the
	// change (by calling NPL_EmbedSize).  -bing 11/15/95
	//
	if (fApp != NULL)
	{
		SBooleanRect frameBinding;
		GetFrameBinding(frameBinding);
		if (frameBinding.top && frameBinding.bottom && frameBinding.left && frameBinding.right)
			NPL_EmbedSize(fApp);
	}
}

void CPluginView::MoveBy(Int32 inHorizDelta, Int32 inVertDelta, Boolean inRefresh)
{
	LView::MoveBy(inHorizDelta, inVertDelta, inRefresh);
	if (fWindowed)
		ResetDrawRect();
}



// ���dragging

void CPluginView::AdaptToNewSurroundings()
{
	LView::AdaptToNewSurroundings();

	//
	// Set the port for dragging here, since we need 
	// a superview to get the port, and we don't have
	// a superview until this method is called.
	//
	LDropArea::RemoveDropArea((LDropArea*)this, mDragWindow);
	mDragWindow = GetMacPort();
	LDropArea::AddDropArea((LDropArea*)this, mDragWindow);
}

Boolean	CPluginView::DragIsAcceptable(DragReference /*inDragRef*/)
{
	//
	// We claim to accept drops, so we won't get the
	// Navigator drag&drop behavior over the plug-in.
	// If the plug-in installs its own drag handlers,
	// it will get control after we're called and can
	// handle the drag as it sees fit.
	//
	return true;
}

void CPluginView::HiliteDropArea(DragReference /*inDragRef*/)
{
	// Don�t hilite: let the plug-in do it if it accepts drags.
}

void CPluginView::UnhiliteDropArea(DragReference /*inDragRef*/)
{
	// Don�t unhilite: let the plug-in do it if it accepts drags.
}




// ���events

// Passes the event to our plugin
Boolean CPluginView::PassEvent(EventRecord& inEvent)
{
	if (fApp)
		return NPL_HandleEvent(fApp, (NPEvent*)&inEvent, NULL);
	else
		return false;
}


void CPluginView::ResetDrawRect()
{
	fNPWindow.window = &fNPPort;

	if (fWindowed) {

		fNPPort.port = (CGrafPtr) GetMacPort();
		fNPWindow.x = mImageLocation.h;
		fNPWindow.y = mImageLocation.v;
		fNPWindow.width = mFrameSize.width;
		fNPWindow.height = mFrameSize.height;
	
		if (mRevealedRect.left < mRevealedRect.right)	// Code from FocusDraw
		{		// We are visible
			XP_ASSERT(!fHidden);
			fNPPort.portx = mPortOrigin.h;
			fNPPort.porty = mPortOrigin.v;

			fNPWindow.clipRect.top = mRevealedRect.top;
			fNPWindow.clipRect.left = mRevealedRect.left;
			fNPWindow.clipRect.right = mRevealedRect.right;
			fNPWindow.clipRect.bottom = mRevealedRect.bottom;
		}
		else	// We are invisible
		{
			fNPWindow.clipRect.top = 0;
			fNPWindow.clipRect.left = 0;
			fNPWindow.clipRect.right = 0;
			fNPWindow.clipRect.bottom = 0;
		}
	}
	else {
		Rect frame;
		Point localPoint, portPoint;
		SPoint32 imagePoint;
		Rect clipRect;
		Point portOrigin;
		
		// XXX This is gross. We're getting our parent and casting it down to a
		// CHTMLView so that we can get element position and port information from it.
		CHTMLView *parentView = (CHTMLView *)GetSuperView();

		// Get the parent's port and port origin. Note that this will be different
		// depending on whether it's onscreen or offscreen.
		fNPPort.port = (CGrafPtr) parentView->GetCurrentPort(portOrigin);
		
		// Get the local position from the hyperview. This is a function of the
		// location of the layout element, the scrolled position and the position
		// of the layer containing the embed.
		parentView->CalcElementPosition((LO_Element *)fEmbedStruct, frame);
		
		// Convert it into image coordinates
		localPoint.h = frame.left - (fEmbedStruct->objTag.x + fEmbedStruct->objTag.x_offset);
		localPoint.v = frame.top - (fEmbedStruct->objTag.y + fEmbedStruct->objTag.y_offset);
		portPoint = localPoint;
		localPoint.h -= portOrigin.h;
		localPoint.v -= portOrigin.v;
		parentView->LocalToImagePoint(localPoint, imagePoint);
		
		fNPWindow.x = imagePoint.h;
		fNPWindow.y = imagePoint.v;
		fNPWindow.width = frame.right - frame.left;
		fNPWindow.height = frame.bottom - frame.top;
		
		fNPPort.portx = -localPoint.h;
		fNPPort.porty = -localPoint.v;
		
		// The clipRect in this case is the bounding box of the clip region
		// set by the compositor. Convert to port coordinates.
		clipRect = (*fNPPort.port->clipRgn)->rgnBBox;
		topLeft(clipRect).h -= portOrigin.h;
		topLeft(clipRect).v -= portOrigin.v;
		botRight(clipRect).h -= portOrigin.h;
		botRight(clipRect).v -= portOrigin.v;
		
		fNPWindow.clipRect.top = clipRect.top;
		fNPWindow.clipRect.left = clipRect.left;
		fNPWindow.clipRect.right = clipRect.right;
		fNPWindow.clipRect.bottom = clipRect.bottom;
		
		// Keep the view's position in sync with that specified by layout
		parentView->LocalToPortPoint(portPoint);
		if ((portPoint.h != -mPortOrigin.h) || (portPoint.v != -mPortOrigin.v)) {
			MoveBy(mPortOrigin.h + portPoint.h, mPortOrigin.v + portPoint.v, false);
		}
	}
}


//
// This method is used for printing fullscreen plugins, which might
// want the opportunity to completely take over printing the page.
// We will pass them whether the user clicked on the print button
// or the Print menu (�printOne�), a handle to the print record,
// and a boolean that they can set to true if they actually want to
// handle the print.  We return this boolean to our caller (CHyperView:
// ObeyCommand) so they know whether they should print or not.
// If plugins do not implement the Print entry point at all, the 
// boolean �pluginPrinted� will still be at its default value (false),
// so we will still print.
//
Boolean CPluginView::PrintFullScreen(Boolean printOne, THPrint printRecHandle)
{
	if (fApp == NULL)
		return false;

	NPPrint npPrint;
	npPrint.mode = NP_FULL;
	npPrint.print.fullPrint.pluginPrinted = false;
	npPrint.print.fullPrint.printOne = printOne;
	npPrint.print.fullPrint.platformPrint = (void*) printRecHandle;
	
	NPL_Print(fApp, (void*) &npPrint);
		
	return npPrint.print.fullPrint.pluginPrinted;
}

void CPluginView::PrintEmbedded()
{
	if (fApp == NULL)
		return;

	NPPrint npPrint;
	npPrint.mode = NP_EMBED;
	npPrint.print.embedPrint.window = fNPWindow;
	npPrint.print.embedPrint.platformPrint = (void*) fNPWindow.window; 	// Pass the NP_Port
	
	NPL_Print(fApp, (void*) &npPrint);
}




void CPluginView::SetBrokenPlugin()
{
	fBrokenPlugin = true;
	fBrokenIcon = CIconList::GetIcon(326);		// ��� Need a constant
	// Tell layout that we're windowed so we can display an icon
	LO_SetEmbedType(fEmbedStruct, PR_TRUE);
	Refresh();									// Plugin will draw broken icon now that fBrokenPlugin is set
}


void CPluginView::DrawBroken(Boolean hilite)
{
	//
	// If the plugin is broken, we don�t have a valid plugin instance
	// to ask to draw, so we should handle drawing ourselves.
	// Current behavior is to draw a box with a broken plugin icon
	// in it.
	//
	Rect drawRect;
	if (this->CalcLocalFrameRect(drawRect))		// We want a local rect since SetOrigin has been called
	{		
		short height = drawRect.bottom - drawRect.top;
		short width = drawRect.right - drawRect.left;
		
		if (height >= 4 && width >= 4)
		{
			UGraphics::SetFore(CPrefs::Anchor);
			FrameRect(&drawRect);
			
			if (height >= 32 && width >= 32 && fBrokenIcon != NULL)
			{
				short centerX = (drawRect.right - drawRect.left) >> 1;
				short centerY = (drawRect.bottom - drawRect.top) >> 1;
				drawRect.top = centerY - 16;
				drawRect.bottom = centerY + 16;
				drawRect.left = centerX - 16;
				drawRect.right = centerX + 16;
				
				::PlotCIconHandle(&drawRect, atAbsoluteCenter, hilite ? ttSelected : ttNone, fBrokenIcon);
			}
		}
	}
}


#ifdef LAYERS
Boolean CPluginView::HandleEmbedEvent(CL_Event *event)
{
	fe_EventStruct *fe_event = (fe_EventStruct *)event->fe_event;

	EventRecord macEvent;
	::OSEventAvail(0, &macEvent);

	// For windowless plugins, the last draw might have been to the offscreen port,
	// but for event handling, we want the current port to be the onscreen port (so
	// that mouse coordinate conversion and the like can happen correctly).
	if (!fWindowed)
		FocusDraw();
		
	switch (event->type) {
		case CL_EVENT_MOUSE_BUTTON_DOWN:
			{
				//SMouseDownEvent *mouseDown = (SMouseDownEvent *)fe_event->event;
				
				// Now pass the mouse event
				//macEvent = mouseDown->macEvent;
				// modified for new fe_EventStruct 1997-02-22 mjc
				SMouseDownEvent mouseDown = fe_event->event.mouseDownEvent;
				macEvent = mouseDown.macEvent;
			}
			break;
		case CL_EVENT_MOUSE_BUTTON_UP:
		case CL_EVENT_KEY_DOWN:
			//macEvent = *(EventRecord *)fe_event->event;
			macEvent = fe_event->event.macEvent; // 1997-02-22 mjc
			break;
		case CL_EVENT_MOUSE_MOVE:
			//macEvent = *(EventRecord *)fe_event->event;
			macEvent = fe_event->event.macEvent; // 1997-02-22 mjc
			macEvent.what = nsPluginEventType_AdjustCursorEvent;
			break;
		case CL_EVENT_KEY_FOCUS_GAINED:
			macEvent.what = nsPluginEventType_GetFocusEvent;
			break;
		case CL_EVENT_KEY_FOCUS_LOST:
			macEvent.what = nsPluginEventType_LoseFocusEvent;
			break;
		default:
			return false;
	}
	
	return PassEvent(macEvent);
}
#endif // LAYERS
