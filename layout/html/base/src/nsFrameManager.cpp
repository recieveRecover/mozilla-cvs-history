/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
#include "nsIFrameManager.h"
#include "nsIFrame.h"
#include "nsIPresContext.h"
#include "nsIPresShell.h"
#include "nsIStyleSet.h"
#include "nsIStyleContext.h"
#include "nsStyleChangeList.h"
#include "nsIEventQueueService.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "prthread.h"
#include "plhash.h"
#include "nsDST.h"
#include "nsPlaceholderFrame.h"
#include "nsLayoutAtoms.h"
#ifdef NS_DEBUG
#include "nsISupportsArray.h"
#include "nsIStyleRule.h"
#endif
#include "nsILayoutHistoryState.h"
#include "nsIStatefulFrame.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"

// Class IID's
static NS_DEFINE_IID(kEventQueueServiceCID, NS_EVENTQUEUESERVICE_CID);

// IID's
static NS_DEFINE_IID(kIEventQueueServiceIID,  NS_IEVENTQUEUESERVICE_IID);

//----------------------------------------------------------------------

// Thin veneer around an NSPR hash table. Provides a map from a key that's
// a pointer to a value that's also a pointer.
class FrameHashTable {
public:
  FrameHashTable(PRUint32 aNumBuckets = 16);
  ~FrameHashTable();

  // Gets the value associated with the specified key
  void* Get(void* aKey);

  // Creates an association between the key and value. Returns the previous
  // value associated with the key, or NULL if there was none
  void* Put(void* aKey, void* aValue);

  // Removes association for the key, and returns its associated value
  void* Remove(void* aKey);

  // Removes all entries from the hash table
  void  Clear();

#ifdef NS_DEBUG
  void  Dump(FILE* fp);
#endif

protected:
  PLHashTable* mTable;
};

MOZ_DECL_CTOR_COUNTER(UndisplayedNode);

struct UndisplayedNode {
  UndisplayedNode(nsIContent* aContent, nsIStyleContext* aStyle)
  {
    MOZ_COUNT_CTOR(UndisplayedNode);
    mContent = aContent;
    mStyle = aStyle;
    NS_ADDREF(mStyle);
    mNext = nsnull;
  }
  UndisplayedNode(nsIStyleContext* aPseudoStyle) 
  {
    MOZ_COUNT_CTOR(UndisplayedNode);
    mContent = nsnull;
    mStyle = aPseudoStyle;
    NS_ADDREF(mStyle);
    mNext = nsnull;
  }
  ~UndisplayedNode(void)
  {
    MOZ_COUNT_DTOR(UndisplayedNode);
    NS_RELEASE(mStyle);
    if (mNext) {
      delete mNext;
    }
  }

  nsIContent*       mContent;
  nsIStyleContext*  mStyle;
  UndisplayedNode*  mNext;
};

class UndisplayedMap {
public:
  UndisplayedMap(PRUint32 aNumBuckets = 16);
  ~UndisplayedMap(void);

  UndisplayedNode* GetFirstNode(nsIContent* aParentContent);

  nsresult AddNodeFor(nsIContent* aParentContent, nsIContent* aChild, nsIStyleContext* aStyle);
  nsresult AddNodeFor(nsIContent* aParentContent, nsIStyleContext* aPseudoStyle);

  nsresult RemoveNodeFor(nsIContent* aParentContent, UndisplayedNode* aNode);
  nsresult RemoveNodesFor(nsIContent* aParentContent);

  // Removes all entries from the hash table
  void  Clear(void);

protected:
  PLHashEntry** GetEntryFor(nsIContent* aParentContent);
  nsresult      AppendNodeFor(UndisplayedNode* aNode, nsIContent* aParentContent);

  PLHashTable*  mTable;
  PLHashEntry** mLastLookup;
};

//----------------------------------------------------------------------

class FrameManager;

struct CantRenderReplacedElementEvent : public PLEvent {
  CantRenderReplacedElementEvent(FrameManager* aFrameManager, nsIFrame* aFrame);

  nsIFrame*  mFrame;                     // the frame that can't be rendered
  CantRenderReplacedElementEvent* mNext; // next event in the list
};

class FrameManager : public nsIFrameManager
{
public:
  FrameManager();
  virtual ~FrameManager();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIFrameManager
  NS_IMETHOD Init(nsIPresShell* aPresShell, nsIStyleSet* aStyleSet);

  // Gets and sets the root frame
  NS_IMETHOD GetRootFrame(nsIFrame** aRootFrame) const;
  NS_IMETHOD SetRootFrame(nsIFrame* aRootFrame);
  
  // Primary frame functions
  NS_IMETHOD GetPrimaryFrameFor(nsIContent* aContent, nsIFrame** aPrimaryFrame);
  NS_IMETHOD SetPrimaryFrameFor(nsIContent* aContent,
                                nsIFrame*   aPrimaryFrame);
  NS_IMETHOD ClearPrimaryFrameMap();

  // Placeholder frame functions
  NS_IMETHOD GetPlaceholderFrameFor(nsIFrame*  aFrame,
                                    nsIFrame** aPlaceholderFrame) const;
  NS_IMETHOD SetPlaceholderFrameFor(nsIFrame* aFrame,
                                    nsIFrame* aPlaceholderFrame);
  NS_IMETHOD ClearPlaceholderFrameMap();

  // Undisplayed content functions
  NS_IMETHOD SetUndisplayedContent(nsIContent* aContent, nsIStyleContext* aStyleContext);
  NS_IMETHOD SetUndisplayedPseudoIn(nsIStyleContext* aPseudoContext, 
                                    nsIContent* aParentContent);
  NS_IMETHOD ClearUndisplayedContentIn(nsIContent* aContent, nsIContent* aParentContent);
  NS_IMETHOD ClearAllUndisplayedContentIn(nsIContent* aParentContent);
  NS_IMETHOD ClearUndisplayedContentMap();

  // Functions for manipulating the frame model
  NS_IMETHOD AppendFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIPresContext& aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIFrame*       aParentFrame,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD ReplaceFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame,
                          nsIFrame*       aNewFrame);
  
  NS_IMETHOD CantRenderReplacedElement(nsIPresContext* aPresContext,
                                       nsIFrame*       aFrame);

  NS_IMETHOD NotifyDestroyingFrame(nsIFrame* aFrame);

  NS_IMETHOD ReParentStyleContext(nsIPresContext& aPresContext,
                                  nsIFrame* aFrame, 
                                  nsIStyleContext* aNewParentContext);
  NS_IMETHOD ComputeStyleChangeFor(nsIPresContext& aPresContext,
                                   nsIFrame* aFrame, 
                                   PRInt32 aAttrNameSpaceID,
                                   nsIAtom* aAttribute,
                                   nsStyleChangeList& aChangeList,
                                   PRInt32 aMinChange,
                                   PRInt32& aTopLevelChange);

  NS_IMETHOD CaptureFrameState(nsIPresContext*        aPresContext,
                               nsIFrame*              aFrame,
                               nsILayoutHistoryState* aState);
  NS_IMETHOD RestoreFrameState(nsIPresContext*        aPresContext,
                               nsIFrame*              aFrame,
                               nsILayoutHistoryState* aState);

  // Gets and sets properties on a given frame
  NS_IMETHOD GetFrameProperty(nsIFrame* aFrame,
                              nsIAtom*  aPropertyName,
                              PRUint32  aOptions,
                              void**    aPropertyValue);
  NS_IMETHOD SetFrameProperty(nsIFrame*            aFrame,
                              nsIAtom*             aPropertyName,
                              void*                aPropertyValue,
                              NSFMPropertyDtorFunc aPropDtorFunc);
  NS_IMETHOD RemoveFrameProperty(nsIFrame* aFrame,
                                 nsIAtom*  aPropertyName);

#ifdef NS_DEBUG
  NS_IMETHOD DebugVerifyStyleTree(nsIFrame* aFrame);
#endif

private:
  struct PropertyList {
    nsCOMPtr<nsIAtom>    mName;          // property name
    nsDST*               mFrameValueMap; // map of frame/value pairs
    NSFMPropertyDtorFunc mDtorFunc;      // property specific value dtor function
    PropertyList*        mNext;

    PropertyList(nsIAtom*             aName,
                 NSFMPropertyDtorFunc aDtorFunc,
                 nsDST::NodeArena*    aDSTNodeArena);
    ~PropertyList();

    // Removes the property associated with the given frame, and destroys
    // the property value
    PRBool RemovePropertyForFrame(nsIPresContext* aPresContext, nsIFrame* aFrame);

    // Remove and destroy all remaining properties
    void   RemoveAllProperties(nsIPresContext* aPresContext);
  };

  nsIPresShell*                   mPresShell;    // weak link, because the pres shell owns us
  nsIStyleSet*                    mStyleSet;     // weak link. pres shell holds a reference
  nsIFrame*                       mRootFrame;
  nsDST::NodeArena*               mDSTNodeArena; // weak link. DST owns
  nsDST*                          mPrimaryFrameMap;
  FrameHashTable*                 mPlaceholderMap;
  UndisplayedMap*                 mUndisplayedMap;
  CantRenderReplacedElementEvent* mPostedEvents;
  PropertyList*                   mPropertyList;

  void ReResolveStyleContext(nsIPresContext& aPresContext,
                             nsIFrame* aFrame,
                             nsIStyleContext* aParentContext,
                             nsIContent* aParentContent,
                             PRInt32 aAttrNameSpaceID,
                             nsIAtom* aAttribute,
                             nsStyleChangeList& aChangeList, 
                             PRInt32 aMinChange,
                             PRInt32& aResultChange);

  void RevokePostedEvents();
  CantRenderReplacedElementEvent** FindPostedEventFor(nsIFrame* aFrame);
  void DequeuePostedEventFor(nsIFrame* aFrame);
  void DestroyPropertyList(nsIPresContext* aPresContext);
  PropertyList* GetPropertyListFor(nsIAtom* aPropertyName) const;
  void RemoveAllPropertiesFor(nsIPresContext* aPresContext, nsIFrame* aFrame);

  friend struct CantRenderReplacedElementEvent;
  static void HandlePLEvent(CantRenderReplacedElementEvent* aEvent);
  static void DestroyPLEvent(CantRenderReplacedElementEvent* aEvent);
};

//----------------------------------------------------------------------

NS_LAYOUT nsresult
NS_NewFrameManager(nsIFrameManager** aInstancePtrResult)
{
  NS_ENSURE_ARG_POINTER(aInstancePtrResult);
  if (!aInstancePtrResult) {
    return NS_ERROR_NULL_POINTER;
  }
  FrameManager* it = new FrameManager;
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(nsIFrameManager::GetIID(), (void **)aInstancePtrResult);
}

FrameManager::FrameManager()
{
  NS_INIT_REFCNT();
}

NS_IMPL_ADDREF(FrameManager)
NS_IMPL_RELEASE(FrameManager)

FrameManager::~FrameManager()
{
  nsCOMPtr<nsIPresContext> presContext;
  mPresShell->GetPresContext(getter_AddRefs(presContext));
  
  // Revoke any events posted to the event queue that we haven't processed yet
  RevokePostedEvents();

  // Destroy the frame hierarchy. Don't destroy the property lists until after
  // we've destroyed the frame hierarchy because some frames may expect to be
  // able to retrieve their properties during destruction
  if (mRootFrame) {
    mRootFrame->Destroy(*presContext);
    mRootFrame = nsnull;
  }
  
  delete mPrimaryFrameMap;
  delete mPlaceholderMap;
  delete mUndisplayedMap;
  DestroyPropertyList(presContext);
}

nsresult
FrameManager::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (aIID.Equals(GetIID())) {
    *aInstancePtr = (void*)(nsIFrameManager*)this;
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return NS_NOINTERFACE;
}

NS_IMETHODIMP
FrameManager::Init(nsIPresShell* aPresShell,
                   nsIStyleSet*  aStyleSet)
{
  mPresShell = aPresShell;
  mStyleSet = aStyleSet;

  // Allocate the node arena that's shared by all the DST objects
  mDSTNodeArena = nsDST::NewMemoryArena();
  if (!mDSTNodeArena) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

NS_IMETHODIMP
FrameManager::GetRootFrame(nsIFrame** aRootFrame) const
{
  NS_ENSURE_ARG_POINTER(aRootFrame);
  *aRootFrame = mRootFrame;
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::SetRootFrame(nsIFrame* aRootFrame)
{
  NS_PRECONDITION(!mRootFrame, "already have a root frame");
  if (mRootFrame) {
    return NS_ERROR_UNEXPECTED;
  }

  mRootFrame = aRootFrame;
  return NS_OK;
}

//----------------------------------------------------------------------

// Primary frame functions
NS_IMETHODIMP
FrameManager::GetPrimaryFrameFor(nsIContent* aContent, nsIFrame** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aContent);
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mPrimaryFrameMap) {
    mPrimaryFrameMap->Search(aContent, 0, (void**)aResult);
    if (!*aResult) {
      nsCOMPtr<nsIStyleSet>    styleSet;
      nsCOMPtr<nsIPresContext> presContext;

      // Give the frame construction code the opportunity to return the
      // frame that maps the content object
      mPresShell->GetStyleSet(getter_AddRefs(styleSet));
      mPresShell->GetPresContext(getter_AddRefs(presContext));
      styleSet->FindPrimaryFrameFor(presContext, this, aContent, aResult);
    }
  } else {
    *aResult = nsnull;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::SetPrimaryFrameFor(nsIContent* aContent,
                                 nsIFrame*   aPrimaryFrame)
{
  NS_ENSURE_ARG_POINTER(aContent);

  // If aPrimaryFrame is NULL, then remove the mapping
  if (!aPrimaryFrame) {
    if (mPrimaryFrameMap) {
      mPrimaryFrameMap->Remove(aContent);
    }
  } else {
    // Create a new DST if necessary
    if (!mPrimaryFrameMap) {
      mPrimaryFrameMap = new nsDST(mDSTNodeArena);
      if (!mPrimaryFrameMap) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }

    // Add a mapping to the hash table
    mPrimaryFrameMap->Insert(aContent, (void*)aPrimaryFrame, nsnull);
  }
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::ClearPrimaryFrameMap()
{
  if (mPrimaryFrameMap) {
    mPrimaryFrameMap->Clear();
  }
  return NS_OK;
}

// Placeholder frame functions
NS_IMETHODIMP
FrameManager::GetPlaceholderFrameFor(nsIFrame*  aFrame,
                                     nsIFrame** aResult) const
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aFrame);
  if (!aResult || !aFrame) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mPlaceholderMap) {
    *aResult = (nsIFrame*)mPlaceholderMap->Get(aFrame);
  } else {
    *aResult = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
FrameManager::SetPlaceholderFrameFor(nsIFrame* aFrame,
                                     nsIFrame* aPlaceholderFrame)
{
  NS_ENSURE_ARG_POINTER(aFrame);
#ifdef NS_DEBUG
  // Verify that the placeholder frame is of the correct type
  if (aPlaceholderFrame) {
    nsIAtom*  frameType;
  
    aPlaceholderFrame->GetFrameType(&frameType);
    NS_PRECONDITION(nsLayoutAtoms::placeholderFrame == frameType, "unexpected frame type");
    NS_IF_RELEASE(frameType);
  }
#endif

  // If aPlaceholderFrame is NULL, then remove the mapping
  if (!aPlaceholderFrame) {
    if (mPlaceholderMap) {
      mPlaceholderMap->Remove(aFrame);
    }
  } else {
    // Create a new hash table if necessary
    if (!mPlaceholderMap) {
      mPlaceholderMap = new FrameHashTable;
      if (!mPlaceholderMap) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    
    // Add a mapping to the hash table
    mPlaceholderMap->Put(aFrame, (void*)aPlaceholderFrame);
  }
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::ClearPlaceholderFrameMap()
{
  if (mPlaceholderMap) {
    mPlaceholderMap->Clear();
  }
  return NS_OK;
}

//----------------------------------------------------------------------

NS_IMETHODIMP
FrameManager::SetUndisplayedContent(nsIContent* aContent, 
                                    nsIStyleContext* aStyleContext)
{
  if (! mUndisplayedMap) {
    mUndisplayedMap = new UndisplayedMap;
  }
  if (mUndisplayedMap) {
    nsresult result = NS_OK;
    nsIContent* parent = nsnull;
    aContent->GetParent(parent);
    if (parent) {
      result = mUndisplayedMap->AddNodeFor(parent, aContent, aStyleContext);
      NS_RELEASE(parent);
    }
    return result;
  }
  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
FrameManager::SetUndisplayedPseudoIn(nsIStyleContext* aPseudoContext, 
                                     nsIContent* aParentContent)
{
  if (! mUndisplayedMap) {
    mUndisplayedMap = new UndisplayedMap;
  }
  if (mUndisplayedMap) {
    return mUndisplayedMap->AddNodeFor(aParentContent, aPseudoContext);
  }
  return NS_ERROR_OUT_OF_MEMORY;
}
                                     
NS_IMETHODIMP
FrameManager::ClearUndisplayedContentIn(nsIContent* aContent, nsIContent* aParentContent)
{
  if (mUndisplayedMap) {
    UndisplayedNode* node = mUndisplayedMap->GetFirstNode(aParentContent);
    while (node) {
      if (node->mContent == aContent) {
        return mUndisplayedMap->RemoveNodeFor(aParentContent, node);
      }
      node = node->mNext;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::ClearAllUndisplayedContentIn(nsIContent* aParentContent)
{
  if (mUndisplayedMap) {
    return mUndisplayedMap->RemoveNodesFor(aParentContent);
  }
  return NS_OK;
}

NS_IMETHODIMP
FrameManager::ClearUndisplayedContentMap()
{
  if (mUndisplayedMap) {
    mUndisplayedMap->Clear();
  }
  return NS_OK;
}

//----------------------------------------------------------------------

NS_IMETHODIMP
FrameManager::AppendFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIFrame*       aParentFrame,
                           nsIAtom*        aListName,
                           nsIFrame*       aFrameList)
{
  return aParentFrame->AppendFrames(aPresContext, aPresShell, aListName,
                                    aFrameList);
}

NS_IMETHODIMP
FrameManager::InsertFrames(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIFrame*       aParentFrame,
                           nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList)
{
  return aParentFrame->InsertFrames(aPresContext, aPresShell, aListName,
                                    aPrevFrame, aFrameList);
}

NS_IMETHODIMP
FrameManager::RemoveFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIFrame*       aParentFrame,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame)
{
  return aParentFrame->RemoveFrame(aPresContext, aPresShell, aListName,
                                   aOldFrame);
}

NS_IMETHODIMP
FrameManager::ReplaceFrame(nsIPresContext& aPresContext,
                           nsIPresShell&   aPresShell,
                           nsIFrame*       aParentFrame,
                           nsIAtom*        aListName,
                           nsIFrame*       aOldFrame,
                           nsIFrame*       aNewFrame)
{
  return aParentFrame->ReplaceFrame(aPresContext, aPresShell, aListName,
                                    aOldFrame, aNewFrame);
}

//----------------------------------------------------------------------

NS_IMETHODIMP
FrameManager::NotifyDestroyingFrame(nsIFrame* aFrame)
{
  // Dequeue and destroy and posted events for this frame
  DequeuePostedEventFor(aFrame);

  // Remove all properties associated with the frame
  nsCOMPtr<nsIPresContext> presContext;
  mPresShell->GetPresContext(getter_AddRefs(presContext));

  RemoveAllPropertiesFor(presContext, aFrame);
  return NS_OK;
}

void
FrameManager::RevokePostedEvents()
{
  if (mPostedEvents) {
    mPostedEvents = nsnull;

    // Revoke any events in the event queue that are owned by us
    nsIEventQueueService* eventService;
    nsresult              rv;

    rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                      kIEventQueueServiceIID,
                                      (nsISupports **)&eventService);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIEventQueue> eventQueue;
      rv = eventService->GetThreadEventQueue(PR_GetCurrentThread(), 
                                             getter_AddRefs(eventQueue));
      nsServiceManager::ReleaseService(kEventQueueServiceCID, eventService);

      if (NS_SUCCEEDED(rv) && eventQueue) {
        eventQueue->RevokeEvents(this);
      }
    }
  }
}

CantRenderReplacedElementEvent**
FrameManager::FindPostedEventFor(nsIFrame* aFrame)
{
  CantRenderReplacedElementEvent** event = &mPostedEvents;

  while (*event) {
    if ((*event)->mFrame == aFrame) {
      return event;
    }
    event = &(*event)->mNext;
  }

  return event;
}

void
FrameManager::DequeuePostedEventFor(nsIFrame* aFrame)
{
  // If there's a posted event for this frame, then remove it
  CantRenderReplacedElementEvent** event = FindPostedEventFor(aFrame);
  if (*event) {
    CantRenderReplacedElementEvent* tmp = *event;

    // Remove it from our linked list of posted events
    *event = (*event)->mNext;
    
    // Dequeue it from the event queue
    nsIEventQueueService* eventService;
    nsresult              rv;

    rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                      kIEventQueueServiceIID,
                                      (nsISupports **)&eventService);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIEventQueue> eventQueue;
      rv = eventService->GetThreadEventQueue(PR_GetCurrentThread(), 
                                             getter_AddRefs(eventQueue));
      nsServiceManager::ReleaseService(kEventQueueServiceCID, eventService);

      if (NS_SUCCEEDED(rv) && eventQueue) {
        PLEventQueue* plqueue;

        eventQueue->GetPLEventQueue(&plqueue);
        if (plqueue) {
          // Removes the event and destroys it
          PL_DequeueEvent(tmp, plqueue);
        }
      }
    }
  }
}

void
FrameManager::HandlePLEvent(CantRenderReplacedElementEvent* aEvent)
{
  FrameManager* frameManager = (FrameManager*)aEvent->owner;
  
  // Remove the posted event from the linked list
  CantRenderReplacedElementEvent** events = &frameManager->mPostedEvents;
  while (*events) {
    if (*events == aEvent) {
      *events = (*events)->mNext;
      break;
    } else {
      events = &(*events)->mNext;
    }
  }

  // Notify the style system and then process any reflow commands that
  // are generated
  nsCOMPtr<nsIPresContext>  presContext;
  frameManager->mPresShell->GetPresContext(getter_AddRefs(presContext));
  frameManager->mStyleSet->CantRenderReplacedElement(presContext, aEvent->mFrame);
  frameManager->mPresShell->ProcessReflowCommands();
}

void
FrameManager::DestroyPLEvent(CantRenderReplacedElementEvent* aEvent)
{
  delete aEvent;
}

CantRenderReplacedElementEvent::CantRenderReplacedElementEvent(FrameManager* aFrameManager,
                                                               nsIFrame*     aFrame)
{
  // Note: because the frame manager owns us we don't hold a reference to the
  // frame manager
  PL_InitEvent(this, aFrameManager, (PLHandleEventProc)&FrameManager::HandlePLEvent,
               (PLDestroyEventProc)&FrameManager::DestroyPLEvent);
  mFrame = aFrame;
}

NS_IMETHODIMP
FrameManager::CantRenderReplacedElement(nsIPresContext* aPresContext,
                                        nsIFrame*       aFrame)
{
  nsIEventQueueService* eventService;
  nsresult              rv;

  // We need to notify the style stystem, but post the notification so it
  // doesn't happen now
  rv = nsServiceManager::GetService(kEventQueueServiceCID,
                                    kIEventQueueServiceIID,
                                    (nsISupports **)&eventService);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIEventQueue> eventQueue;
    rv = eventService->GetThreadEventQueue(PR_GetCurrentThread(), 
                                           getter_AddRefs(eventQueue));
    nsServiceManager::ReleaseService(kEventQueueServiceCID, eventService);

    if (NS_SUCCEEDED(rv) && eventQueue) {
#ifdef NS_DEBUG
      // Verify that there isn't already a posted event associated with
      // this frame
      NS_ASSERTION(!*FindPostedEventFor(aFrame), "frame already has posted event");
#endif
      CantRenderReplacedElementEvent* ev;

      // Create a new event
      ev = new CantRenderReplacedElementEvent(this, aFrame);

      // Add the event to our linked list of posted events
      ev->mNext = mPostedEvents;
      mPostedEvents = ev;

      // Post the event
      eventQueue->PostEvent(ev);
    }
  }

  return rv;
}

#ifdef NS_DEBUG
static void
DumpContext(nsIFrame* aFrame, nsIStyleContext* aContext)
{
  if (aFrame) {
    fputs("frame: ", stdout);
    nsAutoString  name;
    nsIFrameDebug*  frameDebug;

    if (NS_SUCCEEDED(aFrame->QueryInterface(nsIFrameDebug::GetIID(), (void**)&frameDebug))) {
      frameDebug->GetFrameName(name);
      fputs(name, stdout);
    }
    fprintf(stdout, " (%p)", aFrame);
  }
  if (aContext) {
    fprintf(stdout, " style: %p ", aContext);

    nsIAtom* pseudoTag;
    aContext->GetPseudoType(pseudoTag);
    if (pseudoTag) {
      nsAutoString  buffer;
      pseudoTag->ToString(buffer);
      fputs(buffer, stdout);
      fputs(" ", stdout);
      NS_RELEASE(pseudoTag);
    }

    PRInt32 count = aContext->GetStyleRuleCount();
    if (0 < count) {
      fputs("{\n", stdout);
      nsISupportsArray* rules = aContext->GetStyleRules();
      PRInt32 ix;
      for (ix = 0; ix < count; ix++) {
        nsIStyleRule* rule = (nsIStyleRule*)rules->ElementAt(ix);
        rule->List(stdout, 1);
        NS_RELEASE(rule);
      }
      NS_RELEASE(rules);
      fputs("}\n", stdout);
    }
    else {
      fputs("{}\n", stdout);
    }
  }
}

static void
VerifyContextParent(nsIFrame* aFrame, nsIStyleContext* aContext, nsIStyleContext* aParentContext)
{
  nsIStyleContext*  actualParentContext = aContext->GetParent();

  if (aParentContext) {
    if (aParentContext != actualParentContext) {
      DumpContext(aFrame, aContext);
      if (aContext == aParentContext) {
        fputs("Using parent's style context\n\n", stdout);
      }
      else {
        fputs("Wrong parent style context: ", stdout);
        DumpContext(nsnull, actualParentContext);
        fputs("should be using: ", stdout);
        DumpContext(nsnull, aParentContext);
        fputs("\n", stdout);
      }
    }
  }
  else {
    if (actualParentContext) {
      DumpContext(aFrame, aContext);
      fputs("Has parent context: ", stdout);
      DumpContext(nsnull, actualParentContext);
      fputs("Should be null\n\n", stdout);
    }
  }
  NS_IF_RELEASE(actualParentContext);
}

static void
VerifyContextParent(nsIFrame* aFrame, nsIStyleContext* aParentContext)
{
  nsIStyleContext*  context;
  aFrame->GetStyleContext(&context);
  VerifyContextParent(aFrame, context, aParentContext);
  NS_RELEASE(context);
}

static void
VerifyStyleTree(nsIFrame* aFrame, nsIStyleContext* aParentContext)
{
  nsIStyleContext*  context;
  aFrame->GetStyleContext(&context);

  VerifyContextParent(aFrame, aParentContext);

  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;
  nsIFrame* child;
  nsIAtom*  frameType;

  do {
    child = nsnull;
    nsresult result = aFrame->FirstChild(childList, &child);
    while ((NS_SUCCEEDED(result)) && child) {
      nsFrameState  state;
      child->GetFrameState(&state);
      if (NS_FRAME_OUT_OF_FLOW != (state & NS_FRAME_OUT_OF_FLOW)) {
        // only do frames that are in flow
        child->GetFrameType(&frameType);
        if (nsLayoutAtoms::placeholderFrame == frameType) { // placeholder
          // get out of flow frame and recurse there
          nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)child)->GetOutOfFlowFrame();
          NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

          nsIStyleContext* outOfFlowContext;
          outOfFlowFrame->GetStyleContext(&outOfFlowContext);
          VerifyContextParent(child, outOfFlowContext);
          NS_RELEASE(outOfFlowContext);

          VerifyStyleTree(outOfFlowFrame, context);
        }
        else { // regular frame
          VerifyStyleTree(child, context);
        }
        NS_IF_RELEASE(frameType);
      }

      child->GetNextSibling(&child);
    }

    NS_IF_RELEASE(childList);
    aFrame->GetAdditionalChildListName(listIndex++, &childList);
  } while (childList);
  
  // do additional contexts 
  PRInt32 contextIndex = -1;
  while (1 == 1) {
    nsIStyleContext* extraContext = nsnull;
    aFrame->GetAdditionalStyleContext(++contextIndex, &extraContext);
    if (extraContext) {
      VerifyContextParent(aFrame, extraContext, context);
      NS_RELEASE(extraContext);
    }
    else {
      break;
    }
  }
  NS_RELEASE(context);
}

NS_IMETHODIMP
FrameManager::DebugVerifyStyleTree(nsIFrame* aFrame)
{
  if (aFrame) {
    nsIStyleContext* context;
    aFrame->GetStyleContext(&context);
    nsIStyleContext* parentContext = context->GetParent();
    VerifyStyleTree(aFrame, parentContext);
    NS_IF_RELEASE(parentContext);
    NS_RELEASE(context);
  }
  return NS_OK;
}
#endif

NS_IMETHODIMP
FrameManager::ReParentStyleContext(nsIPresContext& aPresContext,
                                   nsIFrame* aFrame, 
                                   nsIStyleContext* aNewParentContext)
{
  nsresult result = NS_ERROR_NULL_POINTER;
  if (aFrame) {
#ifdef NS_DEBUG
    DebugVerifyStyleTree(aFrame);
#endif

    nsIStyleContext* oldContext = nsnull;
    aFrame->GetStyleContext(&oldContext);

    if (oldContext) {
      nsIStyleContext* newContext = nsnull;
      result = mStyleSet->ReParentStyleContext(&aPresContext, 
                                               oldContext, aNewParentContext,
                                               &newContext);
      if (NS_SUCCEEDED(result) && newContext) {
        if (newContext != oldContext) {
          PRInt32 listIndex = 0;
          nsIAtom* childList = nsnull;
          nsIFrame* child;
          nsIAtom*  frameType;

          do {
            child = nsnull;
            result = aFrame->FirstChild(childList, &child);
            while ((NS_SUCCEEDED(result)) && child) {
              nsFrameState  state;
              child->GetFrameState(&state);
              if (NS_FRAME_OUT_OF_FLOW != (state & NS_FRAME_OUT_OF_FLOW)) {
                // only do frames that are in flow
                child->GetFrameType(&frameType);
                if (nsLayoutAtoms::placeholderFrame == frameType) { // placeholder
                  // get out of flow frame and recurse there
                  nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)child)->GetOutOfFlowFrame();
                  NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

                  result = ReParentStyleContext(aPresContext, outOfFlowFrame, newContext);

                  // reparent placeholder's context under out of flow frame
                  nsIStyleContext*  outOfFlowContext;
                  outOfFlowFrame->GetStyleContext(&outOfFlowContext);
                  ReParentStyleContext(aPresContext, child, outOfFlowContext);
                  NS_RELEASE(outOfFlowContext);
                }
                else { // regular frame
                  result = ReParentStyleContext(aPresContext, child, newContext);
                }
                NS_IF_RELEASE(frameType);
              }

              child->GetNextSibling(&child);
            }

            NS_IF_RELEASE(childList);
            aFrame->GetAdditionalChildListName(listIndex++, &childList);
          } while (childList);
          
          aFrame->SetStyleContext(&aPresContext, newContext);

          // do additional contexts 
          PRInt32 contextIndex = -1;
          while (1 == 1) {
            nsIStyleContext* oldExtraContext = nsnull;
            result = aFrame->GetAdditionalStyleContext(++contextIndex, &oldExtraContext);
            if (NS_SUCCEEDED(result)) {
              if (oldExtraContext) {
                nsIStyleContext* newExtraContext = nsnull;
                result = mStyleSet->ReParentStyleContext(&aPresContext,
                                                         oldExtraContext, newContext,
                                                         &newExtraContext);
                if (NS_SUCCEEDED(result) && newExtraContext) {
                  aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
                  NS_RELEASE(newExtraContext);
                }
                NS_RELEASE(oldExtraContext);
              }
            }
            else {
              result = NS_OK; // ok not to have extras (or run out)
              break;
            }
          }
#ifdef NS_DEBUG
          VerifyStyleTree(aFrame, aNewParentContext);
#endif
        }
        NS_RELEASE(newContext);
      }
      NS_RELEASE(oldContext);
    }
  }
  return result;
}

static PRBool
HasAttributeContent(nsIStyleContext* aStyleContext, 
                    PRInt32 aNameSpaceID,
                    nsIAtom* aAttribute)
{
  PRBool  result = PR_FALSE;
  if (aStyleContext) {
    const nsStyleContent* content = (const nsStyleContent*)aStyleContext->GetStyleData(eStyleStruct_Content);
    PRUint32 count = content->ContentCount();
    while ((0 < count) && (! result)) {
      nsStyleContentType  contentType;
      nsAutoString        contentString;
      content->GetContentAt(--count, contentType, contentString);
      if (eStyleContentType_Attr == contentType) {
        nsIAtom* attrName = nsnull;
        PRInt32 attrNameSpace = kNameSpaceID_None;
        PRInt32 barIndex = contentString.FindChar('|'); // CSS namespace delimiter
        if (-1 != barIndex) {
          nsAutoString  nameSpaceVal;
          contentString.Left(nameSpaceVal, barIndex);
          PRInt32 error;
          attrNameSpace = nameSpaceVal.ToInteger(&error, 10);
          contentString.Cut(0, barIndex + 1);
          if (contentString.Length()) {
            attrName = NS_NewAtom(contentString);
          }
        }
        else {
          attrName = NS_NewAtom(contentString);
        }
        if ((attrName == aAttribute) && 
            ((attrNameSpace == aNameSpaceID) || 
             (attrNameSpace == kNameSpaceID_Unknown))) {
          result = PR_TRUE;
        }
        NS_IF_RELEASE(attrName);
      }
    }
  }
  return result;
}
    

static PRInt32
CaptureChange(nsIStyleContext* aOldContext, nsIStyleContext* aNewContext,
              nsIFrame* aFrame, nsIContent* aContent,
              nsStyleChangeList& aChangeList, PRInt32 aMinChange)
{
  PRInt32 ourChange = NS_STYLE_HINT_NONE;
  aNewContext->CalcStyleDifference(aOldContext, ourChange);
  if (aMinChange < ourChange) {
    aChangeList.AppendChange(aFrame, aContent, ourChange);
    aMinChange = ourChange;
  }
  return aMinChange;
}

void
FrameManager::ReResolveStyleContext(nsIPresContext& aPresContext,
                                    nsIFrame* aFrame,
                                    nsIStyleContext* aParentContext,
                                    nsIContent* aParentContent,
                                    PRInt32 aAttrNameSpaceID,
                                    nsIAtom* aAttribute,
                                    nsStyleChangeList& aChangeList, 
                                    PRInt32 aMinChange,
                                    PRInt32& aResultChange)
{
  nsIStyleContext* oldContext = nsnull; 
  nsresult result = aFrame->GetStyleContext(&oldContext);
  if (NS_SUCCEEDED(result) && oldContext) {
    nsIAtom* pseudoTag = nsnull;
    oldContext->GetPseudoType(pseudoTag);
    nsIContent* localContent = nsnull;
    nsIContent* content = nsnull;
    result = aFrame->GetContent(&localContent);
    if (NS_SUCCEEDED(result) && localContent) {
      content = localContent;
    }
    else {
      content = aParentContent;
    }
    if (aParentContent && aAttribute) { // attribute came from parent, we don't care about it here when recursing
      nsFrameState  frameState;
      aFrame->GetFrameState(&frameState);
      if (0 == (frameState & NS_FRAME_GENERATED_CONTENT)) { // keep it for generated content
        aAttribute = nsnull;
      }
    }

    // do primary context
    nsIStyleContext* newContext = nsnull;
    if (pseudoTag) {
      aPresContext.ResolvePseudoStyleContextFor(content, pseudoTag, aParentContext, PR_FALSE,
                                                &newContext);
      NS_RELEASE(pseudoTag);
    }
    else {
      NS_ASSERTION(localContent, "non pseudo-element frame without content node");
      aPresContext.ResolveStyleContextFor(content, aParentContext, PR_FALSE, &newContext);
    }
    NS_ASSERTION(newContext, "failed to get new style context");
    if (newContext) {
      if (newContext != oldContext) {
        aMinChange = CaptureChange(oldContext, newContext, aFrame, content, aChangeList, aMinChange);
        if (aMinChange < NS_STYLE_HINT_FRAMECHANGE) { // if frame gets regenerated, let it keep old context
          aFrame->SetStyleContext(&aPresContext, newContext);
        }
      }
      else {
        oldContext->RemapStyle(&aPresContext, PR_FALSE);
        if (aAttribute && (aMinChange < NS_STYLE_HINT_REFLOW) &&
            HasAttributeContent(oldContext, aAttrNameSpaceID, aAttribute)) {
          aChangeList.AppendChange(aFrame, content, NS_STYLE_HINT_REFLOW);
        }
      }
      NS_RELEASE(oldContext);
    }
    else {
      NS_ERROR("resolve style context failed");
      newContext = oldContext;  // new context failed, recover... (take ref)
    }

    // do additional contexts 
    PRInt32 contextIndex = -1;
    while (1 == 1) {
      nsIStyleContext* oldExtraContext = nsnull;
      result = aFrame->GetAdditionalStyleContext(++contextIndex, &oldExtraContext);
      if (NS_SUCCEEDED(result)) {
        if (oldExtraContext) {
          nsIStyleContext* newExtraContext = nsnull;
          oldExtraContext->GetPseudoType(pseudoTag);
          NS_ASSERTION(pseudoTag, "extra style context is not pseudo element");
          result = aPresContext.ResolvePseudoStyleContextFor(content, pseudoTag, newContext,
                                                             PR_FALSE, &newExtraContext);
          NS_RELEASE(pseudoTag);
          if (NS_SUCCEEDED(result) && newExtraContext) {
            if (oldExtraContext != newExtraContext) {
              aMinChange = CaptureChange(oldExtraContext, newExtraContext, aFrame, 
                                         content, aChangeList, aMinChange);
              if (aMinChange < NS_STYLE_HINT_FRAMECHANGE) {
                aFrame->SetAdditionalStyleContext(contextIndex, newExtraContext);
              }
            }
            else {
              oldExtraContext->RemapStyle(&aPresContext, PR_FALSE);
              if (aAttribute && (aMinChange < NS_STYLE_HINT_REFLOW) &&
                  HasAttributeContent(oldContext, aAttrNameSpaceID, aAttribute)) {
                aChangeList.AppendChange(aFrame, content, NS_STYLE_HINT_REFLOW);
              }
            }
            NS_RELEASE(newExtraContext);
          }
          NS_RELEASE(oldExtraContext);
        }
      }
      else {
        break;
      }
    }

    // now look for undisplayed child content and pseudos
    if (localContent && mUndisplayedMap) {
      UndisplayedNode* undisplayed = mUndisplayedMap->GetFirstNode(localContent);
      while (undisplayed) {
        nsIStyleContext* undisplayedContext = nsnull;
        if (undisplayed->mContent) {  // child content
          aPresContext.ResolveStyleContextFor(undisplayed->mContent, newContext, 
                                              PR_FALSE, &undisplayedContext);
        }
        else {  // pseudo element
          undisplayed->mStyle->GetPseudoType(pseudoTag);
          NS_ASSERTION(pseudoTag, "pseudo element without tag");
          aPresContext.ResolvePseudoStyleContextFor(localContent, pseudoTag, newContext, PR_FALSE,
                                                    &undisplayedContext);
          NS_RELEASE(pseudoTag);
        }
        if (undisplayedContext) {
          if (undisplayedContext == undisplayed->mStyle) {
            undisplayedContext->RemapStyle(&aPresContext);
          }
          const nsStyleDisplay* display = 
                (const nsStyleDisplay*)undisplayedContext->GetStyleData(eStyleStruct_Display);
          if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
            aChangeList.AppendChange(nsnull, ((undisplayed->mContent) ? undisplayed->mContent : localContent), 
                                     NS_STYLE_HINT_FRAMECHANGE);
          }
          NS_RELEASE(undisplayedContext);
        }
        undisplayed = undisplayed->mNext;
      }
    }

    aResultChange = aMinChange;

    // now do children
    PRInt32 listIndex = 0;
    nsIAtom* childList = nsnull;
    PRInt32 childChange;
    nsIFrame* child;
    nsIAtom* frameType = nsnull;

    do {
      child = nsnull;
      result = aFrame->FirstChild(childList, &child);
      while ((NS_SUCCEEDED(result)) && (child)) {
        nsFrameState  state;
        child->GetFrameState(&state);
        if (NS_FRAME_OUT_OF_FLOW != (state & NS_FRAME_OUT_OF_FLOW)) {
          // only do frames that are in flow
          child->GetFrameType(&frameType);
          if (nsLayoutAtoms::placeholderFrame == frameType) { // placeholder
            // get out of flow frame and recurse there
            nsIFrame* outOfFlowFrame = ((nsPlaceholderFrame*)child)->GetOutOfFlowFrame();
            NS_ASSERTION(outOfFlowFrame, "no out-of-flow frame");

            ReResolveStyleContext(aPresContext, outOfFlowFrame, newContext, content,
                                  aAttrNameSpaceID, aAttribute,
                                  aChangeList, aMinChange, childChange);

            // reresolve placeholder's context under out of flow frame
            nsIStyleContext*  outOfFlowContext;
            outOfFlowFrame->GetStyleContext(&outOfFlowContext);
            ReResolveStyleContext(aPresContext, child, outOfFlowContext, content,
                                  kNameSpaceID_Unknown, nsnull,
                                  aChangeList, aMinChange, childChange);
            NS_RELEASE(outOfFlowContext);
          }
          else {  // regular child frame
            ReResolveStyleContext(aPresContext, child, newContext, content,
                                  aAttrNameSpaceID, aAttribute,
                                  aChangeList, aMinChange, childChange);
          }
        }
        child->GetNextSibling(&child);
      }

      NS_IF_RELEASE(childList);
      aFrame->GetAdditionalChildListName(listIndex++, &childList);
    } while (childList);
    // XXX need to do overflow frames???

    NS_RELEASE(newContext);
    NS_IF_RELEASE(localContent);
  }
}

NS_IMETHODIMP
FrameManager::ComputeStyleChangeFor(nsIPresContext& aPresContext,
                                    nsIFrame* aFrame, 
                                    PRInt32 aAttrNameSpaceID,
                                    nsIAtom* aAttribute,
                                    nsStyleChangeList& aChangeList,
                                    PRInt32 aMinChange,
                                    PRInt32& aTopLevelChange)
{
  aTopLevelChange = NS_STYLE_HINT_NONE;
  nsIFrame* frame = aFrame;

  do {
    nsIStyleContext* styleContext = nsnull;
    frame->GetStyleContext(&styleContext);
    nsIStyleContext* parentContext = styleContext->GetParent();
    PRInt32 frameChange;
    ReResolveStyleContext(aPresContext, frame, parentContext, nsnull,
                          aAttrNameSpaceID, aAttribute,
                          aChangeList, aMinChange, frameChange);
#ifdef NS_DEBUG
    VerifyStyleTree(frame, parentContext);
#endif
    NS_IF_RELEASE(parentContext);
    NS_RELEASE(styleContext);
    if (aTopLevelChange < frameChange) {
      aTopLevelChange = frameChange;
    }

    frame->GetNextInFlow(&frame);
  } while (frame);
  return NS_OK;
}


static nsresult
CaptureFrameStateFor(nsIPresContext* aPresContext, nsIFrame* aFrame, nsILayoutHistoryState* aState)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");

  // See if the frame is stateful.
  // Frames are not ref-counted so no addref/release is required on statefulFrame.
  nsIStatefulFrame* statefulFrame = nsnull;
  aFrame->QueryInterface(nsIStatefulFrame::GetIID(), (void**) &statefulFrame);
  if (nsnull != statefulFrame) {
    // If so, get the content ID, state type and the state and
    // add an association between (ID, type) and (state) to the
    // history state storage object, aState.
    nsCOMPtr<nsIContent> content;    
    rv = aFrame->GetContent(getter_AddRefs(content));   
    if (NS_SUCCEEDED(rv)) {
      PRUint32 ID;
      rv = content->GetContentID(&ID);
      if (NS_SUCCEEDED(rv) && ID) { // Must have ID (don't do anonymous content)
        nsIStatefulFrame::StateType type = nsIStatefulFrame::eNoType;
        rv = statefulFrame->GetStateType(aPresContext, &type);
        if (NS_SUCCEEDED(rv)) {
          nsISupports* frameState;
          rv = statefulFrame->SaveState(aPresContext, &frameState);
          if (NS_SUCCEEDED(rv)) {
            rv = aState->AddState(ID, frameState, type);
          }
        }
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
FrameManager::CaptureFrameState(nsIPresContext* aPresContext, nsIFrame* aFrame, nsILayoutHistoryState* aState)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");

  rv = CaptureFrameStateFor(aPresContext, aFrame, aState);

  // Now capture state recursively for the frame hierarchy rooted at aFrame
  nsIAtom*  childListName = nsnull;
  PRInt32   childListIndex = 0;
  do {    
    nsIFrame* childFrame;
    aFrame->FirstChild(childListName, &childFrame);
    while (childFrame) {             
      rv = CaptureFrameState(aPresContext, childFrame, aState);
      // Get the next sibling child frame
      childFrame->GetNextSibling(&childFrame);
    }
    NS_IF_RELEASE(childListName);
    aFrame->GetAdditionalChildListName(childListIndex++, &childListName);
  } while (childListName);

  return rv;
}

static nsresult
RestoreFrameStateFor(nsIPresContext* aPresContext, nsIFrame* aFrame, nsILayoutHistoryState* aState)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");

  // See if the frame is stateful.
  // Frames are not ref-counted so no addref/release is required on statefulFrame.
  nsIStatefulFrame* statefulFrame = nsnull;
  aFrame->QueryInterface(nsIStatefulFrame::GetIID(), (void**) &statefulFrame);
  if (nsnull != statefulFrame) {
    // If so, get the content ID, state type and the frame state and
    // ask the frame object to restore its state.    
    nsCOMPtr<nsIContent> content;    
    rv = aFrame->GetContent(getter_AddRefs(content));   
    if (NS_SUCCEEDED(rv)) {
      PRUint32 ID;
      rv = content->GetContentID(&ID);
      if (NS_SUCCEEDED(rv) && ID) { // Must have ID (don't do anonymous content)
        nsIStatefulFrame::StateType type = nsIStatefulFrame::eNoType;
        rv = statefulFrame->GetStateType(aPresContext, &type);
        if (NS_SUCCEEDED(rv)) {
          nsISupports* frameState = nsnull;
          rv = aState->GetState(ID, &frameState, type);          
          if (NS_SUCCEEDED(rv) && nsnull != frameState) {
            rv = statefulFrame->RestoreState(aPresContext, frameState);  
          }
        }
      }
    }
  }  

  return rv;
}

NS_IMETHODIMP
FrameManager::RestoreFrameState(nsIPresContext* aPresContext, nsIFrame* aFrame, nsILayoutHistoryState* aState)
{
  nsresult rv = NS_OK;
  NS_PRECONDITION(nsnull != aFrame && nsnull != aState, "null parameters passed in");
  
  rv = RestoreFrameStateFor(aPresContext, aFrame, aState);

  // Now restore state recursively for the frame hierarchy rooted at aFrame
  nsIAtom*  childListName = nsnull;
  PRInt32   childListIndex = 0;
  do {    
    nsIFrame* childFrame;
    aFrame->FirstChild(childListName, &childFrame);
    while (childFrame) {             
      rv = RestoreFrameState(aPresContext, childFrame, aState);
      // Get the next sibling child frame
      childFrame->GetNextSibling(&childFrame);
    }
    NS_IF_RELEASE(childListName);
    aFrame->GetAdditionalChildListName(childListIndex++, &childListName);
  } while (childListName);

  return rv;
}

//----------------------------------------------------------------------

static PLHashNumber
HashKey(void* key)
{
  return (PLHashNumber)key;
}

static PRIntn
CompareKeys(void* key1, void* key2)
{
  return key1 == key2;
}

MOZ_DECL_CTOR_COUNTER(FrameHashTable);

FrameHashTable::FrameHashTable(PRUint32 aNumBuckets)
{
  MOZ_COUNT_CTOR(FrameHashTable);
  mTable = PL_NewHashTable(aNumBuckets, (PLHashFunction)HashKey,
                           (PLHashComparator)CompareKeys,
                           (PLHashComparator)nsnull,
                           nsnull, nsnull);
}

FrameHashTable::~FrameHashTable()
{
  MOZ_COUNT_DTOR(FrameHashTable);
  PL_HashTableDestroy(mTable);
}

void*
FrameHashTable::Get(void* aKey)
{
  PRInt32 hashCode = (PRInt32) aKey;
  PLHashEntry** hep = PL_HashTableRawLookup(mTable, hashCode, aKey);
  PLHashEntry* he = *hep;
  if (he) {
    return he->value;
  }
  return nsnull;
}

void*
FrameHashTable::Put(void* aKey, void* aData)
{
  PRInt32 hashCode = (PRInt32) aKey;
  PLHashEntry** hep = PL_HashTableRawLookup(mTable, hashCode, aKey);
  PLHashEntry* he = *hep;
  if (he) {
    void* oldValue = he->value;
    he->value = aData;
    return oldValue;
  }
  PL_HashTableRawAdd(mTable, hep, hashCode, aKey, aData);
  return nsnull;
}

void*
FrameHashTable::Remove(void* aKey)
{
  PRInt32 hashCode = (PRInt32) aKey;
  PLHashEntry** hep = PL_HashTableRawLookup(mTable, hashCode, aKey);
  PLHashEntry* he = *hep;
  void* oldValue = nsnull;
  if (he) {
    oldValue = he->value;
    PL_HashTableRawRemove(mTable, hep, he);
  }
  return oldValue;
}

static PRIntn
RemoveEntry(PLHashEntry* he, PRIntn i, void* arg)
{
  // Remove and free this entry and continue enumerating
  return HT_ENUMERATE_REMOVE | HT_ENUMERATE_NEXT;
}

void
FrameHashTable::Clear()
{
  PL_HashTableEnumerateEntries(mTable, RemoveEntry, 0);
}

#ifdef NS_DEBUG
static PRIntn
EnumEntries(PLHashEntry* he, PRIntn i, void* arg)
{
  // Continue enumerating
  return HT_ENUMERATE_NEXT;
}

void
FrameHashTable::Dump(FILE* fp)
{
  PL_HashTableDump(mTable, EnumEntries, fp);
}
#endif

void
FrameManager::DestroyPropertyList(nsIPresContext* aPresContext)
{
  if (mPropertyList) {
    while (mPropertyList) {
      PropertyList* tmp = mPropertyList;

      mPropertyList = mPropertyList->mNext;
      tmp->RemoveAllProperties(aPresContext);
      delete tmp;
    }
  }
}

FrameManager::PropertyList*
FrameManager::GetPropertyListFor(nsIAtom* aPropertyName) const
{
  PropertyList* result;

  for (result = mPropertyList; result; result = result->mNext) {
    if (result->mName.get() == aPropertyName) {
      break;
    }
  }

  return result;
}

void
FrameManager::RemoveAllPropertiesFor(nsIPresContext* aPresContext,
                                     nsIFrame*       aFrame)
{
  for (PropertyList* prop = mPropertyList; prop; prop = prop->mNext) {
    prop->RemovePropertyForFrame(aPresContext, aFrame);
  }
}

NS_IMETHODIMP
FrameManager::GetFrameProperty(nsIFrame* aFrame,
                               nsIAtom*  aPropertyName,
                               PRUint32  aOptions,
                               void**    aPropertyValue)
{
  NS_ENSURE_ARG_POINTER(aPropertyName);
  PropertyList* propertyList = GetPropertyListFor(aPropertyName);
  nsresult      result;

  if (propertyList) {
    result = propertyList->mFrameValueMap->Search(aFrame, aOptions, aPropertyValue);

  } else {
    *aPropertyValue = 0;
    result = NS_IFRAME_MGR_PROP_NOT_THERE;
  }

  return result;
}

NS_IMETHODIMP
FrameManager::SetFrameProperty(nsIFrame*            aFrame,
                               nsIAtom*             aPropertyName,
                               void*                aPropertyValue,
                               NSFMPropertyDtorFunc aPropDtorFunc)
{
  NS_ENSURE_ARG_POINTER(aPropertyName);
  PropertyList* propertyList = GetPropertyListFor(aPropertyName);
  nsresult      result = NS_OK;

  if (propertyList) {
    // Make sure the dtor function matches
    if (aPropDtorFunc != propertyList->mDtorFunc) {
      return NS_ERROR_INVALID_ARG;
    }

  } else {
    propertyList = new PropertyList(aPropertyName, aPropDtorFunc, mDSTNodeArena);
    if (!propertyList) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    propertyList->mNext = mPropertyList;
    mPropertyList = propertyList;
  }

  // The current property value (if there is one) is replaced and the current
  // value is destroyed
  void* oldValue;
  result = propertyList->mFrameValueMap->Insert(aFrame, aPropertyValue, &oldValue);
  if (NS_DST_VALUE_OVERWRITTEN == result) {
    if (propertyList->mDtorFunc) {
      nsCOMPtr<nsIPresContext> presContext;
      mPresShell->GetPresContext(getter_AddRefs(presContext));

      propertyList->mDtorFunc(presContext, aFrame, aPropertyName, oldValue);
    }
  }

  return result;
}

NS_IMETHODIMP
FrameManager::RemoveFrameProperty(nsIFrame* aFrame,
                                  nsIAtom*  aPropertyName)
{
  NS_ENSURE_ARG_POINTER(aPropertyName);
  PropertyList* propertyList = GetPropertyListFor(aPropertyName);
  nsresult      result = NS_IFRAME_MGR_PROP_NOT_THERE;

  if (propertyList) {
    nsCOMPtr<nsIPresContext> presContext;
    mPresShell->GetPresContext(getter_AddRefs(presContext));
    
    if (propertyList->RemovePropertyForFrame(presContext, aFrame)) {
      result = NS_OK;
    }
  }

  return result;
}

//----------------------------------------------------------------------

MOZ_DECL_CTOR_COUNTER(UndisplayedMap);

UndisplayedMap::UndisplayedMap(PRUint32 aNumBuckets)
{
  MOZ_COUNT_CTOR(UndisplayedMap);
  mTable = PL_NewHashTable(aNumBuckets, (PLHashFunction)HashKey,
                           (PLHashComparator)CompareKeys,
                           (PLHashComparator)nsnull,
                           nsnull, nsnull);
  mLastLookup = nsnull;
}

UndisplayedMap::~UndisplayedMap(void)
{
  MOZ_COUNT_DTOR(UndisplayedMap);
  Clear();
  PL_HashTableDestroy(mTable);
}

PLHashEntry**  
UndisplayedMap::GetEntryFor(nsIContent* aParentContent)
{
  if (mLastLookup && (aParentContent == (*mLastLookup)->key)) {
    return mLastLookup;
  }
  PLHashNumber hashCode = (PLHashNumber)(void*)aParentContent;
  PLHashEntry** entry = PL_HashTableRawLookup(mTable, hashCode, aParentContent);
  if (*entry) {
    mLastLookup = entry;
  }
  return entry;
}

UndisplayedNode* 
UndisplayedMap::GetFirstNode(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  if (*entry) {
    return (UndisplayedNode*)((*entry)->value);
  }
  return nsnull;
}

nsresult      
UndisplayedMap::AppendNodeFor(UndisplayedNode* aNode, nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  if (*entry) {
    UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
    while (node->mNext) {
      NS_ASSERTION((node->mContent != aNode->mContent) ||
                   ((node->mContent == nsnull) && 
                    (node->mStyle != aNode->mStyle)), "node in map twice");
      node = node->mNext;
    }
    node->mNext = aNode;
  }
  else {
    PLHashNumber hashCode = (PLHashNumber)(void*)aParentContent;
    PL_HashTableRawAdd(mTable, entry, hashCode, aParentContent, aNode);
    mLastLookup = nsnull; // hashtable may have shifted bucket out from under us
  }
  return NS_OK;
}

nsresult 
UndisplayedMap::AddNodeFor(nsIContent* aParentContent, nsIContent* aChild, 
                           nsIStyleContext* aStyle)
{
  UndisplayedNode*  node = new UndisplayedNode(aChild, aStyle);
  if (! node) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return AppendNodeFor(node, aParentContent);
}

nsresult
UndisplayedMap::AddNodeFor(nsIContent* aParentContent, nsIStyleContext* aPseudoStyle)
{
  UndisplayedNode*  node = new UndisplayedNode(aPseudoStyle);
  if (! node) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return AppendNodeFor(node, aParentContent);
}

nsresult 
UndisplayedMap::RemoveNodeFor(nsIContent* aParentContent, UndisplayedNode* aNode)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  NS_ASSERTION(*entry, "content not in map");
  if (*entry) {
    if ((UndisplayedNode*)((*entry)->value) == aNode) {  // first node
      if (aNode->mNext) {
        (*entry)->value = aNode->mNext;
        aNode->mNext = nsnull;
      }
      else {
        PL_HashTableRawRemove(mTable, entry, *entry);
        mLastLookup = nsnull; // hashtable may have shifted bucket out from under us
      }
    }
    else {
      UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
      while (node->mNext) {
        if (node->mNext == aNode) {
          node->mNext = aNode->mNext;
          aNode->mNext = nsnull;
          break;
        }
        node = node->mNext;
      }
    }
  }
  delete aNode;
  return NS_OK;
}

nsresult
UndisplayedMap::RemoveNodesFor(nsIContent* aParentContent)
{
  PLHashEntry** entry = GetEntryFor(aParentContent);
  NS_ASSERTION(entry, "content not in map");
  if (*entry) {
    UndisplayedNode*  node = (UndisplayedNode*)((*entry)->value);
    delete node;
    if (entry == mLastLookup) {
      mLastLookup = nsnull;
    }
    PL_HashTableRawRemove(mTable, entry, *entry);
  }
  return NS_OK;
}

static PRIntn
RemoveUndisplayedEntry(PLHashEntry* he, PRIntn i, void* arg)
{
  UndisplayedNode*  node = (UndisplayedNode*)(he->value);
  delete node;
  // Remove and free this entry and continue enumerating
  return HT_ENUMERATE_REMOVE | HT_ENUMERATE_NEXT;
}

void
UndisplayedMap::Clear(void)
{
  mLastLookup = nsnull;
  PL_HashTableEnumerateEntries(mTable, RemoveUndisplayedEntry, 0);
}

//----------------------------------------------------------------------
    
FrameManager::PropertyList::PropertyList(nsIAtom*             aName,
                                         NSFMPropertyDtorFunc aDtorFunc,
                                         nsDST::NodeArena*    aDSTNodeArena)
  : mName(aName), mFrameValueMap(new nsDST(aDSTNodeArena)),
    mDtorFunc(aDtorFunc), mNext(nsnull)
{
}

FrameManager::PropertyList::~PropertyList()
{
  delete mFrameValueMap;
}

class DestroyPropertyValuesFunctor : public nsDSTNodeFunctor {
public:
  DestroyPropertyValuesFunctor(nsIPresContext*      aPresContext,
                               nsIAtom*             aPropertyName,
                               NSFMPropertyDtorFunc aDtorFunc)
    : mPresContext(aPresContext), mPropertyName(aPropertyName), mDtorFunc(aDtorFunc) {}

  virtual void operator () (void *aKey, void *aValue) {
    mDtorFunc(mPresContext, (nsIFrame*)aKey, mPropertyName, aValue);
  }

  nsIPresContext*      mPresContext;
  nsIAtom*             mPropertyName;
  NSFMPropertyDtorFunc mDtorFunc;
};

void
FrameManager::PropertyList::RemoveAllProperties(nsIPresContext* aPresContext)
{
  if (mDtorFunc) {
    DestroyPropertyValuesFunctor  functor(aPresContext, mName, mDtorFunc);

    // Enumerate any remaining frame/value pairs and destroy the value object
    mFrameValueMap->Enumerate(functor);
  }
}

PRBool
FrameManager::PropertyList::RemovePropertyForFrame(nsIPresContext* aPresContext,
                                                   nsIFrame*       aFrame)
{
  void*     value;
  nsresult  result;
   
  // If the property exists, then we need to run the dtor function so
  // do a search with the option to remove the key/value pair
  result = mFrameValueMap->Search(aFrame, NS_DST_REMOVE_KEY_VALUE, &value);

  if (NS_OK == result) {
    // The property was set
    if (mDtorFunc) {
      // Destroy the property value
      mDtorFunc(aPresContext, aFrame, mName, value);
    }

    return PR_TRUE;
  }

  return PR_FALSE;
}

