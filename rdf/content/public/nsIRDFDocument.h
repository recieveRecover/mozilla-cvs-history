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

/*

  An RDF-specific extension to nsIXMLDocument. Includes methods for
  setting the root resource of the document content model, a factory
  method for constructing the children of a node, etc.

  XXX This should really be called nsIXULDocument.

 */

#ifndef nsIRDFDocument_h___
#define nsIRDFDocument_h___

class nsIContent; // XXX nsIXMLDocument.h is bad and doesn't declare this class...

#include "nsIXMLDocument.h"

class nsIAtom;
class nsIRDFCompositeDataSource;
class nsIRDFContent;
class nsIRDFContentModelBuilder;
class nsISupportsArray;
class nsIRDFResource;
class nsIDOMHTMLFormElement;

// {954F0811-81DC-11d2-B52A-000000000000}
#define NS_IRDFDOCUMENT_IID \
{ 0x954f0811, 0x81dc, 0x11d2, { 0xb5, 0x2a, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }

/**
 * RDF document extensions to nsIDocument
 */

class nsIRDFDataSource;

class nsIRDFDocument : public nsIXMLDocument
{
public:
  static const nsIID& GetIID() { static nsIID iid = NS_IRDFDOCUMENT_IID; return iid; }

  /**
   * Set the document's "root" resource.
   */
  NS_IMETHOD SetRootResource(nsIRDFResource* aResource) = 0;

  NS_IMETHOD SplitProperty(nsIRDFResource* aResource,
                           PRInt32* aNameSpaceID,
                           nsIAtom** aTag) = 0;

  // The resource-to-element map is a one-to-many mapping of RDF
  // resources to content elements.

  /**
   * Add an entry to the resource-to-element map.
   */
  NS_IMETHOD AddElementForResource(nsIRDFResource* aResource, nsIContent* aElement) = 0;

  /**
   * Remove an entry from the resource-to-element map.
   */
  NS_IMETHOD RemoveElementForResource(nsIRDFResource* aResource, nsIContent* aElement) = 0;

  /**
   * Get the elements for a particular resource in the resource-to-element
   * map. The nsISupportsArray will be truncated and filled in with
   * nsIContent pointers.
   */
  NS_IMETHOD GetElementsForResource(nsIRDFResource* aResource, nsISupportsArray* aElements) = 0;

  /**
   * Create the contents for an element.
   */
  NS_IMETHOD CreateContents(nsIContent* aElement) = 0;

  /**
   * Add a content model builder to the document.
   */
  NS_IMETHOD AddContentModelBuilder(nsIRDFContentModelBuilder* aBuilder) = 0;

  /**
   * Get the RDF datasource that represents the document.
   */
  NS_IMETHOD GetDocumentDataSource(nsIRDFDataSource** aDatasource) = 0;

  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm) = 0;
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm) = 0;
};

// factory functions
nsresult NS_NewXULDocument(nsIRDFDocument** result);

#endif // nsIRDFDocument_h___
