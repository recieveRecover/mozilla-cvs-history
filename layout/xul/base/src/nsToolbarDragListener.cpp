/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1998
 * Netscape Communications Corporation.  All Rights Reserved.
 */

#include "nsToolbarDragListener.h"
#include "nsToolbarFrame.h"

#include "nsCOMPtr.h"
#include "nsIDOMUIEvent.h"
#include "nsIPresContext.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsXULAtoms.h"
#include "nsIEventStateManager.h"
#include "nsISupportsPrimitives.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"


NS_IMPL_ADDREF(nsToolbarDragListener)
NS_IMPL_RELEASE(nsToolbarDragListener)


//
// nsToolbarDragListener ctor
//
// Init member variables. We can't really do much of anything important here because
// any subframes might not be totally intialized yet, or in the hash table
//
nsToolbarDragListener :: nsToolbarDragListener ( nsToolbarFrame* inToolbar, nsIPresContext* inPresContext )
  : mToolbar(inToolbar), mPresContext(inPresContext), mCurrentDropLoc(-1)
{
  NS_INIT_REFCNT();
  
  // we really need this all over the place. just be safe that we have it.
  NS_ASSERTION ( mPresContext, "no pres context set on toolbar drag listener" );
 
} // nsToolbarDragListener ctor


//
// nsToolbarDragListener dtor
//
// Cleanup.
//
nsToolbarDragListener::~nsToolbarDragListener() 
{
}


//
// QueryInterface
//
// Modeled after scc's reference implementation
//   http://www.mozilla.org/projects/xpcom/QI.html
//
nsresult
nsToolbarDragListener::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if ( !aInstancePtr)
    return NS_ERROR_NULL_POINTER;

  if (aIID.Equals(nsCOMTypeInfo<nsIDOMEventListener>::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIDOMEventListener*, NS_STATIC_CAST(nsIDOMDragListener*, this));
  else if (aIID.Equals(nsCOMTypeInfo<nsIDOMDragListener>::GetIID()))
    *aInstancePtr = NS_STATIC_CAST(nsIDOMDragListener*, this);
  else if (aIID.Equals(nsCOMTypeInfo<nsISupports>::GetIID()))                                   
    *aInstancePtr = NS_STATIC_CAST(nsISupports*, NS_STATIC_CAST(nsIDOMDragListener*, this));
  else
    *aInstancePtr = 0;
  
  nsresult status;
  if ( !*aInstancePtr )
    status = NS_NOINTERFACE;
  else {
    NS_ADDREF( NS_REINTERPRET_CAST(nsISupports*, *aInstancePtr) );
    status = NS_OK;
  }
  return status;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::HandleEvent(nsIDOMEvent* aEvent)
{
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragGesture(nsIDOMEvent* aDragEvent)
{
  // this code should all be in JS.
  return NS_OK;
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragEnter(nsIDOMEvent* aDragEvent)
{
  // We don't need to do anything special here. If anything does need to be done, 
  // the code should all be in JS.
  return NS_OK;
}


//
// ItemMouseIsOver
//
// Figure out which child item mouse is over. |outIndex| is the index of the item the object 
// should be dropped _before_. Therefore if the item should be dropped at the end, the index 
// will be greater than the number of items in the list. |outOnChild| is true if the item
// is a container and the drop would be "on" that item.
// 
void
nsToolbarDragListener :: ItemMouseIsOver ( nsIDOMEvent* aDragEvent, nscoord* outXLoc, 
                                             PRUint32* outIndex, PRBool* outOnChild )
{
  *outOnChild = PR_FALSE;

  // figure out which frame is the right one for determining the drop feedback. Do we
  // want to do this every time? We certainly can't do it upon toolbar creation (hash
  // tables not setup at that time), so we're forced to do it now. What happens if while
  // we're dragging some rule gets fired that causes the frame to go away? 
  nsIFrame* dropAreaFrame = LocateDropAreaFrame();
  if ( !dropAreaFrame ) {
    // gaaak, we're doomed.
    outIndex = 0;
    outXLoc = 0;
    return;
  }
  
  //
  // Get the mouse coordinates from the DOM event, but they will be in the 
  // window/widget coordinate system. We must first get them into the frame-relative
  // coordinate system. Yuck.
  //
  
  // get mouse coordinates and translate them into twips
  nsCOMPtr<nsIDOMUIEvent> uiEvent(do_QueryInterface(aDragEvent));
  PRInt32 x,y = 0;
  uiEvent->GetClientX(&x);
  uiEvent->GetClientY(&y);
  float p2t;
  mPresContext->GetScaledPixelsToTwips(&p2t);
  nscoord onePixel = NSIntPixelsToTwips(1, p2t);
  nscoord xp       = NSIntPixelsToTwips(x, p2t);
  nscoord yp       = NSIntPixelsToTwips(y, p2t);
  
  // compute the offset to top level in twips
  PRInt32 frameOffsetX = 0, frameOffsetY = 0;
  nsIFrame* curr = dropAreaFrame;
  curr->GetParent(&curr);
  float t2p;
  mPresContext->GetTwipsToPixels(&t2p);
  while ( curr ) {
    nsPoint origin;
    curr->GetOrigin(origin);      // in twips    
    frameOffsetX += origin.x;     // build the offset incrementally
    frameOffsetY += origin.y;    
    curr->GetParent(&curr);       // moving up the chain
  } // until we reach the top  

  // subtract the offset from the x,y coord to put into frame relative coordinates.
  xp -= frameOffsetX;
  yp -= frameOffsetY;
  nsPoint pnt(xp, yp);
  
  // get the toolbar's rect
  nsRect tbRect;
  dropAreaFrame->GetRect(tbRect);

  PRUint32 count = 0;
  PRBool found = PR_FALSE;
  nsIFrame* childFrame;
  nsRect    rect;             // child frame's rect
  nsRect    prevRect(-1, -1, 0, 0);

  //
  // Now loop through the child and see if the mouse is over a child
  //
  
  dropAreaFrame->FirstChild(nsnull, &childFrame); 
  while ( childFrame ) {    

    // The mouse coords are in the toolbar's domain
    // Get child's rect and adjust to the toolbar's domain
    childFrame->GetRect(rect);
    rect.MoveBy(tbRect.x, tbRect.y);

    // remember the previous child x location
    if (pnt.x < rect.x && prevRect.x == -1)
      prevRect = rect;

    // now check to see if the mouse inside an items bounds
    if (rect.Contains(pnt)) {
      nsCOMPtr<nsIContent> content;
      childFrame->GetContent(getter_AddRefs(content));
      if ( content ) {
        nsCOMPtr<nsIAtom> tag;
        content->GetTag(*getter_AddRefs(tag));

        // for now I am checking for both titlebutton and toolbar items
        // XXX but the check for titlebutton should be removed in the future
        if (tag.get() == nsXULAtoms::titledbutton || tag.get() == nsXULAtoms::toolbaritem) {

          // now check if item is a container
          PRBool isContainer = PR_FALSE;
          nsCOMPtr<nsIDOMElement> domElement ( do_QueryInterface(content) );
          if ( domElement ) {
            nsAutoString value;
            domElement->GetAttribute(nsAutoString("container"), value);  // can't use an atom here =(
            isContainer = value.Equals("true");
          }
          else
            NS_WARNING("Not a DOM element");        

          // if we have a container, the area is broken up into 3 pieces (left, middle, right). If
          // it isn't it's only broken up into two (left and right)
          PRInt32 xc = -1;
          if ( isContainer ) {
            if (pnt.x <= (rect.x + (rect.width / 4))) {
              *outIndex = count;
              xc = rect.x - tbRect.x;
            } 
            else if (pnt.x >= (rect.x + PRInt32(float(rect.width) *0.75))) {
              *outIndex = count + 1;
              xc = rect.x - tbRect.x + rect.width - onePixel;
            } 
            else {
              // we're on a container, don't draw anything so xc shouldn't get set.
              *outIndex = count;
              *outOnChild = PR_TRUE;
            }
          } else {
            if (pnt.x <= (rect.x + (rect.width / 2))) {
              *outIndex = count;
              xc = rect.x - tbRect.x;
            }
            else {
              *outIndex = count + 1;
              xc = rect.x - tbRect.x + rect.width + onePixel;
            }
          }
          
          *outXLoc = xc;
        }
        else {
          // mouse is over something (probably a spacer) so return the left side of
          // the spacer.
          *outXLoc = rect.x - tbRect.x;
          *outIndex = count;
        }
        
        // found something, break out of the loop
        found = PR_TRUE;
        break;
      }
    } // if mouse is in an item

    nsresult rv = childFrame->GetNextSibling(&childFrame);
    NS_ASSERTION(rv == NS_OK,"failed to get next child");
    count++;
  } // foreach child

  if (!found) {
    *outIndex = count;  // already incremented past last item
    if ( count )
      *outXLoc = prevRect.x - tbRect.x + rect.width + onePixel;
    else
      *outXLoc = onePixel;
  }
}


//
// DragOver
//
// The mouse has moved over the toolbar while a drag is happening. We really just want to
// "annotate" the toolbar with the current drop location. We don't want to make any judgement
// as this stage as to whether or not the drag should be accepted or draw any feedback. 
//
nsresult
nsToolbarDragListener::DragOver(nsIDOMEvent* aDragEvent)
{

#if 0
nsCOMPtr<nsIContent> c;
mToolbar->GetContent ( getter_AddRefs(c) );
nsCOMPtr<nsIDOMNode> d ( do_QueryInterface(c) );
nsCOMPtr<nsIDOMNode> t;
aDragEvent->GetTarget ( getter_AddRefs(t) );
printf ( "DRAGOVER:: toolbar content is %ld, as DOMNode %ld, target is %ld\n", c, d, t );
#endif

  // Check to see if the mouse is over an item and which one it is.
  nscoord xLoc = 0;
  PRBool onChild;
  PRUint32 beforeIndex = 0;
  ItemMouseIsOver(aDragEvent, &xLoc, &beforeIndex, &onChild);
  if ( xLoc != mCurrentDropLoc ) {
  
    // stash the new location in the toolbar's content model. Note that the toolbar code doesn't
    // care at all about "tb-droplocation", only the coordinate so there is no need to send the
    // AttributeChanged() about that attribute.
    nsCOMPtr<nsIContent> content;
    mToolbar->GetContent ( getter_AddRefs(content) );
    if ( content ) {
      char buffer[10];

      	// need the cast, because on some platforms, PR[U]int32 != long, but we're using "%ld"
      sprintf(buffer, "%ld", NS_STATIC_CAST(long, xLoc));
      content->SetAttribute ( kNameSpaceID_None, nsXULAtoms::tbDropLocationCoord, buffer, PR_TRUE );
      sprintf(buffer, "%ld", NS_STATIC_CAST(long, beforeIndex));
      content->SetAttribute ( kNameSpaceID_None, nsXULAtoms::tbDropLocation, "1", PR_FALSE );
      content->SetAttribute ( kNameSpaceID_None, nsXULAtoms::tbDropOn, onChild ? "true" : "false", PR_FALSE );
    }
    
    // cache the current drop location
    mCurrentDropLoc = xLoc;
  }
  
  // NS_OK means event is NOT consumed. We want to make sure JS gets this so it
  // can determine if the drag is allowed.
  return NS_OK; 
}


////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragExit(nsIDOMEvent* aDragEvent)
{
// there are some bugs that cause us to not be able to correctly track dragExit events
// so until then we just get on our knees and pray we don't get fooled again.
#if 0
nsCOMPtr<nsIContent> c;
mToolbar->GetContent ( getter_AddRefs(c) );
nsCOMPtr<nsIDOMNode> d ( do_QueryInterface(c) );
nsCOMPtr<nsIDOMNode> t;
aDragEvent->GetTarget ( getter_AddRefs(t) );
printf ( "DRAGEXIT:: toolbar DOMNode %ld, target is %ld\n", d, t );

  nsCOMPtr<nsIContent> content;
  mToolbar->GetContent ( getter_AddRefs(content) );

  // we will get a drag exit event on sub items because we catch the event on the way down. If
  // the target is not our toolbar, then ignore it.
  nsCOMPtr<nsIDOMNode> toolbarDOMNode ( do_QueryInterface(content) );
  nsCOMPtr<nsIDOMNode> eventTarget;
  aDragEvent->GetTarget ( getter_AddRefs(eventTarget) );
  if ( eventTarget != toolbarDOMNode )
    return NS_OK;

printf("***REAL EXIT EVENT\n");

  // tell the toolbar to not do any more drop feedback. Note that the toolbar code doesn't
  // care at all about "tb-droplocation", only the coordinate so there is no need to send the
  // AttributeChanged() about that attribute.
  char buffer[10];
  sprintf(buffer, "%ld", -1);
  content->SetAttribute ( kNameSpaceID_None, nsXULAtoms::tbDropLocationCoord, buffer, PR_TRUE );
  content->SetAttribute ( kNameSpaceID_None, nsXULAtoms::tbDropLocation, buffer, PR_FALSE );
    
  // cache the current drop location
  mCurrentDropLoc = -1;
#endif

  return NS_OK; // don't consume event
}



////////////////////////////////////////////////////////////////////////
nsresult
nsToolbarDragListener::DragDrop(nsIDOMEvent* aMouseEvent)
{
  // this code should all be in JS.
  return NS_OK;
}


//
// LocateDropAreaFrame
//
// Returns the frame (or subframe) that contains the buttons that can be dragged.
// Either it will be the toolbar frame as a whole, or it will be some subframe of the bar id'd by
// the |dragdroparea| attribute.
//
nsIFrame*
nsToolbarDragListener :: LocateDropAreaFrame ( )
{
  nsIFrame* retVal = nsnull;
  
  // is a subframe the drag/drop area? determine if the attribute is set.
  nsString dropAreaID;
  PRBool dropAreaIsSubframe = PR_FALSE;
  nsCOMPtr<nsIContent> toolbarContent;
  mToolbar->GetContent ( getter_AddRefs(toolbarContent) );
  if ( toolbarContent ) {
    if ( toolbarContent->GetAttribute(kNameSpaceID_None, nsXULAtoms::tbDragDropArea, dropAreaID) == NS_CONTENT_ATTR_HAS_VALUE )
      dropAreaIsSubframe = PR_TRUE;
  }

  // if there is a named subframe, go find it, otherwise use the entire toolbar
  if ( dropAreaIsSubframe ) {
  
    // get the presShell so we can call GetPrimaryFrameFor later.
    nsCOMPtr<nsIPresShell> presShell;
    mPresContext->GetShell ( getter_AddRefs(presShell) );
    
    // get the document so we can get do a GetElementByID.
    nsCOMPtr<nsIDocument> document;
    toolbarContent->GetDocument ( *getter_AddRefs(document) );
    if ( document ) {
      nsCOMPtr<nsIDOMXULDocument> xulDoc ( do_QueryInterface(document) );
      if ( xulDoc ) {
        nsCOMPtr<nsIDOMElement> domElementOfSubframe;
        xulDoc->GetElementById ( dropAreaID, getter_AddRefs(domElementOfSubframe) );

        // finally get the frame associated with that dom node
        nsCOMPtr<nsIContent> contentOfSubframe ( do_QueryInterface(domElementOfSubframe) );
        if ( contentOfSubframe && presShell )
          presShell->GetPrimaryFrameFor ( contentOfSubframe, &retVal );       
      }
    }
    
  } // if named subframe
  else
    retVal = mToolbar;
    
  NS_ASSERTION ( retVal, "toolbar drag listener couldn't figure out the drag area." );
  return retVal;
  
} // LocateDropAreaFrame



