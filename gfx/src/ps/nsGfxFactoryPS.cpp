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


#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"

#include "nsDeviceContextPS.h"
#include "nsGfxPSCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextPS)

struct components_t {
  nsCID cid;
  nsIGenericFactory::ConstructorProcPtr constructor;
  const char *progid;
  const char *description;
};

static components_t components[] =
{
  { NS_DEVICECONTEXTPS_CID, &nsDeviceContextPSConstructor, "component://netscape/gfx/decidecontext/ps", "GFX Postscript Device Context", },
};

NS_IMPL_MODULE(components)
NS_IMPL_NSGETMODULE(nsModule)



#if 0
static NS_DEFINE_CID(kCDeviceContextPS, NS_DEVICECONTEXTPS_CID);
static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);

class nsGfxFactoryPS : public nsIFactory
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD CreateInstance(nsISupports *aOuter,
                            const nsIID &aIID,
                            void **aResult);

  NS_IMETHOD LockFactory(PRBool aLock);

  nsGfxFactoryPS(const nsCID &aClass);
  virtual ~nsGfxFactoryPS();

private:
  nsCID mClassID;

};

nsGfxFactoryPS::nsGfxFactoryPS(const nsCID &aClass) :
  mRefCnt(0),
  mClassID(aClass)
{   
}   

nsGfxFactoryPS::~nsGfxFactoryPS()   
{   
}   

NS_IMPL_ISUPPORTS(nsGfxFactoryPS, nsIFactory::GetIID())


NS_IMETHODIMP
nsGfxFactoryPS::CreateInstance(nsISupports *aOuter,  
                                const nsIID &aIID,  
                                void **aResult)  
{  
  if (aResult == nsnull) {  
    return NS_ERROR_NULL_POINTER;  
  }  

  *aResult = nsnull;  
  
  nsISupports *inst = nsnull;

  if (mClassID.Equals(kCDeviceContextPS)) {
    inst = (nsISupports *)(nsIDeviceContextPS*)new nsDeviceContextPS();
  }

  if (inst == nsnull) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = inst->QueryInterface(aIID, aResult);

  if (rv != NS_OK) {
    delete inst;
  }

  return rv;
  
}

nsresult nsGfxFactoryPS::LockFactory(PRBool aLock)
{
  // Not implemented in simplest case.  
  return NS_OK;
}

nsresult
NSGetFactory(nsISupports* servMgr,
             const nsCID &aClass,
             const char *aClassName,
             const char *aProgID,
             nsIFactory **aFactory)
{
  if (nsnull == aFactory) {
    return NS_ERROR_NULL_POINTER;
  }

  *aFactory = new nsGfxFactoryPS(aClass);

  if (nsnull == aFactory) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return (*aFactory)->QueryInterface(nsIFactory::GetIID(), (void**)aFactory);
  
}

PRBool
NSCanUnload(nsISupports* aServMgr)
{
  return PR_TRUE;
}

nsresult
NSRegisterSelf(nsISupports* aServMgr, const char *fullpath)
{
  nsresult rv;

#ifdef NS_DEBUG
  printf("*** Registering GFX Postscript\n");
#endif

  nsCOMPtr<nsIServiceManager>
    serviceManager(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;
  
  NS_WITH_SERVICE(nsIComponentManager, compMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = compMgr->RegisterComponent(kCDeviceContextPS,
                                  "GFX Postscript Device Context",
                                  "component://netscape/gfx/devicecontext/ps",
                                  fullpath,
                                  PR_TRUE, PR_TRUE);
  
  return rv;
}

nsresult
NSUnregisterSelf(nsISupports* aServMgr, const char *fullpath)
{

  nsresult rv;
  nsCOMPtr<nsIServiceManager>
    serviceManager(do_QueryInterface(aServMgr, &rv));
  if (NS_FAILED(rv)) return rv;
  
  NS_WITH_SERVICE(nsIComponentManager, compMgr, kComponentManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  compMgr->UnregisterComponent(kCDeviceContextPS, fullpath);
  
  return NS_OK;
}
#endif /* 0 */
