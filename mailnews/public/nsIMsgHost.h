/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM nsIMsgHost.idl
 */

#ifndef __gen_nsIMsgHost_h__
#define __gen_nsIMsgHost_h__

#include "nsISupports.h" /* interface nsISupports */


/* starting interface nsIMsgHost */

/* {ADFB3740-AA57-11d2-B7ED-00805F05FFA5} */
#define NS_IMSGHOST_IID_STR "ADFB3740-AA57-11d2-B7ED-00805F05FFA5"
#define NS_IMSGHOST_IID \
  {0xADFB3740, 0xAA57, 0x11d2, \
    { 0xB7, 0xED, 0x00, 0x80, 0x5F, 0x05, 0xFF, 0xA5 }}

class nsIMsgHost : public nsISupports {
 private:
  void operator delete(void *); // NOT TO BE IMPLEMENTED

 public: 
  static const nsIID& IID() {
    static nsIID iid = NS_IMSGHOST_IID;
    return iid;
  }

  /*  <IDL>  */
  NS_IMETHOD GetHostname(char * *aHostname) = 0;
  NS_IMETHOD SetHostname(char * aHostname) = 0;

  /*  <IDL>  */
  NS_IMETHOD GetUiName(char * *aUiName) = 0;
  NS_IMETHOD SetUiName(char * aUiName) = 0;

  /*  <IDL>  */
  NS_IMETHOD GetPort(PRInt32 *aPort) = 0;
  NS_IMETHOD SetPort(PRInt32 aPort) = 0;
};

#endif /* __gen_nsIMsgHost_h__ */
