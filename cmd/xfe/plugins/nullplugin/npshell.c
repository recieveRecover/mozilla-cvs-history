/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
/*
 * npshell.c
 *
 * Netscape Client Plugin API
 * - Function that need to be implemented by plugin developers
 *
 * This file defines a "shell" plugin that plugin developers can use
 * as the basis for a real plugin.  This shell just provides empty
 * implementations of all functions that the plugin can implement
 * that will be called by Netscape (the NPP_xxx methods defined in 
 * npapi.h). 
 *
 * dp Suresh <dp@netscape.com>
 * updated 5/1998 <pollmann@netscape.com>
 *
 */

#include <stdio.h>
#include <Xm/Xm.h>
#include "npapi.h"
#include "nullplugin.h"

/***********************************************************************
 *
 * Implementations of plugin API functions
 *
 ***********************************************************************/

char*
NPP_GetMIMEDescription(void)
{
	return(MIME_TYPES_HANDLED);
}

NPError
NPP_GetValue(void *instance, NPPVariable variable, void *value)
{
	NPError err = NPERR_NO_ERROR;

	switch (variable) {
		case NPPVpluginNameString:
			*((char **)value) = PLUGIN_NAME;
			break;
		case NPPVpluginDescriptionString:
			*((char **)value) = PLUGIN_DESCRIPTION;
			break;
		default:
			err = NPERR_GENERIC_ERROR;
	}
	return err;
}

NPError
NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

jref
NPP_GetJavaClass()
{
    return NULL;
}

void
NPP_Shutdown(void)
{
}

NPError 
NPP_New(NPMIMEType pluginType,
	NPP instance,
	uint16 mode,
	int16 argc,
	char* argn[],
	char* argv[],
	NPSavedData* saved)
{
        PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
		
	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
	
	This = (PluginInstance*) instance->pdata;
	memset(This, 0, sizeof(PluginInstance));

	if (This != NULL)
	{
		/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
		This->mode = mode;
		This->window = (Window) 0;
		This->type = dupMimeType(pluginType);
		This->instance = instance;
		This->pluginsPageUrl = NULL;

		/* Parse argument list passed to plugin instance. */
		while (argc > 0)
		{
		    argc --;
		    if (argv[argc] != NULL)
		    {
			if (!strcasecmp(argn[argc], "PLUGINSPAGE"))
			    This->pluginsPageUrl = strdup(argv[argc]);
			else if (!strcasecmp(argn[argc], "PLUGINURL"))
			    This->pluginsFileUrl = strdup(argv[argc]);
			else if (!strcasecmp(argn[argc], "CODEBASE"))
			    This->pluginsPageUrl = strdup(argv[argc]);
			else if (!strcasecmp(argn[argc], "CLASSID"))
			    This->pluginsFileUrl = strdup(argv[argc]);
			else if (!strcasecmp(argn[argc], "HIDDEN"))
			    This->pluginsHidden = (!strcasecmp(argv[argc], "TRUE"));
		    }
		}

		if (This->pluginsHidden)
		{
		    showPluginDialog(NULL, (XtPointer)This, NULL);
		}

		return NPERR_NO_ERROR;
	}
	else
		return NPERR_OUT_OF_MEMORY_ERROR;
}


NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
	PluginInstance* This;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;

	if (This != NULL) {
		if (This->type)
			NPN_MemFree(This->type);
		if (This->pluginsPageUrl)
			NPN_MemFree(This->pluginsPageUrl);
		if (This->pluginsFileUrl)
		  NPN_MemFree(This->pluginsFileUrl);
		if (This->refreshing) {
		    if (This->action == GET) {
			XtRemoveCallback (This->button, XmNactivateCallback,
				showPluginDialog, (XtPointer)This);
		    } else if (This->action == REFRESH) {
			XtRemoveCallback (This->button, XmNactivateCallback,
				refreshPluginList, (XtPointer)This);
		    }
		    This->exists = FALSE;
		} else {
			if (This->dialog) {
				XtDestroyWidget(This->dialog);
				This->dialog = NULL;
			}
		}
		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}

	return NPERR_NO_ERROR;
}


NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
	PluginInstance* This;
	NPSetWindowCallbackStruct *ws_info;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	This = (PluginInstance*) instance->pdata;
	ws_info = (NPSetWindowCallbackStruct *)window->ws_info;

	if (This->window == (Window) window->window) {
	   /*
	    * The page with the plugin is being resized.
	    * Save any UI information because the next time
	    * around expect a SetWindow with a new window id.
	    */
#ifdef DEBUG
		fprintf(stderr, "Nullplugin: plugin received window resize.\n");
#endif
		return NPERR_NO_ERROR;
	} else {

	    This->window = (Window) window->window;
	    This->x = window->x;
	    This->y = window->y;
	    This->width = window->width;
	    This->height = window->height;
	    This->display = ws_info->display;
	    This->visual = ws_info->visual;
	    This->depth = ws_info->depth;
	    This->colormap = ws_info->colormap;

	    This->exists = FALSE;
	    This->refreshing = FALSE;
	    makeWidget(This);
	}

	return NPERR_NO_ERROR;
}


NPError 
NPP_NewStream(NPP instance,
	      NPMIMEType type,
	      NPStream *stream, 
	      NPBool seekable,
	      uint16 *stype)
{
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	return NPERR_NO_ERROR;
}


int32 STREAMBUFSIZE = 0X0FFFFFFF; /*
				   * If we are reading from a file in NPAsFile
				   * mode so we can take any size stream in our
				   * write call (since we ignore it)
				   */

int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
   /*
    *	PluginInstance* This;
    *	if (instance != NULL)
    *		This = (PluginInstance*) instance->pdata;
    */
	/* Number of bytes ready to accept in NPP_Write() */
	return STREAMBUFSIZE;
}


int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
   /*
    *	PluginInstance* This;
    *
    *	if (instance != NULL)
    *		This = (PluginInstance*) instance->pdata;
    *	else
    *		return len;
    */
	return len;		/* The number of bytes accepted */
}


NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
   /*
    *	PluginInstance* This;
    */
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
   /*
    *	This = (PluginInstance*) instance->pdata;
    */
	return NPERR_NO_ERROR;
}


void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
   /*
    *	PluginInstance* This;
    *	if (instance != NULL)
    *		This = (PluginInstance*) instance->pdata;
    */
}


void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
	if(printInfo == NULL)
		return;

	if (instance != NULL) {
   /*
    *		PluginInstance* This = (PluginInstance*) instance->pdata;
    */
	
		if (printInfo->mode == NP_FULL) {
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin would like to take over
		     *	printing completely when it is in full-screen mode,
		     *	set printInfo->pluginPrinted to TRUE and print your
		     *	plugin as you see fit.  If your plugin wants Netscape
		     *	to handle printing in this case, set
		     *	printInfo->pluginPrinted to FALSE (the default) and
		     *	do nothing.  If you do want to handle printing
		     *	yourself, printOne is true if the print button
		     *	(as opposed to the print menu) was clicked.
		     *	On the Macintosh, platformPrint is a THPrint; on
		     *	Windows, platformPrint is a structure
		     *	(defined in npapi.h) containing the printer name, port,
		     *	etc.
		     *
		     *
		     *	void* platformPrint =
		     *		printInfo->print.fullPrint.platformPrint;
		     *	NPBool printOne =
		     *		printInfo->print.fullPrint.printOne;
		     */	
			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;
		}
		else {	/* If not fullscreen, we must be embedded */
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin is embedded, or is full-screen
		     *	but you returned false in pluginPrinted above, NPP_Print
		     *	will be called with mode == NP_EMBED.  The NPWindow
		     *	in the printInfo gives the location and dimensions of
		     *	the embedded plugin on the printed page.  On the
		     *	Macintosh, platformPrint is the printer port; on
		     *	Windows, platformPrint is the handle to the printing
		     *	device context.
		     *
		     *
		     *	NPWindow* printWindow =
		     *		&(printInfo->print.embedPrint.window);
		     *	void* platformPrint =
		     *		printInfo->print.embedPrint.platformPrint;
		     */
		}
	}
}
