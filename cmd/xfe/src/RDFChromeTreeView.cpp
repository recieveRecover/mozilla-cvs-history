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
/* 
   RDFChromeTreeView.cpp -- view of rdf data
   Created: Stephen Lamm <slamm@netscape.com>, 5-Nov-97.
 */



#include "RDFChromeTreeView.h"
#include "Command.h"
#include "xfe2_extern.h"
#include "xpgetstr.h"

#include <XmL/Tree.h>
#include <Xfe/Button.h>
#include <Xfe/ToolBar.h>
#include <Xm/Label.h>


#define TREE_NAME "RdfTree"
#define MM_PER_INCH      (25.4)
#define POINTS_PER_INCH  (72.0)

extern "C" RDF_NCVocab  gNavCenter;

#ifdef DEBUG_slamm
#define D(x) x
#else
#define D(x)
#endif

#define SECONDS_PER_DAY		86400L

typedef struct _closeRdfViewCBStruct {

  XFE_RDFChromeTreeView *  rdfview;
  XFE_NavCenterView *  ncview;
 } closeRdfViewCBStruct;
extern "C"
{
CL_Compositor *fe_create_compositor (MWContext *context);
XFE_Frame * fe_getFrameFromContext(MWContext* context);
void fe_url_exit (URL_Struct *url, int status, MWContext *context);
void htmlPaneExposeEH(Widget, XtPointer, XEvent *, Boolean *);
void kdebug_printWidgetTree(Widget w, int column);
void fe_set_scrolled_default_size(MWContext *);
void fe_get_final_context_resources(MWContext *);
void fe_find_scrollbar_sizes(MWContext *);

MWContext * fe_CreateNewContext(MWContextType, Widget, fe_colormap * , XP_Bool);
}

//////////////////////////////////////////////////////////////////////////
XFE_RDFChromeTreeView::XFE_RDFChromeTreeView(XFE_Component *	toplevel, 
											 Widget				parent,
											 XFE_View *			parent_view, 
											 MWContext *		context) :
	XFE_RDFTreeView(toplevel, parent, parent_view, context),
	_viewLabel(NULL),
	_controlToolBar(NULL),
	_addBookmarkControl(NULL),
	_closeControl(NULL),
	_modeControl(NULL),
	_htmlPaneForm(NULL),
	_divisionForm(NULL),
	_htmlPane(NULL),
	_htmlPaneHeightPercent(0),
	_htmlPaneHeightFixed(0),
	_htmlPaneSizing(XFE_HTML_PANE_PERCENT)
{
	createViewLabel();

	createDivisionForm();

	createTree();

    createControlToolbar();

    doAttachments();
}
//////////////////////////////////////////////////////////////////////////
XFE_RDFChromeTreeView::~XFE_RDFChromeTreeView()
{
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::createControlToolbar()
{
	XP_ASSERT( _controlToolBar == NULL );
	XP_ASSERT( XfeIsAlive(getBaseWidget()) );

	// Control Tool Bar
	_controlToolBar = 
		XtVaCreateManagedWidget("controlToolBar",
								xfeToolBarWidgetClass,
								getBaseWidget(),
								XmNorientation,				XmHORIZONTAL,
								XmNusePreferredWidth,		True,
								XmNusePreferredHeight,		True,
								XmNchildForceWidth,			False,
								XmNchildForceHeight,		True,
								XmNchildUsePreferredWidth,	True,
								XmNchildUsePreferredHeight,	False,
								NULL);

	// Add Bookmark
	_addBookmarkControl = XtVaCreateManagedWidget("addBookmark",
												  xfeButtonWidgetClass,
												  _controlToolBar,
												  NULL);
	// Close
	_closeControl = XtVaCreateManagedWidget("close",
											xfeButtonWidgetClass,
											_controlToolBar,
											NULL);
#if 0
	// Mode
	_modeControl = XtVaCreateManagedWidget("mode",
										   xfeButtonWidgetClass,
										   _controlToolBar,
										   NULL);
#endif

    /*
	closeRdfViewCBStruct *  cb_str = new closeRdfViewCBStruct;
	
	cb_str->rdfview = this;
	cb_str->ncview = (XFE_NavCenterView *)parent_view;
	*/
	XtAddCallback(_closeControl, XmNactivateCallback, (XtCallbackProc)closeRdfView_cb , (void *)this);

	
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::createViewLabel()
{
	XP_ASSERT( XfeIsAlive(getBaseWidget()) );
	XP_ASSERT( _viewLabel == NULL );

	// View label
	_viewLabel = 
		XtVaCreateManagedWidget("viewLabel",
								xmLabelWidgetClass,
								getBaseWidget(),
								XmNalignment,			XmALIGNMENT_BEGINNING,
								NULL);
                                     
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::createDivisionForm()
{
	XP_ASSERT( XfeIsAlive(getBaseWidget()) );
	XP_ASSERT( _divisionForm == NULL );
    //	XP_ASSERT( XfeIsAlive(_viewLabel) );

	// Division form
	_divisionForm = 
		XtVaCreateManagedWidget("divisionForm",
								xmFormWidgetClass,
								getBaseWidget(),
								XmNtopAttachment,		XmATTACH_WIDGET,
								XmNtopWidget,			_viewLabel,
								XmNrightAttachment,		XmATTACH_FORM,
								XmNleftAttachment,		XmATTACH_FORM,
								XmNbottomAttachment,	XmATTACH_FORM,
								NULL);
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::createHtmlPane()
{
	XP_ASSERT( XfeIsAlive(_divisionForm) );
	XP_ASSERT( _htmlPaneForm == NULL );
	XP_ASSERT( _htmlPane == NULL );
    MWContext *     context = NULL;
    XFE_Frame *  frame = fe_getFrameFromContext(getContext());
    char * url = getenv("HTMLPANEURL");



	_htmlPaneForm = XtVaCreateWidget("htmlPaneForm",
									 xmFormWidgetClass,
									 _divisionForm,
									 XmNshadowThickness,		0,
									 XmNbackground,				0,
									 NULL);

    context = fe_CreateNewContext(MWContextPane, _htmlPaneForm, 
                                  CONTEXT_DATA(getContext())->colormap,
                                  TRUE);

    ViewGlue_addMapping(frame, context);

    /* For some reason these 2 functions are not called in the context 
     * creation call above
     */
    fe_init_image_callbacks(context);
    fe_InitColormap(context);


	_htmlPane = new XFE_HTMLView(this,
								 _htmlPaneForm,
								 this,
								 context);

	addView(_htmlPane);

    XtVaSetValues(_htmlPane->getBaseWidget(),
                  XmNtopAttachment,			XmATTACH_FORM,
                  XmNrightAttachment,		XmATTACH_FORM,
                  XmNleftAttachment,		XmATTACH_FORM,
                  XmNbottomAttachment,		XmATTACH_FORM,
                  NULL);
	
	_htmlPane->show();
    

    XtAddEventHandler((_htmlPane->getBaseWidget()), StructureNotifyMask, True, 
                      (XtEventHandler) htmlPaneExposeEH, (XtPointer) _htmlPane);


}

//////////////////////////////////////////////////////////////////////////
/* virtual */ void
XFE_RDFChromeTreeView::doAttachments()
{
	// Control toolbar on top
    XtVaSetValues(_controlToolBar,
                  XmNtopAttachment,			XmATTACH_FORM,
                  XmNrightAttachment,		XmATTACH_FORM,
                  XmNleftAttachment,		XmATTACH_NONE,
                  XmNbottomAttachment,		XmATTACH_NONE,
                  NULL);

	// View label
    XtVaSetValues(_viewLabel,
                  XmNtopAttachment,		XmATTACH_WIDGET,
                  XmNtopWidget,			_controlToolBar,
                  XmNrightAttachment,	XmATTACH_FORM,
                  XmNleftAttachment,	XmATTACH_FORM,
                  XmNbottomAttachment,	XmATTACH_NONE,
                  NULL);

	// Division form
    XtVaSetValues(_divisionForm,
				  XmNtopAttachment,		XmATTACH_WIDGET,
				  XmNtopWidget,			_viewLabel,
				  XmNrightAttachment,	XmATTACH_FORM,
				  XmNleftAttachment,	XmATTACH_FORM,
				  XmNbottomAttachment,	XmATTACH_FORM,
				  NULL);

	// Html sizing as a percentage
	if (_htmlPaneSizing == XFE_HTML_PANE_PERCENT)
	{
		// Html pane at 0%
		if (_htmlPaneHeightPercent == 0)
		{
			XtVaSetValues(_tree,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			if (XfeIsAlive(_htmlPaneForm))
			{
				XtUnmanageChild(_htmlPaneForm);
			}
		}
		// Html pane at 100%
		else if (_htmlPaneHeightPercent == 100)
		{
			XtVaSetValues(_htmlPaneForm,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			if (XfeIsAlive(_tree))
			{
				XtUnmanageChild(_tree);
			}
		}
		// Html pane between 0% and 100%
		else
		{
			XP_ASSERT( XfeIsAlive(_htmlPaneForm) );
			XP_ASSERT( XfeIsAlive(_tree) );
			
			XtVaSetValues(_htmlPaneForm,
						  XmNtopAttachment,		XmATTACH_POSITION,
//						XmNtopPositition,		_htmlPaneHeightPercent,
						  XmNtopPosition,		(100 - _htmlPaneHeightPercent),
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			XtVaSetValues(_tree,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_WIDGET,
						  XmNbottomWidget,		_htmlPaneForm,
						  NULL);
			
			XtManageChild(_htmlPaneForm);
            //            XtRealizeWidget(_htmlPaneForm);
			XtManageChild(_tree);
		}
	}
	// Html sizing as a fixed height
	else
	{
		Dimension division_height = XfeHeight(_divisionForm);

		// Html pane at 0 height
		if (_htmlPaneHeightFixed == 0)
		{
			XtVaSetValues(_tree,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			if (XfeIsAlive(_htmlPaneForm))
			{
				XtUnmanageChild(_htmlPaneForm);
			}
		}
		// Html pane at >= division_height
		else if (_htmlPaneHeightFixed >= division_height)
		{
			XtVaSetValues(_htmlPaneForm,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			if (XfeIsAlive(_tree))
			{
				XtUnmanageChild(_tree);
			}
		}
		// Html pane between 0 and division_height
		else
		{
			XP_ASSERT( XfeIsAlive(_htmlPaneForm) );
			XP_ASSERT( XfeIsAlive(_tree) );
			
			XtVaSetValues(_htmlPaneForm,
						  XmNheight,			_htmlPaneHeightFixed,
						  NULL);

			XtVaSetValues(_htmlPaneForm,
						  XmNtopAttachment,		XmATTACH_NONE,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_FORM,
						  NULL);
			
			XtVaSetValues(_tree,
						  XmNtopAttachment,		XmATTACH_FORM,
						  XmNrightAttachment,	XmATTACH_FORM,
						  XmNleftAttachment,	XmATTACH_FORM,
						  XmNbottomAttachment,	XmATTACH_WIDGET,
						  XmNbottomWidget,		_htmlPaneForm,
						  NULL);
			
			XtManageChild(_htmlPaneForm);
            //            XtRealizeWidget(_htmlPaneForm);
			XtManageChild(_tree);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::updateRoot() 
{
    char * label = HT_GetViewName(_ht_view);
    
    // XXX  Aurora NEED TO LOCALIZE  XXX
    XmString xmstr = XmStringCreateLocalized(label);
    
    XtVaSetValues(_viewLabel,XmNlabelString,xmstr,NULL);
        
    XmStringFree(xmstr);
        
    // Set the HT properties
    setHTTitlebarProperties(_ht_view, _viewLabel);

    XFE_RDFTreeView::updateRoot();
}
//////////////////////////////////////////////////////////////////////////
Widget
XFE_RDFChromeTreeView::getTreeParent() 
{
	XP_ASSERT( XfeIsAlive(_divisionForm) );

	return _divisionForm;
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::notify(HT_Resource		n, 
							  HT_Event			whatHappened)
{
  D(debugEvent(n, whatHappened,"RV"););

	// HT_EVENT_VIEW_SELECTED
	if (whatHappened == HT_EVENT_VIEW_SELECTED)
	{
        setHTView(HT_GetView(n));
	}
    XFE_RDFTreeView::notify(n, whatHappened);
}
//////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::closeRdfView_cb(Widget /* w */, XtPointer clientData, XtPointer /* callData */)
{


  XFE_RDFChromeTreeView * obj = (XFE_RDFChromeTreeView *) clientData;

  Widget parent = XtParent(obj->getBaseWidget());
  XtUnmanageChild(parent);



#ifdef MOZ_SELECTOR_BAR

#if 0
  closeRdfViewCBStruct * obj = (closeRdfViewCBStruct *) clientData; 
  XFE_NavCenterView * ncview = obj->ncview;
//  Widget nc_base_widget = ncview->getBaseWidget();
  Widget  selector  = (Widget )ncview->getSelector();
/*   XtVaSetValues(nc_base_widget, XmNresizable, True, NULL); */
  XtUnmanageChild(parent);

  XtUnmanageChild(selector);  
  XtVaSetValues(selector, XmNrightAttachment, XmATTACH_FORM, 
                          XmNleftAttachment, XmATTACH_FORM,
                          XmNtopAttachment, XmATTACH_FORM,
                          XmNbottomAttachment, XmATTACH_FORM,
                          NULL);
  XtManageChild(selector);
#endif

#endif /*MOZ_SELECTOR_BAR*/
}

//////////////////////////////////////////////////////////////////////////
//
// XFE_RDFChromeTreeView public methods.
//
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::setHTTitlebarProperties(HT_View view, Widget  titleBar)
{

   Arg           av[30];
   Cardinal      ac=0;
   void *        data=NULL;
   Pixel         pixel;
   PRBool        gotit=False;

   ////////////////  TITLEBAR PROPERTIES   ///////////////////

   ac = 0;
   /* titleBarFGColor */
   HT_GetTemplateData(HT_TopNode(view),  gNavCenter->titleBarFGColor, HT_COLUMN_STRING, &data);
   if (data)
   {
      gotit = fe_GetPixelFromRGBString(getContext(), (char *) data, &pixel);
      if (gotit)
         XtSetArg(av[ac], XmNforeground, pixel); ac++;
   }

   /* titleBarBGColor */
   HT_GetTemplateData(HT_TopNode(view),  gNavCenter->titleBarBGColor, HT_COLUMN_STRING, &data);
   if (data)
   {
      gotit = fe_GetPixelFromRGBString(getContext(), (char *) data, &pixel);
      if (gotit)
         XtSetArg(av[ac], XmNbackground, pixel); ac++;
   }

   /* titleBarBGURL */
   HT_GetTemplateData(HT_TopNode(view),  gNavCenter->titleBarBGURL, HT_COLUMN_STRING, &data);
    if (data)
    {
       /* Do the RDFImage thing */

    }

    XtSetValues(titleBar, av, ac);
}
//////////////////////////////////////////////////////////////////////////
//
// Set the HTML pane height (as a percentage of the view)
//
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::setHtmlPaneHeightPercent(PRUint32 heightPercent)
{
	// Make sure it actually changed
	if (_htmlPaneHeightPercent == heightPercent)
	{
		return;
	}

	// Make sure its not more than 100%
	if (_htmlPaneHeightPercent > 100)
	{
		_htmlPaneHeightPercent = 100;
	}

	// Create the html pane if needed (for the first time)
	if ((heightPercent > 0) && (_htmlPane == NULL))
	{
		createHtmlPane();
	}

	_htmlPaneHeightPercent = heightPercent;

	XP_ASSERT( _htmlPaneForm != NULL );
//	XP_ASSERT( _htmlPane != NULL );

	// Show the html pane form if needed
	XfeSetManagedState(_htmlPaneForm,_htmlPaneHeightPercent != 0);

	// Show the tree if needed
	XfeSetManagedState(_tree,_htmlPaneHeightPercent != 100);
	
	// Redo attachments
    doAttachments();
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::setHtmlPaneHeightFixed(PRUint32 heightFixed)
{
	// Make sure it actually changed
	if (_htmlPaneHeightFixed == heightFixed)
	{
		return;
	}

	// Create the html pane if needed (for the first time)
	if ((heightFixed > 0) && (_htmlPane == NULL))
	{
		createHtmlPane();
	}

	_htmlPaneHeightFixed = heightFixed;

	XP_ASSERT( _htmlPaneForm != NULL );
//	XP_ASSERT( _htmlPane != NULL );

	// Show the html pane form if needed
	XfeSetManagedState(_htmlPaneForm,_htmlPaneHeightFixed > 0);

	// Show the tree if needed
	XfeSetManagedState(_tree,_htmlPaneHeightFixed < XfeHeight(_divisionForm));
	
	// Redo attachments
    doAttachments();
}
//////////////////////////////////////////////////////////////////////////
void
XFE_RDFChromeTreeView::setHtmlPaneSizing(EHtmlPaneSizing sizing)
{
	_htmlPaneSizing = sizing;

	// Redo attachments
    doAttachments();
}
//////////////////////////////////////////////////////////////////////////
extern "C"
void 
htmlPaneExposeEH(Widget w, XtPointer clientData, XEvent * event, Boolean* continue_to_dispatch)
{

    XFE_HTMLView *  htmlview = (XFE_HTMLView *)clientData;
    MWContext *  context = htmlview->getContext();
    char * url = getenv("HTMLPANEURL");
    if (!url)
      url = "http://people.netscape.com/radha/sony/images/tweetee.gif";

    if (event && (event->type == MapNotify))
	{
		// We only need this event handler to be called once
		XtRemoveEventHandler(w,StructureNotifyMask,True,
							 htmlPaneExposeEH, clientData);

        if (!XtIsRealized(w))
           XtRealizeWidget(w);

        // Create the compositor
        context->compositor = fe_create_compositor(context);
        XtVaSetValues (CONTEXT_DATA (context)->scrolled, XmNinitialFocus,
                 CONTEXT_DATA (context)->drawing_area, 0);

        // Do the required scroller voodoo
        fe_set_scrolled_default_size(context);
        fe_get_final_context_resources(context);
        fe_find_scrollbar_sizes(context);
        fe_InitScrolling(context); 
        
        // Load the URL
        if (url)
          NET_GetURL(NET_CreateURLStruct(url, NET_DONT_RELOAD),
               FO_CACHE_AND_PRESENT, context,
               fe_url_exit);

    }

}  /* htmlPaneExposeEH  */

