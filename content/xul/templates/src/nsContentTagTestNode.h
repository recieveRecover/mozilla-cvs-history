/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chris Waterson <waterson@netscape.com>
 */

#ifndef nsContentTagTestNode_h__
#define nsContentTagTestNode_h__

#include "nsRuleNetwork.h"
#include "nsIAtom.h"

class nsConflictSet;

class nsContentTagTestNode : public TestNode
{
public:
    nsContentTagTestNode(InnerNode* aParent,
                         nsConflictSet& aConflictSet,
                         PRInt32 aContentVariable,
                         nsIAtom* aTag);

    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations, void* aClosure) const;

    virtual nsresult GetAncestorVariables(VariableSet& aVariables) const;

protected:
    nsConflictSet& mConflictSet;
    PRInt32 mContentVariable;
    nsCOMPtr<nsIAtom> mTag;
};

#endif // nsContentTagTestNode_h__
