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
#include "RPCClientService.h"
#include "nsISupports.h"
#include "nsISample.h"
#include "nsIRPCTestOut.h"

extern char* transportName;
int main(int argc, char **args) {
    if (argc < 2) {
	printf("format client <server pid>\n");
	exit(1);
    }
    RPCClientService::Initialize(args[1]);
    RPCClientService * rpcService = RPCClientService::GetInstance();
    nsIID iid;
    nsIRPCTestOut *t1; 
    PRBool b;
    PRUint8 o;
    PRInt16 sInt;
    PRInt32 lInt;
    PRInt64 llInt;
    PRUint16 usInt;
    PRUint32 ulInt;
    PRUint64 ullInt;
    float  f;
    double d;
    char   c;
    PRUnichar wc;
    char* s;
    PRUnichar ws;  

    rpcService->CreateObjectProxy(1, NS_GET_IID(nsIRPCTestOut),(nsISupports**)&t1);
    b = PR_TRUE;
    printf("--before call Test1 %d\n",b);
    t1->Test1(&b);
    printf("--after call Test1 %d\n",b);
    t1->Test1(&b);
    printf("____________________________________________\n");
    o = 124;
    printf("--before call Test2 %d\n",o);
    t1->Test2(&o);
    printf("--after call Test2 %d\n",o);
    t1->Test2(&o);
    printf("____________________________________________\n");
    sInt = -32768;
    printf("--before call Test3 %d\n",sInt);
    t1->Test3(&sInt);
    printf("--after call Test3 %d\n",sInt);
    t1->Test3(&sInt);
    printf("____________________________________________\n");
    lInt = -2147483648;
    printf("--before call Test4 %d\n",lInt);
    t1->Test4(&lInt);
    printf("--after call Test4 %d\n",lInt);
    t1->Test4(&lInt);
    printf("____________________________________________\n");
    llInt = -9223372036854775808;
    printf("--before call Test5 %d\n",llInt);
    t1->Test5(&llInt);
    printf("--after call Test5 %d\n",llInt);
    t1->Test5(&llInt);
    printf("____________________________________________\n");
    usInt = 32768;
    printf("--before call Test6 %u\n",usInt);
    t1->Test6(&usInt);
    printf("--after call Test6 %u\n",usInt);
    t1->Test6(&usInt);
    printf("____________________________________________\n");
    ulInt = 4294967295;
    printf("--before call Test7 %u\n",ulInt);
    t1->Test7(&ulInt);
    printf("--after call Test7 %u\n",ulInt);
    t1->Test7(&ulInt);
    printf("____________________________________________\n");
    ullInt = 18446744073709551615;
    printf("--before call Test8 %u\n",ullInt);
    t1->Test8(&ullInt);
    printf("--after call Test8 %u\n",ullInt);
    t1->Test8(&ullInt);
    printf("____________________________________________\n");
    f = .123456789012345678901234567890;
    printf("--before call Test9 %.38f\n",f);
    t1->Test9(&f);
    printf("--after call Test9 %f\n",f);
    t1->Test9(&f);
    printf("____________________________________________\n");
    d = 1.7E308;
    printf("--before call Test10 %.50f\n",d);
    t1->Test10(&d);
    printf("--after call Test10 %f\n",d);
    t1->Test10(&d);
    printf("____________________________________________\n");
    c = 'A';
    printf("--before call Test %d\n",c);
    t1->Test11(&c);
    printf("--after call Test %d\n",c);
    t1->Test11(&c);
    printf("____________________________________________\n");
    wc = 'A';
    printf("--before call Test12 %d\n",wc);
    t1->Test12(&wc);
    printf("--after call Test12 %d\n",wc);
    t1->Test12(&wc);
    printf("____________________________________________\n");
    s = "This is a test.";
//    s = (char *) malloc(sizeof(char) * 50);
    printf("--before call Test13 %s\n",s);
    t1->Test13(&s);
    printf("--before call Test13 %s\n",s);
    t1->Test13(&s);
    printf("____________________________________________\n");

}
