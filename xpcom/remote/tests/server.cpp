/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Sun Microsystems,
 * Inc. Portions created by Sun are
 * Copyright (C) 1999 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */
#include <stdio.h>
#include "RPCServerService.h"
#include "nsISupports.h"
#include "nsIJVMManager.h"
#include "nsIRPCTest.h"
#include <unistd.h>
#include "IDispatcher.h"
#include "nsIThread.h"

class nsRPCTestImpl : public  nsIRPCTest {
    NS_DECL_ISUPPORTS 
    nsRPCTestImpl() {
	NS_INIT_REFCNT();
    }
    NS_IMETHOD Test1(PRInt32 *l) {
        printf("Test1 this=%p\n", this);
	printf("--nsRPCTestImpl::Test1 %d\n",*l);
	*l = 1234;
	printf("--nsRPCTestImpl::Test1 %d\n",*l);
	return NS_OK;
    }
    NS_IMETHOD Test2(PRInt32 l1, PRInt32* l2) {
        printf("Test2 this=%p\n", this);
	printf("--nsRPCTestImpl::Test2 %d %d\n",l1,*l2);
	*l2 = l1;
	printf("--nsRPCTestImpl::Test2 %d %d\n",l1,*l2);
	return NS_OK;
    }
    NS_IMETHOD Test3(const char *s1, char **s2) {
	printf("Test3 s1 %s s2 %s\n",s1,*s2);
	*s2 =  "hi";
	return NS_OK;
    }
    NS_IMETHOD Test4(PRUint32 count, const char **valueArray) {
	printf("--Test4\n");
	printf("count %d\n",count);
	for (int i = 0; i < count; i++) {
	    printf("--valueArray[%d]=%s\n",i,valueArray[i]);
	}
	return NS_OK;
    }
};

NS_IMPL_ISUPPORTS(nsRPCTestImpl, NS_GET_IID(nsIRPCTest));
int main(int argc, char **args) {
    printf("%x\n",getpid());
    nsRPCTestImpl * test1 = new nsRPCTestImpl();
    nsRPCTestImpl * test2 = new nsRPCTestImpl();
    RPCServerService * rpcService = RPCServerService::GetInstance();
    IDispatcher *dispatcher;
    rpcService->GetDispatcher(&dispatcher);
    dispatcher->RegisterWithOID(test1, 1);
    dispatcher->RegisterWithOID(test2, 5);
    while(1) {
	PR_Sleep(100);
    }
    
}

