/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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


#ifndef nsRDFTreeDataModelItem_h__
#define nsRDFTreeDataModelItem_h__

#include "rdf.h"
#include "nsRDFDataModelItem.h"
#include "nsITreeDMItem.h"

class nsRDFTreeDataModel;

////////////////////////////////////////////////////////////////////////

class nsRDFTreeDataModelItem : public nsITreeDMItem, public nsRDFDataModelItem
{
public:
    nsRDFTreeDataModelItem(nsRDFTreeDataModel& tree, RDF_Resource resource);
    virtual ~nsRDFTreeDataModelItem(void);

    ////////////////////////////////////////////////////////////////////////
    // nsISupports interface

    NS_IMETHOD_(nsrefcnt) AddRef(void);
    NS_IMETHOD_(nsrefcnt) Release(void);
    NS_IMETHOD QueryInterface(const nsIID& iid, void** result);

#if 0
    ////////////////////////////////////////////////////////////////////////
    // nsIDMItem interface

    // Inspectors
    NS_IMETHOD GetIconImage(nsIImage*& pImage, nsIImageGroup* pGroup) const;
    NS_IMETHOD GetOpenState(PRBool& answer) const;

    // Methods for iterating over children.
    NS_IMETHOD GetChildCount(PRUint32& count) const;
    NS_IMETHOD GetNthChild(nsIDMItem*& pItem, PRUint32 item) const;

    // Parent access
    NS_IMETHOD GetParent(nsIDMItem*& pItem) const;

    // Setters

    // Methods to query the data model for a specific item displayed within the widget.
    NS_IMETHOD GetStringPropertyValue(nsString& value, const nsString& itemProperty) const;
    NS_IMETHOD GetIntPropertyValue(PRInt32& value, const nsString& itemProperty) const;
#endif

    ////////////////////////////////////////////////////////////////////////
    // nsITreeItem interface

    // Inspectors
    NS_IMETHOD GetTriggerImage(nsIImage*& pImage, nsIImageGroup* pGroup) const;
    NS_IMETHOD GetIndentationLevel(PRUint32& indentation) const;
	
    // Setters



private:
    mutable PRUint32   mCachedIndentationLevel;

    static const PRUint32 kInvalidIndentationLevel;
};

////////////////////////////////////////////////////////////////////////


#endif // nsRDFTreeDataModelItem_h__
