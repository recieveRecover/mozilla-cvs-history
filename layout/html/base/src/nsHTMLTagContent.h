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
#ifndef nsHTMLTagContent_h___
#define nsHTMLTagContent_h___

#include "nsHTMLContent.h"
#include "nsHTMLValue.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIJSScriptObject.h"

class nsIHTMLAttributes;
class nsIPresContext;
class nsIStyleContext;

/** 
 * Base class for tagged html content objects, holds attributes.
 */
class nsHTMLTagContent : public nsHTMLContent, public nsIDOMHTMLElement, public nsIJSScriptObject {
public:

  // nsIContent
  virtual nsIAtom* GetTag() const;
  NS_IMETHOD SizeOf(nsISizeOfHandler* aHandler) const;

  // nsIHTMLContent
  /**
   * Translate the content object into it's source html format
   */
  virtual void ToHTMLString(nsString& out) const;

  /**
   * Translate the content object into the (XIF) XML Interchange Format
   * XIF is an intermediate form of the content model, the buffer
   * will then be parsed into any number of formats including HTML, TXT, etc.

   * BeginConvertToXIF -- opens a container and writes out the attributes
   * ConvertContentToXIF -- typically does nothing unless there is text content
   * FinishConvertToXIF -- closes a container


  */
  virtual void BeginConvertToXIF(nsXIFConverter& aConverter) const;
  virtual void ConvertContentToXIF(nsXIFConverter& aConverter) const;
  virtual void FinishConvertToXIF(nsXIFConverter& aConverter) const;

  
  /**
   * Generic implementation of SetAttribute that translates aName into
   * an uppercase'd atom and invokes SetAttribute(atom, aValue).
   */
  virtual void SetAttribute(const nsString& aName, const nsString& aValue);

  /**
   * Generic implementation of GetAttribute that knows how to map
   * nsHTMLValue's to string's. Note that it cannot map enumerated
   * values directly, but will use AttributeToString to supply a
   * conversion. Subclasses must override AttributeToString to map
   * enumerates to strings and return eContentAttr_HasValue when a
   * conversion is succesful. Subclasses should also override this
   * method if the default string conversions used don't apply
   * correctly to the given attribute.
   */
  virtual nsContentAttr GetAttribute(const nsString& aName,
                                     nsString& aResult) const;

  virtual nsContentAttr GetAttribute(nsIAtom *aAttribute,
                                     nsString &aResult) const;

  virtual void SetAttribute(nsIAtom* aAttribute, const nsString& aValue);

  virtual void SetAttribute(nsIAtom* aAttribute, const nsHTMLValue& aValue);
  virtual void UnsetAttribute(nsIAtom* aAttribute);

  /**
   * Subclasses must override to fill the value object for values that
   * are not stored in mAttributes. If the returned value is of an
   * enumerated type then the subclass must also override the
   * AttributeToString method (see the comment on the other GetAttribute
   * method)
   */
  virtual nsContentAttr GetAttribute(nsIAtom* aAttribute,
                                     nsHTMLValue& aValue) const;

  virtual PRInt32 GetAllAttributeNames(nsISupportsArray* aArray) const;
  virtual PRInt32 GetAttributeCount(void) const;

  virtual void      SetID(nsIAtom* aID);
  virtual nsIAtom*  GetID(void) const;
  virtual void      SetClass(nsIAtom* aClass);  // XXX this will have to change for CSS2
  virtual nsIAtom*  GetClass(void) const;  // XXX this will have to change for CSS2

  virtual nsIStyleRule* GetStyleRule(void);

  // Override from nsHTMLContent to allow setting of event handlers once
  // tag content is added to the doc tree.
  virtual void SetDocument(nsIDocument* aDocument);

  // nsIScriptObjectOwner interface
  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject);

  // nsIJSScriptObject interface
  virtual PRBool    AddProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    DeleteProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    GetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    SetProperty(JSContext *aContext, jsval aID, jsval *aVp);
  virtual PRBool    EnumerateProperty(JSContext *aContext);
  virtual PRBool    Resolve(JSContext *aContext, jsval aID);
  virtual PRBool    Convert(JSContext *aContext, jsval aID);
  virtual void      Finalize(JSContext *aContext);
  
  // nsISupports interface
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  // nsIDOMNode interface
  NS_DECL_IDOMNODE
  
  // nsIDOMElement interface
  NS_DECL_IDOMELEMENT

  // nsIDOMHTMLElement interface
  NS_DECL_IDOMHTMLELEMENT

  // nsIDOMEventReceiver interface
  NS_IMETHOD HandleDOMEvent(nsIPresContext& aPresContext, 
                            nsEvent* aEvent, 
                            nsIDOMEvent** aDOMEvent,
                            PRUint32 aFlags,
                            nsEventStatus& aEventStatus);

  // Utility routines for making attribute parsing easier

  struct EnumTable {
    const char* tag;
    PRInt32 value;
  };

  static PRBool ParseEnumValue(const nsString& aValue,
                               EnumTable* aTable,
                               nsHTMLValue& aResult);

  static PRBool EnumValueToString(const nsHTMLValue& aValue,
                                  EnumTable* aTable,
                                  nsString& aResult);

  static PRBool ParseValueOrPercent(const nsString& aString,
                                    nsHTMLValue& aResult, nsHTMLUnit aValueUnit);

 /** used to parser attribute values that could be either:
   *   integer         (n), 
   *   percent         (n%),
   *   or proportional (n*)
   */
  static void ParseValueOrPercentOrProportional(const nsString& aString,
    																										          nsHTMLValue& aResult, 
																												          nsHTMLUnit aValueUnit);

  static PRBool ValueOrPercentToString(const nsHTMLValue& aValue,
                                       nsString& aResult);

  static PRBool ParseValue(const nsString& aString, PRInt32 aMin,
                           nsHTMLValue& aResult, nsHTMLUnit aValueUnit);

  static PRBool ParseValue(const nsString& aString, PRInt32 aMin, PRInt32 aMax,
                           nsHTMLValue& aResult, nsHTMLUnit aValueUnit);

  static PRBool ParseColor(const nsString& aString, nsHTMLValue& aResult);

  static PRBool ColorToString(const nsHTMLValue& aValue,
                              nsString& aResult);

  /**
   * Parse width, height, border, hspace, vspace. Returns PR_TRUE if
   * the aAttribute is one of the listed attributes.
   */
  static PRBool ParseImageProperty(nsIAtom* aAttribute,
                                   const nsString& aString,
                                   nsHTMLValue& aResult);

  static PRBool ImagePropertyToString(nsIAtom* aAttribute,
                                      const nsHTMLValue& aValue,
                                      nsString& aResult);

  void MapImagePropertiesInto(nsIStyleContext* aContext,
                              nsIPresContext* aPresContext);

  void MapImageBorderInto(nsIStyleContext* aContext,
                          nsIPresContext* aPresContext,
                          nscolor aBorderColors[4]);

  static PRBool ParseAlignParam(const nsString& aString, nsHTMLValue& aResult);

  static PRBool AlignParamToString(const nsHTMLValue& aValue,
                                   nsString& aResult);

  static PRBool ParseDivAlignParam(const nsString& aString, nsHTMLValue& aRes);

  static PRBool DivAlignParamToString(const nsHTMLValue& aValue,
                                   nsString& aResult);

  /** HTML4 table align attributes 
    */
  static PRBool ParseTableAlignParam(const nsString& aString, nsHTMLValue& aRes);

  /** HTML4 table align attributes 
    */
  static PRBool TableAlignParamToString(const nsHTMLValue& aValue,
                                        nsString& aResult);
  
  /** HTML4 table caption align attributes 
    */
  static PRBool ParseTableCaptionAlignParam(const nsString& aString, nsHTMLValue& aRes);

  /** HTML4 table caption align attributes 
    */
  static PRBool TableCaptionAlignParamToString(const nsHTMLValue& aValue,
                                               nsString& aResult);

protected:
  nsHTMLTagContent();
  nsHTMLTagContent(nsIAtom* aTag);
  virtual ~nsHTMLTagContent();
  void SizeOfWithoutThis(nsISizeOfHandler* aHandler) const;

  /**
   * Helper method used by GetAttribute to map an attribute
   * to a string value.
   */
  virtual nsContentAttr AttributeToString(nsIAtom* aAttribute,
                                          nsHTMLValue& aValue,
                                          nsString& aResult) const;

  
  virtual nsresult AddScriptEventListener(nsIAtom* aAttribute, nsHTMLValue& aValue, REFNSIID aIID);
  
  void TriggerLink(nsIPresContext& aPresContext,
                 const nsString& aBase,
                 const nsString& aURLSpec,
                 const nsString& aTargetSpec,
                 PRBool aClick);

  nsIAtom* mTag;
  nsIHTMLAttributes* mAttributes;
};

#endif /* nsHTMLTagContent_h___ */
