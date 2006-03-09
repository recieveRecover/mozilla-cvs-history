/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Original Author: David W. Hyatt (hyatt@netscape.com)
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   Dean Tessman <dean_tessman@hotmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsXULAtoms.h"
#include "nsHTMLAtoms.h"
#include "nsPopupSetFrame.h"
#include "nsIMenuParent.h"
#include "nsMenuFrame.h"
#include "nsBoxFrame.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSRendering.h"
#include "nsINameSpaceManager.h"
#include "nsLayoutAtoms.h"
#include "nsMenuPopupFrame.h"
#include "nsMenuBarFrame.h"
#include "nsIView.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMElement.h"
#include "nsISupportsArray.h"
#include "nsIDOMText.h"
#include "nsBoxLayoutState.h"
#include "nsIScrollableFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsGUIEvent.h"
#include "nsIRootBox.h"

#define NS_MENU_POPUP_LIST_INDEX   0

nsPopupFrameList::nsPopupFrameList(nsIContent* aPopupContent, nsPopupFrameList* aNext)
:mNextPopup(aNext), 
 mPopupFrame(nsnull),
 mPopupContent(aPopupContent),
 mElementContent(nsnull), 
 mCreateHandlerSucceeded(PR_FALSE),
 mIsOpen(PR_FALSE),
 mLastPref(-1,-1)
{
}

nsPopupFrameList* nsPopupFrameList::GetEntry(nsIContent* aPopupContent) {
  if (aPopupContent == mPopupContent)
    return this;

  if (mNextPopup)
    return mNextPopup->GetEntry(aPopupContent);

  return nsnull;
}

nsPopupFrameList* nsPopupFrameList::GetEntryByFrame(nsIFrame* aPopupFrame) {
  if (aPopupFrame == mPopupFrame)
    return this;

  if (mNextPopup)
    return mNextPopup->GetEntryByFrame(aPopupFrame);

  return nsnull;
}

//
// NS_NewPopupSetFrame
//
// Wrapper for creating a new menu popup container
//
nsIFrame*
NS_NewPopupSetFrame(nsIPresShell* aPresShell)
{
  return new (aPresShell) nsPopupSetFrame (aPresShell);
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt) 
nsPopupSetFrame::Release(void)
{
    return NS_OK;
}

//
// QueryInterface
//
NS_INTERFACE_MAP_BEGIN(nsPopupSetFrame)
  NS_INTERFACE_MAP_ENTRY(nsIPopupSetFrame)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)


//
// nsPopupSetFrame cntr
//
nsPopupSetFrame::nsPopupSetFrame(nsIPresShell* aShell):nsBoxFrame(aShell)
{

} // cntr

NS_IMETHODIMP
nsPopupSetFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsStyleContext*  aContext,
                      nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsBoxFrame::Init(aContent, aParent, aContext, aPrevInFlow);

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(aParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(this);
  }

  return rv;
}

NS_IMETHODIMP
nsPopupSetFrame::Destroy(nsPresContext* aPresContext)
{
  // Remove our frame list.
  if (mPopupList) {
    // Try to hide any active popups
    if (nsMenuFrame::sDismissalListener) {
      nsIMenuParent *menuParent = nsMenuFrame::sDismissalListener->GetCurrentMenuParent();
      nsIFrame* frame;
      CallQueryInterface(menuParent, &frame);
      // Rollup popups, but only if they're ours
      if (frame && mPopupList->GetEntryByFrame(frame)) {
        nsMenuFrame::sDismissalListener->Rollup();
      }
    }

    // Actually remove each popup from the list as we go. This
    // keeps things consistent so reentering won't crash us
    while (mPopupList) {
      if (mPopupList->mPopupFrame) {
        mPopupList->mPopupFrame->Destroy(aPresContext);
      }

      nsPopupFrameList* temp = mPopupList;
      mPopupList = mPopupList->mNextPopup;
      delete temp;
    }
  }

  nsIRootBox *rootBox;
  nsresult res = CallQueryInterface(mParent->GetParent(), &rootBox);
  NS_ASSERTION(NS_SUCCEEDED(res), "grandparent should be root box");
  if (NS_SUCCEEDED(res)) {
    rootBox->SetPopupSetFrame(nsnull);
  }

  return nsBoxFrame::Destroy(aPresContext);
}

NS_IMETHODIMP
nsPopupSetFrame::DoLayout(nsBoxLayoutState& aState)
{
  // lay us out
  nsresult rv = nsBoxFrame::DoLayout(aState);

  // lay out all of our currently open popups.
  nsPopupFrameList* currEntry = mPopupList;
  while (currEntry) {
    nsIFrame* popupChild = currEntry->mPopupFrame;
    if (popupChild) {
      NS_ASSERTION(popupChild->IsBoxFrame(), "popupChild is not box!!");

      // then get its preferred size
      nsSize prefSize(0,0);
      nsSize minSize(0,0);
      nsSize maxSize(0,0);

      popupChild->GetPrefSize(aState, prefSize);
      popupChild->GetMinSize(aState, minSize);
      popupChild->GetMaxSize(aState, maxSize);

      BoundsCheck(minSize, prefSize, maxSize);

      // if the pref size changed then set bounds to be the pref size
      // and sync the view. Also set new pref size.
     // if (currEntry->mLastPref != prefSize) {
        popupChild->SetBounds(aState, nsRect(0,0,prefSize.width, prefSize.height));
        RepositionPopup(currEntry, aState);
        currEntry->mLastPref = prefSize;
     // }

      // is the new size too small? Make sure we handle scrollbars correctly
      nsIBox* child;
      popupChild->GetChildBox(&child);

      nsRect bounds(popupChild->GetRect());

      nsCOMPtr<nsIScrollableFrame> scrollframe = do_QueryInterface(child);
      if (scrollframe &&
          scrollframe->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
        // if our pref height
        if (bounds.height < prefSize.height) {
          // layout the child
          popupChild->Layout(aState);

          nsMargin scrollbars = scrollframe->GetActualScrollbarSizes();
          if (bounds.width < prefSize.width + scrollbars.left + scrollbars.right)
          {
            bounds.width += scrollbars.left + scrollbars.right;
            //printf("Width=%d\n",width);
            popupChild->SetBounds(aState, bounds);
          }
        }
      }
    
      // layout the child
      popupChild->Layout(aState);

      // only size popup if open
      if (currEntry->mCreateHandlerSucceeded) {
        nsIView* view = popupChild->GetView();
        nsIViewManager* viewManager = view->GetViewManager();
        nsRect r(0, 0, bounds.width, bounds.height);
        viewManager->ResizeView(view, r);
        viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
      }
    }

    currEntry = currEntry->mNextPopup;
  }

  SyncLayout(aState);

  return rv;
}


#ifdef DEBUG_LAYOUT
NS_IMETHODIMP
nsPopupSetFrame::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
  // see if our state matches the given debug state
  PRBool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  PRBool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  // if it doesn't then tell each child below us the new debug state
  if (debugChanged)
  {
    // XXXdwh fix later.  nobody uses this anymore anyway.
  }

  return NS_OK;
}

nsresult
nsPopupSetFrame::SetDebug(nsBoxLayoutState& aState, nsIFrame* aList, PRBool aDebug)
{
      if (!aList)
          return NS_OK;

      while (aList) {
        if (aList->IsBoxFrame())
          aList->SetDebug(aState, aDebug);

        aList = aList->GetNextSibling();
      }

      return NS_OK;
}
#endif


void
nsPopupSetFrame::RepositionPopup(nsPopupFrameList* aEntry, nsBoxLayoutState& aState)
{
  // Sync up the view.
  if (aEntry && aEntry->mElementContent) {
    nsPresContext* presContext = aState.PresContext();
    nsIFrame* frameToSyncTo = presContext->PresShell()->
      GetPrimaryFrameFor(aEntry->mElementContent);
    ((nsMenuPopupFrame*)(aEntry->mPopupFrame))->SyncViewWithFrame(presContext, 
          aEntry->mPopupAnchor, aEntry->mPopupAlign, frameToSyncTo, aEntry->mXPos, aEntry->mYPos);
  }
}

NS_IMETHODIMP
nsPopupSetFrame::ShowPopup(nsIContent* aElementContent, nsIContent* aPopupContent, 
                           PRInt32 aXPos, PRInt32 aYPos, 
                           const nsString& aPopupType, const nsString& anAnchorAlignment,
                           const nsString& aPopupAlignment)
{
  // First fire the popupshowing event.
  if (!OnCreate(aXPos, aYPos, aPopupContent))
    return NS_OK;
        
  // See if we already have an entry in our list.  We must create a new one on a miss.
  nsPopupFrameList* entry = nsnull;
  if (mPopupList)
    entry = mPopupList->GetEntry(aPopupContent);
  if (!entry) {
    entry = new nsPopupFrameList(aPopupContent, mPopupList);
    if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;
    mPopupList = entry;
  }

  // Cache the element content we're supposed to sync to
  entry->mPopupType = aPopupType;
  entry->mElementContent = aElementContent;
  entry->mPopupAlign = aPopupAlignment;
  entry->mPopupAnchor = anAnchorAlignment;
  entry->mXPos = aXPos;
  entry->mYPos = aYPos;

  // If a frame exists already, go ahead and use it.
  entry->mPopupFrame = GetPresContext()->PresShell()
    ->GetPrimaryFrameFor(aPopupContent);

#ifdef DEBUG_PINK
  printf("X Pos: %d\n", mXPos);
  printf("Y Pos: %d\n", mYPos);
#endif

  // Generate the popup.
  entry->mCreateHandlerSucceeded = PR_TRUE;
  entry->mIsOpen = PR_TRUE;
  MarkAsGenerated(aPopupContent);

  // determine if this menu is a context menu and flag it
  nsIFrame* activeChild = entry->mPopupFrame;
  nsIMenuParent* childPopup = nsnull;
  if (activeChild)
    CallQueryInterface(activeChild, &childPopup);
  if ( childPopup && aPopupType.EqualsLiteral("context") )
    childPopup->SetIsContextMenu(PR_TRUE);

  // Now open the popup.
  OpenPopup(entry, PR_TRUE);

  // Now fire the popupshown event.
  OnCreated(aXPos, aYPos, aPopupContent);

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::HidePopup(nsIFrame* aPopup)
{
  if (!mPopupList)
    return NS_OK; // No active popups

  nsPopupFrameList* entry = mPopupList->GetEntryByFrame(aPopup);
  if (!entry)
    return NS_OK;

  if (entry->mCreateHandlerSucceeded)
    ActivatePopup(entry, PR_FALSE);

  if (entry->mElementContent && entry->mPopupType.EqualsLiteral("context")) {
    // If we are a context menu, and if we are attached to a
    // menupopup, then hiding us should also hide the parent menu
    // popup.
    if (entry->mElementContent->Tag() == nsXULAtoms::menupopup) {
      nsIFrame* popupFrame = GetPresContext()->PresShell()
        ->GetPrimaryFrameFor(entry->mElementContent);
      if (popupFrame) {
        nsIMenuParent *menuParent;
        if (NS_SUCCEEDED(CallQueryInterface(popupFrame, &menuParent))) {
          menuParent->HideChain();
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::DestroyPopup(nsIFrame* aPopup, PRBool aDestroyEntireChain)
{
  if (!mPopupList)
    return NS_OK; // No active popups

  nsPopupFrameList* entry = mPopupList->GetEntryByFrame(aPopup);

  if (entry && entry->mCreateHandlerSucceeded) {    // ensure the popup was created before we try to destroy it
    OpenPopup(entry, PR_FALSE);
    entry->mPopupType.SetLength(0);
  
    if (aDestroyEntireChain && entry->mElementContent && entry->mPopupType.EqualsLiteral("context")) {
      // If we are a context menu, and if we are attached to a
      // menupopup, then destroying us should also dismiss the parent
      // menu popup.
      if (entry->mElementContent->Tag() == nsXULAtoms::menupopup) {
        nsIFrame* popupFrame = GetPresContext()->PresShell()
          ->GetPrimaryFrameFor(entry->mElementContent);
        if (popupFrame) {
          nsIMenuParent *menuParent;
          if (NS_SUCCEEDED(CallQueryInterface(popupFrame, &menuParent))) {
            menuParent->DismissChain();
          }
        }
      }
    }

    // clear things out for next time
    entry->mCreateHandlerSucceeded = PR_FALSE;
    entry->mElementContent = nsnull;
    entry->mXPos = entry->mYPos = 0;
    entry->mLastPref.width = -1;
    entry->mLastPref.height = -1;

    // ungenerate the popup.
    entry->mPopupContent->UnsetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, PR_TRUE);
  }

  return NS_OK;
}

void
nsPopupSetFrame::MarkAsGenerated(nsIContent* aPopupContent)
{
  // Set our attribute, but only if we aren't already generated.
  // Retrieve the menugenerated attribute.
  nsAutoString value;
  aPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, 
                         value);
  if (!value.EqualsLiteral("true")) {
    // Generate this element.
    aPopupContent->SetAttr(kNameSpaceID_None, nsXULAtoms::menugenerated, NS_LITERAL_STRING("true"),
                           PR_TRUE);
  }
}

void
nsPopupSetFrame::OpenPopup(nsPopupFrameList* aEntry, PRBool aActivateFlag)
{
  if (aActivateFlag) {
    ActivatePopup(aEntry, PR_TRUE);

    // register the rollup listeners, etc, but not if we're a tooltip
    if (!aEntry->mPopupType.EqualsLiteral("tooltip")) {
      nsIFrame* activeChild = aEntry->mPopupFrame;
      nsIMenuParent* childPopup = nsnull;
      if (activeChild)
        CallQueryInterface(activeChild, &childPopup);

      // Tooltips don't get keyboard navigation
      if (childPopup && !nsMenuFrame::sDismissalListener) {
        // First check and make sure this popup wants keyboard navigation
        nsAutoString property;    
        aEntry->mPopupContent->GetAttr(kNameSpaceID_None, nsXULAtoms::ignorekeys, property);
        if (!property.EqualsLiteral("true"))
          childPopup->InstallKeyboardNavigator();
      }

      UpdateDismissalListener(childPopup);
    }
  }
  else {
    if (aEntry->mCreateHandlerSucceeded && !OnDestroy(aEntry->mPopupContent))
      return;

    // Unregister, but not if we're a tooltip
    if (!aEntry->mPopupType.EqualsLiteral("tooltip") ) {
      if (nsMenuFrame::sDismissalListener)
        nsMenuFrame::sDismissalListener->Unregister();
    }
    
    // Remove any keyboard navigators
    nsIMenuParent* childPopup = nsnull;
    if (aEntry->mPopupFrame)
      CallQueryInterface(aEntry->mPopupFrame, &childPopup);
    if (childPopup)
      childPopup->RemoveKeyboardNavigator();

    ActivatePopup(aEntry, PR_FALSE);

    OnDestroyed(aEntry->mPopupContent);
  }

  nsBoxLayoutState state(GetPresContext());
  MarkDirtyChildren(state); // Mark ourselves dirty.
}

void
nsPopupSetFrame::ActivatePopup(nsPopupFrameList* aEntry, PRBool aActivateFlag)
{
  if (aEntry->mPopupContent) {
    // When we sync the popup view with the frame, we'll show the popup if |menutobedisplayed|
    // is set by setting the |menuactive| attribute. This used to trip css into showing the menu
    // but now we do it ourselves. 
    if (aActivateFlag)
      // XXXben hook in |width| and |height| usage here? 
      aEntry->mPopupContent->SetAttr(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, NS_LITERAL_STRING("true"), PR_TRUE);
    else {
      aEntry->mPopupContent->UnsetAttr(kNameSpaceID_None, nsXULAtoms::menuactive, PR_TRUE);
      aEntry->mPopupContent->UnsetAttr(kNameSpaceID_None, nsXULAtoms::menutobedisplayed, PR_TRUE);

      // get rid of the reflows we just created. If we leave them hanging around, we
      // can get into trouble if a dialog with a modal event loop comes along and
      // processes the reflows before we get to call DestroyChain(). Processing the
      // reflow will cause the popup to show itself again. (bug 71219)
      nsIDocument* doc = aEntry->mPopupContent->GetDocument();
      if (doc)
        doc->FlushPendingNotifications(Flush_OnlyReflow);
         
      // make sure we hide the popup. We can't assume that we'll have a view
      // since we could be cleaning up after someone that didn't correctly 
      // destroy the popup.
      nsIFrame* activeChild = aEntry->mPopupFrame;
      if (activeChild) {
        nsIView* view = activeChild->GetView();
        NS_ASSERTION(view, "View is gone, looks like someone forgot to roll up the popup!");
        if (view) {
          nsIViewManager* viewManager = view->GetViewManager();
          viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
          nsRect r(0, 0, 0, 0);
          viewManager->ResizeView(view, r);
          if (aEntry->mIsOpen) {
            aEntry->mIsOpen = PR_FALSE;
            FireDOMEvent(NS_LITERAL_STRING("DOMMenuInactive"), aEntry->mPopupContent);
          }
        }
      }
    }
  }
}

PRBool
nsPopupSetFrame::OnCreate(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWING, nsnull,
                     nsMouseEvent::eReal);
  // XXX This is messed up: it needs to account for widgets.
  nsPoint dummy;
  event.widget = GetClosestView()->GetNearestWidget(&dummy);
  event.refPoint.x = aX;
  event.refPoint.y = aY;

  if (aPopupContent) {
    nsIPresShell *shell = GetPresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      // shell may no longer be alive, don't use it here unless you keep a ref
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }

    // The menu is going to show, and the create handler has executed.
    // We should now walk all of our menu item children, checking to see if any
    // of them has a command attribute.  If so, then several attributes must
    // potentially be updated.
 
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aPopupContent->GetDocument()));

    PRUint32 count = aPopupContent->GetChildCount();
    for (PRUint32 i = 0; i < count; i++) {
      nsIContent *grandChild = aPopupContent->GetChildAt(i);

      if (grandChild->Tag() == nsXULAtoms::menuitem) {
        // See if we have a command attribute.
        nsAutoString command;
        grandChild->GetAttr(kNameSpaceID_None, nsXULAtoms::command, command);
        if (!command.IsEmpty()) {
          // We do! Look it up in our document
          nsCOMPtr<nsIDOMElement> commandElt;
          domDoc->GetElementById(command, getter_AddRefs(commandElt));
          nsCOMPtr<nsIContent> commandContent(do_QueryInterface(commandElt));
          if ( commandContent ) {
            nsAutoString commandValue;
            // The menu's disabled state needs to be updated to match the command.
            if (commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, commandValue, PR_TRUE);
            else
              grandChild->UnsetAttr(kNameSpaceID_None, nsHTMLAtoms::disabled, PR_TRUE);

            // The menu's label, accesskey and checked states need to be updated
            // to match the command. Note that unlike the disabled state if the
            // command has *no* value, we assume the menu is supplying its own.
            if (commandContent->GetAttr(kNameSpaceID_None, nsXULAtoms::label, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsXULAtoms::label, commandValue, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::accesskey, commandValue, PR_TRUE);

            if (commandContent->GetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, commandValue))
              grandChild->SetAttr(kNameSpaceID_None, nsHTMLAtoms::checked, commandValue, PR_TRUE);
          }
        }
      }
    }
  }

  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnCreated(PRInt32 aX, PRInt32 aY, nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_SHOWN, nsnull,
                     nsMouseEvent::eReal);
  // XXX See OnCreate above
  //event.point.x = aX;
  //event.point.y = aY;

  if (aPopupContent) {
    nsIPresShell *shell = GetPresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      // shell may no longer be alive, don't use it here unless you keep a ref
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }

  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnDestroy(nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDING, nsnull,
                     nsMouseEvent::eReal);

  if (aPopupContent) {
    nsIPresShell *shell = GetPresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      // shell may no longer be alive, don't use it here unless you keep a ref
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRBool
nsPopupSetFrame::OnDestroyed(nsIContent* aPopupContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(PR_TRUE, NS_XUL_POPUP_HIDDEN, nsnull,
                     nsMouseEvent::eReal);

  if (aPopupContent) {
    nsIPresShell *shell = GetPresContext()->GetPresShell();
    if (shell) {
      nsresult rv = shell->HandleDOMEventWithTarget(aPopupContent, &event,
                                                    &status);
      // shell may no longer be alive, don't use it here unless you keep a ref
      if ( NS_FAILED(rv) || status == nsEventStatus_eConsumeNoDefault )
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

void
nsPopupSetFrame::UpdateDismissalListener(nsIMenuParent* aMenuParent)
{
  if (!nsMenuFrame::sDismissalListener) {
    if (!aMenuParent)
       return;
    // Create the listener and attach it to the outermost window.
    aMenuParent->CreateDismissalListener();
  }
  
  // Make sure the menu dismissal listener knows what the current
  // innermost menu popup frame is.
  nsMenuFrame::sDismissalListener->SetCurrentMenuParent(aMenuParent);
}

NS_IMETHODIMP
nsPopupSetFrame::RemovePopupFrame(nsIFrame* aPopup)
{
  // This was called by the Destroy() method of the popup, so all we have to do is
  // get the popup out of our list, so we don't reflow it later.
  nsPopupFrameList* currEntry = mPopupList;
  nsPopupFrameList* temp = nsnull;
  nsPresContext* presContext = GetPresContext();
  while (currEntry) {
    if (currEntry->mPopupFrame == aPopup) {
      // Remove this entry.
      if (temp)
        temp->mNextPopup = currEntry->mNextPopup;
      else
        mPopupList = currEntry->mNextPopup;
      
      // Destroy the frame.
      currEntry->mPopupFrame->Destroy(presContext);

      // Delete the entry.
      currEntry->mNextPopup = nsnull;
      delete currEntry;

      // Break out of the loop.
      break;
    }

    temp = currEntry;
    currEntry = currEntry->mNextPopup;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPopupSetFrame::AddPopupFrame(nsIFrame* aPopup)
{
  // The entry should already exist, but might not (if someone decided to make their
  // popup visible straightaway, e.g., the autocomplete widget).

  // First look for an entry by content.
  nsIContent* content = aPopup->GetContent();
  nsPopupFrameList* entry = nsnull;
  if (mPopupList)
    entry = mPopupList->GetEntry(content);
  if (!entry) {
    entry = new nsPopupFrameList(content, mPopupList);
    if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;
    mPopupList = entry;
  }
  
  // Set the frame connection.
  entry->mPopupFrame = aPopup;
  
  // Now return.  The remaining entry values will be filled in if/when showPopup is
  // called for this popup.
  return NS_OK;
}

