/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): Judson Valeski
 *							 : David Matiskella
 */
#include "nsXMLMIMEDataSource.h"
#include "nsVoidArray.h"
#include "nsString2.h"
#include "nsMIMEInfoImpl.h"
#include "nsIURL.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsEnumeratorUtils.h"
#include "nsIChannel.h"
#include "nsIFileTransportService.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsFileLocations.h"
#include "nsILocalFile.h"
#include "nsMimeTypes.h"
// Crap
#include "nsIFileSpec.h"
#include "nsFileSpec.h"

const char* kMIME="mime";
const char* kMIMEType="mimetype";
const char* kDescription="description";
const char* kExtensions="extensions";

static NS_DEFINE_CID(kFileTransportServiceCID, NS_FILETRANSPORTSERVICE_CID);
// Hash table helper functions
PRBool DeleteEntry(nsHashKey *aKey, void *aData, void* closure) {
    nsMIMEInfoImpl *entry = (nsMIMEInfoImpl*)aData;
    NS_ASSERTION(entry, "mapping problem");
	NS_RELEASE(entry);
    return PR_TRUE;   
};

// nsISupports methods
NS_IMPL_THREADSAFE_ISUPPORTS1(nsXMLMIMEDataSource, nsIMIMEDataSource);

NS_METHOD
nsXMLMIMEDataSource::Create(nsISupports* aOuter, const nsIID& aIID, void* *aResult) {
    nsXMLMIMEDataSource* service = new nsXMLMIMEDataSource();
    if (!service) return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(service);
    nsresult rv = service->Init();
    if (NS_FAILED(rv)) return rv;
    rv = service->QueryInterface(aIID, aResult);
    NS_RELEASE(service);
    return rv;
}

// nsXMLMIMEDataSource methods
nsXMLMIMEDataSource::nsXMLMIMEDataSource() {
    NS_INIT_REFCNT();
}

nsXMLMIMEDataSource::~nsXMLMIMEDataSource() {
    mInfoObjects->Reset(DeleteEntry, nsnull);
    delete mInfoObjects;
}

nsresult
nsXMLMIMEDataSource::Init() {
    nsresult rv = NS_OK;
    mInfoObjects = new nsHashtable();
    if (!mInfoObjects) return NS_ERROR_OUT_OF_MEMORY;

    rv = NS_NewISupportsArray(getter_AddRefs(mInfoArray));
    if (NS_FAILED(rv)) return rv;

    return InitFromHack();
}

 /* This bad boy needs to retrieve a url, and parse the data coming back, and
  * add entries into the mInfoArray.
  */
nsresult
nsXMLMIMEDataSource::InitFromURI(nsIURI *aUri) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsXMLMIMEDataSource::InitFromFile( nsIFile* /* aFile */ )
{
#if 0
	nsresult rv;
	nsCOMPtr<nsIChannel> channel;
  NS_WITH_SERVICE(nsIFileTransportService, fts, kFileTransportServiceCID, &rv) ;
  if(NS_FAILED(rv)) return rv ;
  // Made second parameter 0 since I really don't know what it is used for
  rv = fts->CreateTransport(aFile,0, "load", 0, 0, getter_AddRefs(channel)) ;
  if(NS_FAILED(rv))
    return rv ;
  
  // we don't need to worry about notification callbacks
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->OpenInputStream(0, 0, getter_AddRefs( stream ) ) ;
#endif	
    
   return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsXMLMIMEDataSource::AddMapping(const char* mimeType, 
                          const char* extension,
                          const char* description,
                          nsIURI* dataURI, PRUint32 type, PRUint32 creator)
{
    nsresult rv = NS_OK;
    // setup the new MIMEInfo object.
    nsMIMEInfoImpl* anInfo = new nsMIMEInfoImpl(mimeType);
    if (!anInfo) return NS_ERROR_OUT_OF_MEMORY;

    anInfo->mExtensions.AppendCString(extension);
    anInfo->mDescription = description;
    anInfo->mURI = dataURI;

		anInfo->mMacType = type;
		anInfo->mMacCreator = creator;
    // The entry is mapped many-to-one and the MIME type is the root mapping.
    
    // First remove any existing mapping.
    rv = Remove(mimeType);
    if (NS_FAILED(rv)) return rv;

    // Next add the new root MIME mapping.
    nsStringKey key(mimeType);
    nsMIMEInfoImpl* oldInfo = (nsMIMEInfoImpl*)mInfoObjects->Put(&key, anInfo);
    NS_ASSERTION(!oldInfo, "we just removed the entry, we shouldn't have one");
    NS_ADDREF(anInfo);

    rv = mInfoArray->AppendElement(anInfo); // update the root array.
    if (NS_FAILED(rv)) return rv;

    // Finally add an extension mapping.
    key = extension;
    oldInfo = (nsMIMEInfoImpl*)mInfoObjects->Put(&key, anInfo);
    NS_ASSERTION(!oldInfo, "file extension mappings should have been cleaned up in the RemoveMapping call");
    
    NS_ADDREF(anInfo);

    return NS_OK;
}

NS_IMETHODIMP
nsXMLMIMEDataSource::Add( nsIMIMEInfo* aMapper )
{
		if ( !aMapper )
			return NS_ERROR_NULL_POINTER;
			
    nsresult rv = NS_OK;
    
		nsXPIDLCString mimeType;
		rv = aMapper->GetMIMEType( getter_Copies( mimeType ) );
		if ( NS_FAILED( rv ) )
			return rv;
    // First remove any existing mapping.
    rv = Remove(mimeType);
    if (NS_FAILED(rv)) return rv;

    // Next add the new root MIME mapping.
    nsStringKey key(mimeType);
    nsMIMEInfoImpl* oldInfo = (nsMIMEInfoImpl*)mInfoObjects->Put(&key, aMapper);
    NS_ASSERTION(!oldInfo, "we just removed the entry, we shouldn't have one");
    NS_ADDREF(aMapper);

    rv = mInfoArray->AppendElement(aMapper); // update the root array.
    if (NS_FAILED(rv)) return rv;

    // Finally add an extension mapping.
    char** extensions;
		PRInt32 count;
		rv = aMapper->GetFileExtensions(& count, &extensions );
		if ( NS_FAILED ( rv ) )
			return rv;
		for ( PRInt32 i = 0; i<count; i++ )
		{
		 	key = extensions[i];
		  oldInfo = (nsMIMEInfoImpl*)mInfoObjects->Put(&key, aMapper);
      NS_ASSERTION(!oldInfo, "file extension mappings should have been cleaned up in the RemoveMapping call");
      NS_ADDREF(aMapper);
			nsAllocator::Free( extensions[i] );
		}
   	nsAllocator::Free( extensions ); 
   
    return NS_OK;
}

// used to cleanup any file extension mappings when 
// a root MIME entry is removed.
PRBool removeExts(nsCString& aElement, void *aData) {
    nsHashtable* infoObjects = (nsHashtable*)aData;
    NS_ASSERTION(infoObjects, "hash table botched up");

    nsStringKey key(aElement);
    nsMIMEInfoImpl* info = (nsMIMEInfoImpl*)infoObjects->Remove(&key);
    NS_RELEASE(info);
    return PR_TRUE;
}

NS_IMETHODIMP
nsXMLMIMEDataSource::Remove(const char* aMIMEType) {
    nsresult rv = NS_OK;
    nsStringKey key(aMIMEType);

    // First remove the root MIME mapping.
    nsMIMEInfoImpl* info = (nsMIMEInfoImpl*)mInfoObjects->Remove(&key);
    if (!info) return NS_OK;

    rv = mInfoArray->RemoveElement(info); // update the root array.
    if (NS_FAILED(rv)) return rv;

    // Next remove any file association mappings.
    rv = info->mExtensions.EnumerateForwards(removeExts, mInfoObjects);
    NS_RELEASE(info);
    if (NS_FAILED(rv)) return rv;

		// mMacCache is potentially invalid
		mMacCache.Reset();
		
    return NS_OK;
}

nsresult
nsXMLMIMEDataSource::AppendExtension(const char* mimeType, const char* extension) {
    nsStringKey key(mimeType);

    nsMIMEInfoImpl* info = (nsMIMEInfoImpl*)mInfoObjects->Get(&key);
    if (!info) return NS_ERROR_FAILURE;

    info->mExtensions.AppendCString(extension);

    // Add another file extension mapping.
    key = extension;
    nsMIMEInfoImpl* oldInfo = (nsMIMEInfoImpl*)mInfoObjects->Put(&key, info);
    NS_IF_RELEASE(oldInfo); // overwrite any existing mapping.
    NS_ADDREF(info);

    return NS_OK;
}

nsresult
nsXMLMIMEDataSource::RemoveExtension(const char* aExtension) {
    nsresult rv = NS_OK;
    nsStringKey key(aExtension);

    // First remove the extension mapping.
    nsMIMEInfoImpl* info = (nsMIMEInfoImpl*)mInfoObjects->Remove(&key);
    if (!info) return NS_ERROR_FAILURE;
    
    // Next remove the root MIME mapping from the array and hash
    // IFF this was the only file extension mapping left.
    PRBool removed = info->mExtensions.RemoveCString(key.GetString());
    NS_ASSERTION(removed, "mapping problem");

    if (info->GetExtCount() == 0) {
        // it's empty, remove the root mapping from hash and array.
        nsXPIDLCString mimeType;
        rv = info->GetMIMEType(getter_Copies(mimeType));
        if (NS_FAILED(rv)) return rv;

        key = (const char*)mimeType;
        nsMIMEInfoImpl* rootEntry = (nsMIMEInfoImpl*)mInfoObjects->Remove(&key);
        NS_ASSERTION(rootEntry, "mapping miss-hap");

        rv = mInfoArray->RemoveElement(rootEntry); // update the root array
        if (NS_FAILED(rv)) return rv;

        NS_RELEASE(rootEntry);
    }
    
    NS_RELEASE(info);

    return NS_OK;
}

NS_IMETHODIMP
nsXMLMIMEDataSource::GetEnumerator(nsISimpleEnumerator* *aEnumerator) {
		return NS_NewArrayEnumerator( aEnumerator, mInfoArray);
 
}

NS_IMETHODIMP
nsXMLMIMEDataSource::Serialize() {

	nsresult rv = NS_OK;
	#if 0
// Init mFile Hack
	// Lovely hack until the filespec and nsIFile stuff is merged together
   // nsFileSpec dirSpec;
   // nsCOMPtr<nsIFileSpec> spec = NS_LocateFileOrDirectory(nsSpecialFileSpec::App_UserProfileDirectory50);
   // if (!spec) return NS_ERROR_FAILURE;
   // spec->GetFileSpec(&dirSpec);
   nsCOMPtr<nsILocalFile> file;
	//	char* path;
//		spec->GetNativePath(& path );
		const char* path = "development:";
		rv = NS_NewLocalFile( path, getter_AddRefs(file));
	
		if ( NS_SUCCEEDED( rv ) )
    {
    	
    	file->Append("mime.xml");
			mFile = file;
			file->SetLeafName("mime.temp");
		}
		mFile = file;
//
	
	nsCOMPtr<nsIChannel> channel;
  NS_WITH_SERVICE(nsIFileTransportService, fts, kFileTransportServiceCID, &rv) ;
  if(NS_FAILED(rv)) return rv ;
 
  rv = fts->CreateTransport( mFile,0, "load", 0, 0, getter_AddRefs(channel)) ;
  if(NS_FAILED(rv))
    return rv ;
  
  // we don't need to worry about notification callbacks
  nsCOMPtr<nsIOutputStream> stream;
  rv = channel->OpenOutputStream( 0, getter_AddRefs( stream ) ) ;
	
	nsCOMPtr<nsISimpleEnumerator> enumerator;	
	rv = GetEnumerator( getter_AddRefs( enumerator ) );
  if ( NS_FAILED( rv ) )
  	return rv;
  nsCString buffer;
  nsXPIDLCString cdata;
  nsXPIDLString unidata;
  
  buffer="<?xml version=\"1.0\"?>\r";
  PRUint32 bytesWritten;
  PRBool more;
 
	rv = stream->Write( buffer  , buffer.Length(), &bytesWritten );

  while ( NS_SUCCEEDED( enumerator->HasMoreElements(& more ) )&& more ) 
	{
		nsCOMPtr<nsIMIMEInfo> info;
	  rv = enumerator->GetNext( getter_AddRefs( info ) );
	  if ( NS_FAILED ( rv ) )
			return rv;
			
		buffer="<mimetype ";
		rv = info->GetDescription( getter_Copies( unidata ) );
		if ( NS_FAILED ( rv ) )
			return rv;
		buffer+= kDescription;
		buffer+="=\"";
		nsString temp = unidata;
		char* utf8 = temp.ToNewUTF8String();
		buffer+=utf8;
		nsAllocator::Free( utf8 );
		buffer+="\" ";
		
		rv = info->GetMIMEType( getter_Copies( cdata ) );
		if ( NS_FAILED ( rv ) )
			return rv;
		buffer+=kMIMEType;
		buffer+="=\"";
		buffer+=cdata;
		buffer+="\" ";
		
		char** extensions;
		PRInt32 count;
		rv = info->GetFileExtensions(& count, &extensions );
		if ( NS_FAILED ( rv ) )
			return rv;
		buffer+=kExtensions;
		buffer+="=\"";
		for ( PRInt32 i = 0; i<(count-1); i++ )
		{
			buffer+=extensions[i];
			buffer+=",";
			nsAllocator::Free( extensions[i] );
		}
		buffer+=extensions[count-1];
		buffer+="\" ";
		nsAllocator::Free( extensions[count-1] );
		nsAllocator::Free( extensions );
			
		buffer+="/>\r";
	
		rv = stream->Write( buffer  , buffer.Length(), &bytesWritten );
	
  	if ( NS_FAILED( rv ) )
  		return rv;
  }
  rv = stream->Close();
 // Now delete the old file and rename the new one
  mFile->Delete( PR_FALSE);
  file->MoveTo( NULL, "mime.xml");
#endif
  return rv;

}

nsresult
nsXMLMIMEDataSource::InitFromHack() {
    nsresult rv;

    rv = AddMapping(TEXT_PLAIN, "txt", "Text File", nsnull, 'TEXT','ttxt');
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(TEXT_PLAIN, "text");
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(APPLICATION_OCTET_STREAM, "exe", "Binary Executable", nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "bin");
    if (NS_FAILED(rv)) return rv;
#if defined(VMS)
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "sav");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "bck");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "pcsi");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "pcsi-dcx_axpexe");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_OCTET_STREAM, "pcsi-dcx_vaxexe");
    if (NS_FAILED(rv)) return rv;
#endif

    rv = AddMapping(TEXT_HTML, "htm", "Hyper Text Markup Language", nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(TEXT_HTML, "html");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(TEXT_HTML, "shtml");
    if (NS_FAILED(rv)) return rv;
		
		rv = AppendExtension(TEXT_HTML, "ehtml");
		if (NS_FAILED(rv)) return rv;

    rv = AddMapping(TEXT_RDF, "rdf", "Resource Description Framework", nsnull);
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(TEXT_XUL, "xul", "XML-Based User Interface Language", nsnull, 'TEXT','ttxt' );
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(TEXT_XML, "xml", "Extensible Markup Language", nsnull);
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(TEXT_CSS, "css", "Style Sheet", nsnull, 'TEXT','ttxt');
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(APPLICATION_JAVASCRIPT, "js", "Javascript Source File", nsnull, 'TEXT','ttxt');
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(MESSAGE_RFC822, "eml", "RFC-822 data", nsnull);
    if (NS_FAILED(rv)) return rv;
    
    rv = AddMapping(APPLICATION_GZIP2, "gz", "gzip", nsnull);
    if (NS_FAILED(rv)) return rv;


    rv = AddMapping(IMAGE_GIF, "gif", "GIF Image", nsnull, 'GIFf','GCon' );
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(IMAGE_JPG, "jpeg", "JPEG Image", nsnull, 'JPEG', 'GCon' );
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(IMAGE_JPG, "jpg");
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(IMAGE_PNG, "png", "PNG Image", nsnull);
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(IMAGE_ART, "art", "ART Image", nsnull);
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(IMAGE_TIFF, "tiff", "TIFF Image", nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(IMAGE_TIFF, "tif");
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(APPLICATION_POSTSCRIPT, "ps", "Postscript File", nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_POSTSCRIPT, "eps");
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(APPLICATION_POSTSCRIPT, "ai");
    if (NS_FAILED(rv)) return rv;
                 
    rv = AddMapping(TEXT_RTF, "rtf", "Rich Text Format", nsnull);
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(TEXT_RTF, "rtf");
    if (NS_FAILED(rv)) return rv;

    rv = AddMapping(TEXT_CPP, "cpp", "CPP file", nsnull,  'TEXT','CWIE');
    if (NS_FAILED(rv)) return rv;
    rv = AppendExtension(TEXT_CPP, "cpp");
    if (NS_FAILED(rv)) return rv;
    
    rv = AddMapping( "application/x-arj", "arj", "ARj file", nsnull);
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}


// nsIMIMEService methods
NS_IMETHODIMP
nsXMLMIMEDataSource::GetFromExtension(const char *aFileExt, nsIMIMEInfo **_retval) {
    // for now we're assuming file extensions are case insensitive.
    nsCAutoString fileExt(aFileExt);
    fileExt.ToLowerCase();

    nsStringKey key(fileExt.GetBuffer());

    nsMIMEInfoImpl *entry = (nsMIMEInfoImpl*)mInfoObjects->Get(&key);
    if (!entry) return NS_ERROR_FAILURE;
    NS_ADDREF(entry);
    *_retval = NS_STATIC_CAST(nsIMIMEInfo*, entry);
    return NS_OK;
}



NS_IMETHODIMP
nsXMLMIMEDataSource::GetFromMIMEType(const char *aMIMEType, nsIMIMEInfo **_retval) {
    nsCAutoString MIMEType(aMIMEType);
    MIMEType.ToLowerCase();

    nsStringKey key(MIMEType.GetBuffer());

    nsMIMEInfoImpl *entry = (nsMIMEInfoImpl*)mInfoObjects->Get(&key);
    if (!entry) return NS_ERROR_FAILURE;
    NS_ADDREF(entry);
    *_retval = NS_STATIC_CAST(nsIMIMEInfo*, entry);
    return NS_OK;
}

NS_IMETHODIMP
nsXMLMIMEDataSource::GetFromTypeCreator(PRUint32 aType, PRUint32 aCreator, const char* aExt,  nsIMIMEInfo **_retval)
{
		    
    char buf[16];
    buf[0] = aType;
    buf[4] = aCreator;
    nsAutoString keyString( buf,8 );
    keyString+=aExt;
    nsStringKey key(  keyString );
    // Check if in cache for real quick look up of common ( html,js, xul, ...) types
		nsIMIMEInfo *entry = (nsIMIMEInfo*)mMacCache.Get(&key);
    if (entry)
    {
    	NS_ADDREF(entry);
   	  *_retval = entry;
   		return NS_OK;
	 }
	 // iterate through looking for a match
	 // must match type, bonus points for ext, and app
	 // use the same scoring as IC
	 PRInt32 score = 0;
	 nsCOMPtr<nsISimpleEnumerator> enumerator;
	 nsresult rv = GetEnumerator( getter_AddRefs( enumerator ) );
	 if ( NS_FAILED( rv ) )
	 		return rv;
	 nsCOMPtr< nsIMIMEInfo> info;
	 nsCOMPtr<nsIMIMEInfo> bestMatch;
	 PRBool hasMore;
	 nsCString extString ( aExt );
	 while( NS_SUCCEEDED(enumerator->HasMoreElements( & hasMore ) ) && hasMore )
	 {
	 		enumerator->GetNext( getter_AddRefs( info ) );
	 		PRUint32 type, creator;
	 		info->GetMacType( &type );
	 		info->GetMacCreator( &creator );
	 		
	 		if ( type == aType )
	 		{
	 			PRInt32 scoreOfElement  = 1;
	 			if ( creator == aCreator )
	 				scoreOfElement++;
	 			
	 			PRBool tempBool = PR_FALSE;
	 			info->ExtensionExists( aExt, &tempBool );
	 			if ( tempBool )
	 				scoreOfElement+=	extString.Length()*2; 		
	 		
	 			if ( scoreOfElement > score )
	 			{
	 				score = scoreOfElement;
	 				bestMatch = info;
	 			}
	 		}
	 }
	 
	 if( score != 0 )
	 {
	 		*_retval = bestMatch;
	 		NS_ADDREF( 	*_retval );
	 		
	 		// Add to cache
	 		mMacCache.Put( &key, bestMatch.get() );
	 		return NS_OK;
	 }
	 
	 return NS_ERROR_FAILURE;
}