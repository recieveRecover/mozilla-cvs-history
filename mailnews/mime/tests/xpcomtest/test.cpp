#include <stdio.h>
#include "nsIComponentManager.h" 
#include "nsRFC822toHTMLStreamConverter.h"
#include "nsMimeObjectClassAccess.h"

static NS_DEFINE_CID(kRFC822toHTMLStreamConverterCID, NS_RFC822_HTML_STREAM_CONVERTER_CID); 
static NS_DEFINE_CID(kMimeObjectClassAccessCID, NS_MIME_OBJECT_CLASS_ACCESS_CID); 

/* 
 * This is just a testing for libmime. All I'm doing is loading a component,
 * and querying it for a particular interface. It is its only purpose / use....
 */
int main(int argc, char *argv[]) 
{ 
  nsRFC822toHTMLStreamConverter *sample; 
  nsMimeObjectClassAccess	*objAccess;
  
  // register our dll
  nsComponentManager::RegisterComponent(kRFC822toHTMLStreamConverterCID, NULL, NULL,
                                "mime.dll", PR_FALSE, PR_FALSE);
  nsComponentManager::RegisterComponent(kMimeObjectClassAccessCID, NULL, NULL,
                                "mime.dll", PR_FALSE, PR_FALSE);
  
  nsresult res = nsComponentManager::CreateInstance(kRFC822toHTMLStreamConverterCID, 
                    NULL, nsIStreamConverter::GetIID(), (void **) &sample); 
  if (res == NS_OK && sample) 
  { 
    void *stream;

    printf("We succesfully obtained a nsRFC822toHTMLStreamConverter interface....\n");
    sample->SetOutputStream((nsIOutputStream *)stream);
    printf("Releasing the interface now...\n");
    sample->Release(); 
  } 

  printf("Time for try the nsMimeObjectClassAccess class...\n");
  res = nsComponentManager::CreateInstance(kMimeObjectClassAccessCID, 
                    NULL, nsIMimeObjectClassAccess::GetIID(), 
                    (void **) &objAccess); 
  if (res == NS_OK && objAccess) 
  { 
  void *ptr;
  
    printf("We succesfully obtained a nsMimeObjectClassAccess interface....\n");
    if (objAccess->GetmimeInlineTextClass(&ptr) == NS_OK)
      printf("Text Class Pointer = %x\n", ptr);
    else
      printf("Failed on XP-COM call\n");
    
    printf("Releasing the interface now...\n");
    objAccess->Release(); 
  } 
  return 0;  
}
