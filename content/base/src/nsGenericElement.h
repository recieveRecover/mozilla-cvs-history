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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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
#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIXMLContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNSEventTarget.h"
#include "nsILinkHandler.h"
#include "nsGenericDOMNodeList.h"
#include "nsIEventListenerManager.h"
#include "nsContentUtils.h"
#include "pldhash.h"
#include "nsAttrAndChildArray.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsISupportsArray;
class nsIDOMNamedNodeMap;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsDOMAttributeMap;
class nsIURI;
class nsVoidArray;
class nsINodeInfo;
class nsIControllers;
class nsIDOMNSFeatureFactory;

typedef unsigned long PtrBits;

/**
 * This bit will be set if the nsGenericElement doesn't have nsDOMSlots
 */
#define GENERIC_ELEMENT_DOESNT_HAVE_DOMSLOTS   0x00000001U

/**
 * This bit will be set if the element has a range list in the range list hash
 */
#define GENERIC_ELEMENT_HAS_RANGELIST          0x00000002U

/**
 * This bit will be set if the element has a listener manager in the listener
 * manager hash
 */
#define GENERIC_ELEMENT_HAS_LISTENERMANAGER    0x00000004U

/** Whether this content is anonymous */
#define GENERIC_ELEMENT_IS_ANONYMOUS           0x00000008U

/** Whether this content has had any properties set on it */
#define GENERIC_ELEMENT_HAS_PROPERTIES         0x00000010U

/** Whether this content may have a frame */
#define GENERIC_ELEMENT_MAY_HAVE_FRAME         0x00000020U

/** Three bits are element type specific. */
#define ELEMENT_TYPE_SPECIFIC_BITS_OFFSET      6


/**
 * Class that implements the nsIDOMNodeList interface (a list of children of
 * the content), by holding a reference to the content and delegating GetLength
 * and Item to its existing child list.
 * @see nsIDOMNodeList
 */
class nsChildContentList : public nsGenericDOMNodeList 
{
public:
  /**
   * @param aContent the content whose children will make up the list
   */
  nsChildContentList(nsIContent *aContent);
  virtual ~nsChildContentList();

  // nsIDOMNodeList interface
  NS_DECL_NSIDOMNODELIST
  
  /** Drop the reference to the content */
  void DropReference();

private:
  /** The content whose children make up the list (weak reference) */
  nsIContent *mContent;
};

/**
 * There are a set of DOM- and scripting-specific instance variables
 * that may only be instantiated when a content object is accessed
 * through the DOM. Rather than burn actual slots in the content
 * objects for each of these instance variables, we put them off
 * in a side structure that's only allocated when the content is
 * accessed through the DOM.
 */
class nsDOMSlots
{
public:
  nsDOMSlots(PtrBits aFlags);
  ~nsDOMSlots();

  PRBool IsEmpty();

  PtrBits mFlags;

  /**
   * An object implementing nsIDOMNodeList for this content (childNodes)
   * @see nsIDOMNodeList
   * @see nsGenericHTMLElement::GetChildNodes
   */
  nsRefPtr<nsChildContentList> mChildNodes;

  /**
   * The .style attribute (an interface that forwards to the actual
   * style rules)
   * @see nsGenericHTMLElement::GetStyle */
  nsRefPtr<nsDOMCSSDeclaration> mStyle;

  /**
   * An object implementing nsIDOMNamedNodeMap for this content (attributes)
   * @see nsGenericElement::GetAttributes
   */
  nsRefPtr<nsDOMAttributeMap> mAttributeMap;

  union {
    /**
    * The nearest enclosing content node with a binding that created us.
    * @see nsGenericElement::GetBindingParent
    */
    nsIContent* mBindingParent;  // [Weak]

    /**
    * The controllers of the XUL Element.
    */
    nsIControllers* mControllers; // [OWNER]
  };
};

class RangeListMapEntry : public PLDHashEntryHdr
{
public:
  RangeListMapEntry(const void *aKey)
    : mKey(aKey), mRangeList(nsnull)
  {
  }

  ~RangeListMapEntry()
  {
    delete mRangeList;
  }

private:
  const void *mKey; // must be first to look like PLDHashEntryStub

public:
  // We want mRangeList to be an nsAutoVoidArray but we can't make an
  // nsAutoVoidArray a direct member of RangeListMapEntry since it
  // will be moved around in memory, and nsAutoVoidArray can't deal
  // with that.
  nsVoidArray *mRangeList;
};

class EventListenerManagerMapEntry : public PLDHashEntryHdr
{
public:
  EventListenerManagerMapEntry(const void *aKey)
    : mKey(aKey)
  {
  }

  ~EventListenerManagerMapEntry()
  {
    if (mListenerManager) {
      mListenerManager->SetListenerTarget(nsnull);
    }
  }

private:
  const void *mKey; // must be first, to look like PLDHashEntryStub

public:
  nsCOMPtr<nsIEventListenerManager> mListenerManager;
};


/**
 * A tearoff class for nsGenericElement to implement the nsIDOM3Node functions
 */
class nsNode3Tearoff : public nsIDOM3Node
{
public:
  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOM3NODE

  nsNode3Tearoff(nsIContent *aContent) : mContent(aContent)
  {
  }

  static nsresult GetTextContent(nsIContent *aContent,
                                 nsAString &aTextContent);

  static nsresult SetTextContent(nsIContent *aContent,
                                 const nsAString &aTextContent);

protected:
  virtual ~nsNode3Tearoff() {};

private:
  nsCOMPtr<nsIContent> mContent;
};


#define NS_EVENT_TEAROFF_CACHE_SIZE 4

/**
 * nsDOMEventRTTearoff is a tearoff class used by nsGenericElement and
 * nsGenericDOMDataNode classes for implementing the interfaces
 * nsIDOMEventReceiver and nsIDOMEventTarget
 *
 * Use the method nsDOMEventRTTearoff::Create() to create one of these babies.
 * @see nsDOMEventRTTearoff::Create
 */

class nsDOMEventRTTearoff : public nsIDOMEventReceiver,
                            public nsIDOM3EventTarget,
                            public nsIDOMNSEventTarget
{
private:
  // This class uses a caching scheme so we don't let users of this
  // class create new instances with 'new', in stead the callers
  // should use the static method
  // nsDOMEventRTTearoff::Create(). That's why the constructor and
  // destrucor of this class is private.

  nsDOMEventRTTearoff(nsIContent *aContent);

  static nsDOMEventRTTearoff *mCachedEventTearoff[NS_EVENT_TEAROFF_CACHE_SIZE];
  static PRUint32 mCachedEventTearoffCount;

  /**
   * This method gets called by Release() when it's time to delete the
   * this object, in stead of always deleting the object we'll put the
   * object in the cache if unless the cache is already full.
   */
  void LastRelease();

  nsresult GetEventReceiver(nsIDOMEventReceiver **aReceiver);
  nsresult GetDOM3EventTarget(nsIDOM3EventTarget **aTarget);

public:
  virtual ~nsDOMEventRTTearoff();

  /**
   * Use this static method to create instances of nsDOMEventRTTearoff.
   * @param aContent the content to create a tearoff for
   */
  static nsDOMEventRTTearoff *Create(nsIContent *aContent);

  /**
   * Call before shutdown to clear the cache and free memory for this class.
   */
  static void Shutdown();

  // nsISupports
  NS_DECL_ISUPPORTS

  // nsIDOMEventTarget
  NS_DECL_NSIDOMEVENTTARGET

  // nsIDOM3EventTarget
  NS_DECL_NSIDOM3EVENTTARGET

  // nsIDOMEventReceiver
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID);
  NS_IMETHOD GetListenerManager(nsIEventListenerManager** aResult);
  NS_IMETHOD HandleEvent(nsIDOMEvent *aEvent);
  NS_IMETHOD GetSystemEventGroup(nsIDOMEventGroup** aGroup);

  // nsIDOMNSEventTarget
  NS_DECL_NSIDOMNSEVENTTARGET

private:
  /**
   * Strong reference back to the content object from where an instance of this
   * class was 'torn off'
   */
  nsCOMPtr<nsIContent> mContent;
};


/**
 * A generic base class for DOM elements, implementing many nsIContent,
 * nsIDOMNode and nsIDOMElement methods.
 */
class nsGenericElement : public nsIXMLContent
{
public:
  nsGenericElement(nsINodeInfo *aNodeInfo);
  virtual ~nsGenericElement();

  NS_DECL_ISUPPORTS

  /**
   * Called during QueryInterface to give the binding manager a chance to
   * get an interface for this element.
   */
  nsresult PostQueryInterface(REFNSIID aIID, void** aInstancePtr);

  /** Free globals, to be called from module destructor */
  static void Shutdown();

  // nsIContent interface methods
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual PRBool IsNativeAnonymous() const;
  virtual void SetNativeAnonymous(PRBool aAnonymous);
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual PRInt32 IndexOf(nsIContent* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult AppendChildTo(nsIContent* aKid, PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
  virtual nsIAtom *GetIDAttributeName() const;
  virtual nsIAtom *GetClassAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, PRBool aNotify);
  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  virtual nsresult GetAttrNameAt(PRUint32 aIndex, PRInt32* aNameSpaceID,
                                 nsIAtom** aName, nsIAtom** aPrefix) const;
  virtual PRUint32 GetAttrCount() const;
  virtual nsresult RangeAdd(nsIDOMRange* aRange);
  virtual void RangeRemove(nsIDOMRange* aRange);
  virtual const nsVoidArray *GetRangeList() const;
  virtual nsresult HandleDOMEvent(nsPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus* aEventStatus);
  virtual void SetFocus(nsPresContext* aContext);
  virtual nsIContent *GetBindingParent() const;
  virtual PRBool IsContentOfType(PRUint32 aFlags) const;
  virtual nsresult GetListenerManager(nsIEventListenerManager** aResult);
  virtual already_AddRefed<nsIURI> GetBaseURI() const;
  virtual void* GetProperty(nsIAtom  *aPropertyName,
                            nsresult *aStatus = nsnull) const;
  virtual nsresult SetProperty(nsIAtom            *aPropertyName,
                               void               *aValue,
                               NSPropertyDtorFunc  aDtor);
  virtual nsresult DeleteProperty(nsIAtom  *aPropertyName);
  virtual void*    UnsetProperty(nsIAtom *aPropertyName,
                                 nsresult *aStatus = nsnull);
  virtual void SetMayHaveFrame(PRBool aMayHaveFrame);
  virtual PRBool MayHaveFrame() const;
  void SetHasProperties();

  /**
   * This calls Clone to do the actual cloning so that we end up with the
   * right class for the clone.
   */
  nsresult CloneContent(nsNodeInfoManager *aNodeInfoManager, PRBool aDeep,
                        nsIContent **aResult) const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const;
  virtual void DumpContent(FILE* out, PRInt32 aIndent,PRBool aDumpAll) const;
#endif

  virtual nsIAtom* GetID() const;
  virtual const nsAttrValue* GetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);
  NS_IMETHOD_(PRBool)
    IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute, 
                                              PRInt32 aModType) const;
  /*
   * Attribute Mapping Helpers
   */
  struct MappedAttributeEntry {
    nsIAtom** attribute;
  };
  
  /**
   * A common method where you can just pass in a list of maps to check
   * for attribute dependence. Most implementations of
   * IsAttributeMapped should use this function as a default
   * handler.
   */
  static PRBool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const aMaps[],
                          PRUint32 aMapCount);

  // nsIXMLContent interface methods
  NS_IMETHOD MaybeTriggerAutoLink(nsIDocShell *aShell);

  // nsIDOMNode method implementation
  NS_IMETHOD GetNodeName(nsAString& aNodeName);
  NS_IMETHOD GetLocalName(nsAString& aLocalName);
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue);
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode);
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling);
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling);
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument);
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI);
  NS_IMETHOD GetPrefix(nsAString& aPrefix);
  NS_IMETHOD SetPrefix(const nsAString& aPrefix);
  NS_IMETHOD Normalize();
  NS_IMETHOD IsSupported(const nsAString& aFeature,
                         const nsAString& aVersion, PRBool* aReturn);
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes);
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes);
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes);
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild);
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild);
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    return InsertBefore(aNewChild, nsnull, aReturn);
  }

  // nsIDOMElement method implementation
  NS_IMETHOD GetTagName(nsAString& aTagName);
  NS_IMETHOD GetAttribute(const nsAString& aName,
                          nsAString& aReturn);
  NS_IMETHOD SetAttribute(const nsAString& aName,
                          const nsAString& aValue);
  NS_IMETHOD RemoveAttribute(const nsAString& aName);
  NS_IMETHOD GetAttributeNode(const nsAString& aName,
                              nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagName(const nsAString& aTagname,
                                  nsIDOMNodeList** aReturn);
  NS_IMETHOD GetAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aLocalName,
                            nsAString& aReturn);
  NS_IMETHOD SetAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            const nsAString& aValue);
  NS_IMETHOD RemoveAttributeNS(const nsAString& aNamespaceURI,
                               const nsAString& aLocalName);
  NS_IMETHOD GetAttributeNodeNS(const nsAString& aNamespaceURI,
                                const nsAString& aLocalName,
                                nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                    const nsAString& aLocalName,
                                    nsIDOMNodeList** aReturn);
  NS_IMETHOD HasAttribute(const nsAString& aName, PRBool* aReturn);
  NS_IMETHOD HasAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aLocalName,
                            PRBool* aReturn);

  //----------------------------------------

  /**
   * Add a script event listener with the given event handler name
   * (like onclick) and with the value as JS   
   * @param aEventName the event listener name
   * @param aValue the JS to attach
   */
  nsresult AddScriptEventListener(nsIAtom* aEventName,
                                  const nsAString& aValue);

  /**
   * Trigger a link with uri aLinkURI.  If aClick is false, this triggers a
   * mouseover on the link, otherwise it triggers a load, after doing a
   * security check.
   * @param aPresContext the pres context.
   * @param aVerb how the link will be loaded (replace page, new window, etc.)
   * @param aOriginURI the URI the request originates from.  Used as the origin
   *        uri for a CheckLoadURI call. 
   * @param aLinkURI the URI of the link
   * @param aTargetSpec the target (like target=, may be empty)
   * @param aClick whether this was a click or not (if false, it assumes you
   *        just hovered over the link)
   * @param aIsUserTriggered whether the user triggered the link.
   *        This would be false for loads from auto XLinks or from the
   *        click() method if we ever implement it.
   */
  nsresult TriggerLink(nsPresContext* aPresContext,
                       nsLinkVerb aVerb,
                       nsIURI* aOriginURI,
                       nsIURI* aLinkURI,
                       const nsAFlatString& aTargetSpec,
                       PRBool aClick,
                       PRBool aIsUserTriggered);
  /**
   * Do whatever needs to be done when the mouse leaves a link
   */
  nsresult LeaveLink(nsPresContext* aPresContext);

  /**
   * Take two text nodes and append the second to the first.
   * @param aFirst the node which will contain first + second [INOUT]
   * @param aSecond the node which will be appended
   */
  nsresult JoinTextNodes(nsIContent* aFirst,
                         nsIContent* aSecond);

  /**
   * Set .document in the immediate children of said content (but not in
   * content itself).  SetDocument() in the children will recursively call
   * this.
   *
   * @param aContent the content to get the children of
   * @param aDocument the document to set
   * @param aCompileEventHandlers whether to initialize the event handlers in
   *        the document (used by nsXULElement)
   */
  static void SetDocumentInChildrenOf(nsIContent* aContent, 
				      nsIDocument* aDocument, PRBool aCompileEventHandlers);

  /**
   * Check whether a spec feature/version is supported.
   * @param aObject the object, which should support the feature,
   *        for example nsIDOMNode or nsIDOMDOMImplementation
   * @param aFeature the feature ("Views", "Core", "HTML", "Range" ...)
   * @param aVersion the version ("1.0", "2.0", ...)
   * @param aReturn whether the feature is supported or not [OUT]
   */
  static nsresult InternalIsSupported(nsISupports* aObject,
                                      const nsAString& aFeature,
                                      const nsAString& aVersion,
                                      PRBool* aReturn);

  static nsresult InternalGetFeature(nsISupports* aObject,
                                     const nsAString& aFeature,
                                     const nsAString& aVersion,
                                     nsISupports** aReturn);
  
  static already_AddRefed<nsIDOMNSFeatureFactory>
    GetDOMFeatureFactory(const nsAString& aFeature, const nsAString& aVersion);

  /**
   * Quick helper to determine whether there are any mutation listeners
   * of a given type that apply to this content (are at or above it).
   * @param aContent the content to look for listeners for
   * @param aType the type of listener (NS_EVENT_BITS_MUTATION_*)
   * @return whether there are mutation listeners of the specified type for
   *         this content or not
   */
  static PRBool HasMutationListeners(nsIContent* aContent,
                                     PRUint32 aType);

  static PRBool ShouldFocus(nsIContent *aContent);

  /**
   * Checks if a node is the ancestor of another.
   */
  static PRBool isSelfOrAncestor(nsIContent *aNode,
                                 nsIContent *aPossibleAncestor);

  /**
   * Actual implementation of the DOM InsertBefore method.  Shared by
   * nsDocument.  When called from nsDocument, aParent will be null.
   *
   * @param aNewChild The child to insert
   * @param aRefChild The child to insert before
   * @param aParent The parent to use for the new child
   * @param aDocument The document to use for the new child.
   *                  Must be non-null, if aParent is null and must match
   *                  aParent->GetCurrentDoc() if aParent is not null.
   * @param aChildArray The child array to work with
   * @param aReturn [out] the child we insert
   */
  static nsresult doInsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                                 nsIContent* aParent, nsIDocument* aDocument,
                                 nsAttrAndChildArray& aChildArray,
                                 nsIDOMNode** aReturn);

  /**
   * Actual implementation of the DOM ReplaceChild method.  Shared by
   * nsDocument.  When called from nsDocument, aParent will be null.
   *
   * @param aNewChild The child to replace with
   * @param aOldChild The child to replace
   * @param aParent The parent to use for the new child
   * @param aDocument The document to use for the new child.
   *                  Must be non-null if aParent is null and must match
   *                  aParent->GetCurrentDoc() if aParent is not null.
   * @param aChildArray The child array to work with
   * @param aReturn [out] the child we insert
   */
  static nsresult doReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                                 nsIContent* aParent, nsIDocument* aDocument,
                                 nsAttrAndChildArray& aChildArray,
                                 nsIDOMNode** aReturn);

  static nsresult InitHashes();

  static PLDHashTable sEventListenerManagersHash;
  static PLDHashTable sRangeListsHash;

  /**
   * Struct that stores info on an attribute.  The name and value must
   * either both be null or both be non-null.
   */
  struct nsAttrInfo {
    nsAttrInfo(const nsAttrName* aName, const nsAttrValue* aValue) :
      mName(aName), mValue(aValue) {}
    nsAttrInfo(const nsAttrInfo& aOther) :
      mName(aOther.mName), mValue(aOther.mValue) {}
      
    const nsAttrName* mName;
    const nsAttrValue* mValue;
  };
  
protected:
  /**
   * Set attribute and (if needed) notify documentobservers and fire off
   * mutation events.
   *
   * @param aNamespaceID  namespace of attribute
   * @param aAttribute    local-name of attribute
   * @param aPrefix       aPrefix of attribute
   * @param aOldValue     previous value of attribute. Only needed if
   *                      aFireMutation is true.
   * @param aParsedValue  parsed new value of attribute
   * @param aModification is this a attribute-modification or addition. Only
   *                      needed if aFireMutation or aNotify is true.
   * @param aFireMutation should mutation-events be fired?
   * @param aNotify       should we notify document-observers?
   */
  nsresult SetAttrAndNotify(PRInt32 aNamespaceID,
                            nsIAtom* aName,
                            nsIAtom* aPrefix,
                            const nsAString& aOldValue,
                            nsAttrValue& aParsedValue,
                            PRBool aModification,
                            PRBool aFireMutation,
                            PRBool aNotify);

  /**
   * Convert an attribute string value to attribute type based on the type of
   * attribute.  Called by SetAttr().  Note that at the moment we only do this
   * for attributes in the null namespace (kNameSpaceID_None).
   *
   * @param aNamespaceID the namespace of the attribute to convert
   * @param aAttribute the attribute to convert
   * @param aValue the string value to convert
   * @param aResult the nsAttrValue [OUT]
   * @return PR_TRUE if the parsing was successful, PR_FALSE otherwise
   */
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  /**
   * Try to set the attribute as a mapped attribute, if applicable.  This will
   * only be called for attributes that are in the null namespace and only on
   * attributes that returned true when passed to IsAttributeMapped.  The
   * caller will not try to set the attr in any other way if this method
   * returns PR_TRUE (the value of aRetval does not matter for that purpose).
   *
   * @param aDocument the current document of this node (an optimization)
   * @param aName the name of the attribute
   * @param aValue the nsAttrValue to set
   * @param [out] aRetval the nsresult status of the operation, if any.
   * @return PR_TRUE if the setting was attempted, PR_FALSE otherwise.
   */
  virtual PRBool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);

  /**
   * Hook that is called by nsGenericElement::SetAttr to allow subclasses to
   * deal with attribute sets.  This will only be called after we verify that
   * we're actually doing an attr set and will be called before ParseAttribute
   * and hence before we've set the new value.
   *
   * @param aNamespaceID the namespace of the attr being set
   * @param aName the localname of the attribute being set
   * @param aValue the value it's being set to.  If null, the attr is being
   *        removed.
   * // XXXbz we don't actually call this method when we're removing attrs yet.
   *          But we will eventually.
   * @param aNotify Whether we plan to notify document observers.
   */
  // Note that this is inlined so that when subclasses call it it gets
  // inlined.  Those calls don't go through a vtable.
  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify)
  {
    return NS_OK;
  }

  /**
   * Hook that is called by nsGenericElement::SetAttr to allow subclasses to
   * deal with attribute sets.  This will only be called after we have called
   * SetAndTakeAttr (that is, after we have actually set the attr).
   *
   * @param aNamespaceID the namespace of the attr being set
   * @param aName the localname of the attribute being set
   * @param aValue the value it's being set to.  If null, the attr is being
   *        removed.
   * // XXXbz we don't actually call this method when we're removing attrs yet.
   *          But we will eventually.
   * @param aNotify Whether we plan to notify document observers.
   */
  // Note that this is inlined so that when subclasses call it it gets
  // inlined.  Those calls don't go through a vtable.
  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify)
  {
    return NS_OK;
  }

  /**
   * Hook to allow subclasses to produce a different nsIEventListenerManager if
   * needed for attachment of attribute-defined handlers
   */
  virtual nsresult
    GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                   nsISupports** aTarget,
                                   PRBool* aDefer);

  /**
   * Get the attr info for the given namespace ID and attribute name.  The
   * namespace ID must not be kNameSpaceID_Unknown and the name must not be
   * null.  Note that this can only return info on attributes that actually
   * live on this element (and is only virtual to handle XUL prototypes).  That
   * is, this should only be called from methods that only care about attrs
   * that effectively live in mAttrsAndChildren.
   */
  virtual nsAttrInfo GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const;

  /**
   * Duplicate children by calling importNode and append them to another
   * element.
   *
   * @param aDst the element to append the imported children to
   * @param aImportDocument the document to use to call importNode
   */
  nsresult ImportChildrenTo(nsGenericElement *aDst,
                            nsIDOMDocument *aImportDocument) const;

  /**
   * Clone children and append them to another element.
   *
   * @param aDst the element to append the imported children to
   */
  nsresult CloneChildrenTo(nsGenericElement *aDst) const;

  /**
   * Copy attributes and children to another content object
   * @param aDest the object to copy to
   * @param aDeep whether to copy children
   */
  nsresult CopyInnerTo(nsGenericElement* aDest, PRBool aDeep) const;

  /**
   * Internal hook for converting an attribute name-string to an atomized name
   */
  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  /**
   * Will be called when InsertChildAt, AppendChildTo or RemoveChildAt is used,
   * before anything else is done.
   * The processing of the calling method will be cancelled if this method
   * does not succeed.
   * @param aKid    the content object which will be added or removed. Never null.
   * @param aIndex  the same index which is used as a parameter with InsertChildAt;
   *                aIndex gets value from GetChildCount() when AppendChildTo is
   *                called. When a child will be removed, this is the index of
   *                the aKid.
   * @param aRemove PR_TRUE, if a child object is being removed.
   */
  virtual nsresult WillAddOrRemoveChild(nsIContent* aKid,
                                        PRUint32 aIndex,
                                        PRBool aRemove);

  PRBool HasDOMSlots() const
  {
    return !(mFlagsOrSlots & GENERIC_ELEMENT_DOESNT_HAVE_DOMSLOTS);
  }

  nsDOMSlots *GetDOMSlots()
  {
    if (!HasDOMSlots()) {
      nsDOMSlots *slots = new nsDOMSlots(mFlagsOrSlots);

      if (!slots) {
        return nsnull;
      }

      mFlagsOrSlots = NS_REINTERPRET_CAST(PtrBits, slots);
    }

    return NS_REINTERPRET_CAST(nsDOMSlots *, mFlagsOrSlots);
  }

  nsDOMSlots *GetExistingDOMSlots() const
  {
    if (!HasDOMSlots()) {
      return nsnull;
    }

    return NS_REINTERPRET_CAST(nsDOMSlots *, mFlagsOrSlots);
  }

  PtrBits GetFlags() const
  {
    if (HasDOMSlots()) {
      return NS_REINTERPRET_CAST(nsDOMSlots *, mFlagsOrSlots)->mFlags;
    }

    return mFlagsOrSlots;
  }

  void SetFlags(PtrBits aFlagsToSet)
  {
    nsDOMSlots *slots = GetExistingDOMSlots();

    if (slots) {
      slots->mFlags |= aFlagsToSet;

      return;
    }

    mFlagsOrSlots |= aFlagsToSet;
  }

  void UnsetFlags(PtrBits aFlagsToUnset)
  {
    nsDOMSlots *slots = GetExistingDOMSlots();

    if (slots) {
      slots->mFlags &= ~aFlagsToUnset;

      return;
    }

    mFlagsOrSlots &= ~aFlagsToUnset;
  }

  PRBool HasRangeList() const
  {
    PtrBits flags = GetFlags();

    return (flags & GENERIC_ELEMENT_HAS_RANGELIST && sRangeListsHash.ops);
  }

  PRBool HasEventListenerManager() const
  {
    PtrBits flags = GetFlags();

    return (flags & GENERIC_ELEMENT_HAS_LISTENERMANAGER &&
            sEventListenerManagersHash.ops);
  }

  PRBool HasProperties() const
  {
    PtrBits flags = GetFlags();

    return (flags & GENERIC_ELEMENT_HAS_PROPERTIES) != 0;
  }

  /**
   * GetContentsAsText will take all the textnodes that are children
   * of |this| and concatenate the text in them into aText.  It
   * completely ignores any non-text-node children of |this|; in
   * particular it does not descend into any children of |this| that
   * happen to be container elements.
   *
   * @param aText the resulting text [OUT]
   */
  void GetContentsAsText(nsAString& aText);

  /**
   * Method to clone this element. This needs to be overriden by all element
   * classes. aNodeInfo should be identical to this element's nodeInfo, except
   * for the document which may be different. If aDeep is true all descendants
   * will be cloned too (by calling the DOM method cloneNode on them if the
   * ownerDocument of aNodeInfo is the same as the ownerDocument of this
   * element or by calling the DOM method importNode if they differ).
   *
   * @param aNodeInfo the nodeinfo to use for the clone
   * @param aDeep if true all descendants will be cloned too (attributes are
   *              always cloned)
   * @param aResult the clone
   */
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, PRBool aDeep,
                         nsIContent **aResult) const = 0;

  /**
   * A basic implementation of the DOM cloneNode method. Calls CloneContent to
   * do the actual cloning of the element.
   *
   * @param aDeep if true all descendants will be cloned too (attributes on the
   *              element are always cloned)
   * @param aSource nsIDOMNode pointer to this node
   * @param aResult the clone
   */
  nsresult CloneNode(PRBool aDeep, nsIDOMNode *aSource,
                     nsIDOMNode **aResult) const;

  /**
   * Used for either storing flags for this element or a pointer to
   * this elements nsDOMSlots. See the definition of the
   * GENERIC_ELEMENT_* macros for the layout of the bits in this
   * member.
   */
  PtrBits mFlagsOrSlots;

  /**
   * Array containing all attributes and children for this element
   */
  nsAttrAndChildArray mAttrsAndChildren;
};

// Internal non-public interface

// IID for the nsIDocumentFragment interface
#define NS_IDOCUMENTFRAGMENT_IID      \
{ 0xd8fb2853, 0xf6d6, 0x4499, \
  {0x9c, 0x60, 0x6c, 0xa2, 0x75, 0x35, 0x09, 0xeb} }

// nsIDocumentFragment interface
/**
 * These methods are supposed to be used when *all* children of a
 * document fragment are moved at once into a new parent w/o
 * changing the relationship between the children. If the moving
 * operation fails and some children were moved to a new parent and
 * some weren't, ReconnectChildren() should be called to remove the
 * children from their possible new parent and re-insert the
 * children into the document fragment. Once the operation is
 * complete and all children are successfully moved into their new
 * parent DropChildReferences() should be called so that the
 * document fragment will loose its references to the children.
 */
class nsIDocumentFragment : public nsIDOMDocumentFragment
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENTFRAGMENT_IID)

  /** Tell the children their parent is gone */
  NS_IMETHOD DisconnectChildren() = 0;
  /** Put all children back in the fragment */
  NS_IMETHOD ReconnectChildren() = 0;
  /** Drop references to children */
  NS_IMETHOD DropChildReferences() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocumentFragment, NS_IDOCUMENTFRAGMENT_IID)

#define NS_FORWARD_NSIDOMNODE_NO_CLONENODE(_to)                               \
  NS_IMETHOD GetNodeName(nsAString& aNodeName) {                              \
    return _to GetNodeName(aNodeName);                                        \
  }                                                                           \
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue) {                            \
    return _to GetNodeValue(aNodeValue);                                      \
  }                                                                           \
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue) {                      \
    return _to SetNodeValue(aNodeValue);                                      \
  }                                                                           \
  NS_IMETHOD GetNodeType(PRUint16* aNodeType) {                               \
    return _to GetNodeType(aNodeType);                                        \
  }                                                                           \
  NS_IMETHOD GetParentNode(nsIDOMNode** aParentNode) {                        \
    return _to GetParentNode(aParentNode);                                    \
  }                                                                           \
  NS_IMETHOD GetChildNodes(nsIDOMNodeList** aChildNodes) {                    \
    return _to GetChildNodes(aChildNodes);                                    \
  }                                                                           \
  NS_IMETHOD GetFirstChild(nsIDOMNode** aFirstChild) {                        \
    return _to GetFirstChild(aFirstChild);                                    \
  }                                                                           \
  NS_IMETHOD GetLastChild(nsIDOMNode** aLastChild) {                          \
    return _to GetLastChild(aLastChild);                                      \
  }                                                                           \
  NS_IMETHOD GetPreviousSibling(nsIDOMNode** aPreviousSibling) {              \
    return _to GetPreviousSibling(aPreviousSibling);                          \
  }                                                                           \
  NS_IMETHOD GetNextSibling(nsIDOMNode** aNextSibling) {                      \
    return _to GetNextSibling(aNextSibling);                                  \
  }                                                                           \
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes) {                \
    return _to GetAttributes(aAttributes);                                    \
  }                                                                           \
  NS_IMETHOD GetOwnerDocument(nsIDOMDocument** aOwnerDocument) {              \
    return _to GetOwnerDocument(aOwnerDocument);                              \
  }                                                                           \
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI) {                      \
    return _to GetNamespaceURI(aNamespaceURI);                                \
  }                                                                           \
  NS_IMETHOD GetPrefix(nsAString& aPrefix) {                                  \
    return _to GetPrefix(aPrefix);                                            \
  }                                                                           \
  NS_IMETHOD SetPrefix(const nsAString& aPrefix) {                            \
    return _to SetPrefix(aPrefix);                                            \
  }                                                                           \
  NS_IMETHOD GetLocalName(nsAString& aLocalName) {                            \
    return _to GetLocalName(aLocalName);                                      \
  }                                                                           \
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,       \
                          nsIDOMNode** aReturn) {                             \
    return _to InsertBefore(aNewChild, aRefChild, aReturn);                   \
  }                                                                           \
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,       \
                          nsIDOMNode** aReturn) {                             \
    return _to ReplaceChild(aNewChild, aOldChild, aReturn);                   \
  }                                                                           \
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn) {       \
    return _to RemoveChild(aOldChild, aReturn);                               \
  }                                                                           \
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn) {       \
    return _to AppendChild(aNewChild, aReturn);                               \
  }                                                                           \
  NS_IMETHOD HasChildNodes(PRBool* aReturn) {                                 \
    return _to HasChildNodes(aReturn);                                        \
  }                                                                           \
  NS_IMETHOD Normalize() {                                                    \
    return _to Normalize();                                                   \
  }                                                                           \
  NS_IMETHOD IsSupported(const nsAString& aFeature,                           \
                         const nsAString& aVersion, PRBool* aReturn) {        \
    return _to IsSupported(aFeature, aVersion, aReturn);                      \
  }                                                                           \
  NS_IMETHOD HasAttributes(PRBool* aReturn) {                                 \
    return _to HasAttributes(aReturn);                                        \
  }                                                                           \
  NS_IMETHOD CloneNode(PRBool aDeep, nsIDOMNode** aReturn);                   \
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, PRBool aDeep,                \
                         nsIContent **aResult) const;


/**
 * Macros to implement CloneNode(). _elementName is the class for which to
 * implement CloneNode, _implClass is the class to use to cast to
 * nsIDOMNode*.
 */
#define NS_IMPL_DOM_CLONENODE_AMBIGUOUS(_elementName, _implClass)           \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, PRBool aDeep,                   \
                    nsIContent **aResult) const                             \
{                                                                           \
  *aResult = nsnull;                                                        \
                                                                            \
  _elementName *it = new _elementName(aNodeInfo);                           \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsIContent> kungFuDeathGrip = it;                                \
  nsresult rv = CopyInnerTo(it, aDeep);                                     \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}                                                                           \
NS_IMETHODIMP                                                               \
_elementName::CloneNode(PRBool aDeep, nsIDOMNode **aResult)                 \
{                                                                           \
  return nsGenericElement::CloneNode(aDeep,                                 \
                                     NS_STATIC_CAST(_implClass*, this),     \
                                     aResult);                              \
}

#define NS_IMPL_DOM_CLONENODE(_elementName)                                 \
NS_IMPL_DOM_CLONENODE_AMBIGUOUS(_elementName, _elementName)

#define NS_IMPL_DOM_CLONENODE_WITH_INIT(_elementName)                       \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, PRBool aDeep,                   \
                    nsIContent **aResult) const                             \
{                                                                           \
  *aResult = nsnull;                                                        \
                                                                            \
  _elementName *it = new _elementName(aNodeInfo);                           \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsIContent> kungFuDeathGrip = it;                                \
  nsresult rv = it->Init();                                                 \
  rv |= CopyInnerTo(it, aDeep);                                             \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}                                                                           \
NS_IMETHODIMP                                                               \
_elementName::CloneNode(PRBool aDeep, nsIDOMNode **aResult)                 \
{                                                                           \
  return nsGenericElement::CloneNode(aDeep, this, aResult);                 \
}

#endif /* nsGenericElement_h___ */
