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
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */
#include "nsMsgRDFUtils.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIServiceManager.h"
#include "prprf.h"
#include "nsCOMPtr.h"
#include "nsIAllocator.h"

 
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);

PRBool
peq(nsIRDFResource* r1, nsIRDFResource* r2)
{
  PRBool result;
  if (NS_SUCCEEDED(r1->EqualsResource(r2, &result)) && result) {
    return PR_TRUE;
  } else {
    return PR_FALSE;
  }
}

static PRBool
peqWithParameter(nsIRDFResource *r1, nsIRDFResource *r2, PRBool *isParameter, const char *parameter)
{
	char *r1Str, *r2Str;
	nsString r1nsStr, r2nsStr, r1nsParameterStr;

	r1->GetValue(&r1Str);
	r2->GetValue(&r2Str);

	r1nsStr = r1Str;
	r2nsStr = r2Str;
	r1nsParameterStr = r1Str;

	nsAllocator::Free(r1Str);
	nsAllocator::Free(r2Str);

	//probably need to not assume this will always come directly after property.
	r1nsParameterStr +=parameter;

	if(r1nsStr == r2nsStr)
	{
		*isParameter = PR_FALSE;
		return PR_TRUE;
	}
	else if(r1nsParameterStr == r2nsStr)
	{
		*isParameter = PR_TRUE;
		return PR_TRUE;
	}
  else
	{
		//In case the resources are equal but the values are different.  I'm not sure if this
		//could happen but it is feasible given interface.
		*isParameter = PR_FALSE;
		return(peq(r1, r2));
	}
}

PRBool
peqCollationSort(nsIRDFResource *r1, nsIRDFResource *r2, PRBool *isCollationSort)
{

	if(!isCollationSort)
		return PR_FALSE;

	return peqWithParameter(r1, r2, isCollationSort, "?collation=true");
}

PRBool
peqSort(nsIRDFResource* r1, nsIRDFResource* r2, PRBool *isSort)
{
	if(!isSort)
		return PR_FALSE;

	return peqWithParameter(r1, r2, isSort, "?sort=true");
}

nsresult createNode(nsString& str, nsIRDFNode **node)
{
	*node = nsnull;
	nsresult rv; 
	NS_WITH_SERVICE(nsIRDFService, rdf, kRDFServiceCID, &rv); 
	if (NS_FAILED(rv)) return rv;  
	nsCOMPtr<nsIRDFLiteral> value;
	rv = rdf->GetLiteral(str.GetUnicode(), getter_AddRefs(value));
	if(NS_SUCCEEDED(rv)) {
		*node = value;
		NS_IF_ADDREF(*node);
	}
	return rv;
}

nsresult createNode(PRUint32 value, nsIRDFNode **node)
{
	nsresult rv;
	char *valueStr = PR_smprintf("%d", value);
	nsString str(valueStr);
	PR_smprintf_free(valueStr);
	rv = createNode(str, node);
	return rv;
}

nsresult GetTargetHasAssertion(nsIRDFDataSource *dataSource, nsIRDFResource* folderResource,
							   nsIRDFResource *property,PRBool tv, nsIRDFNode *target,PRBool* hasAssertion)
{
	nsresult rv;
	if(!hasAssertion)
		return NS_ERROR_NULL_POINTER;

	nsCOMPtr<nsIRDFNode> currentTarget;

	rv = dataSource->GetTarget(folderResource, property,tv, getter_AddRefs(currentTarget));
	if(NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIRDFLiteral> value1(do_QueryInterface(target));
		nsCOMPtr<nsIRDFLiteral> value2(do_QueryInterface(currentTarget));
		if(value1 && value2)
			//If the two values are equal then it has this assertion
			rv = value1->EqualsLiteral(value2, hasAssertion);
	}
	else
		rv = NS_NOINTERFACE;

	return rv;

}

