/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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




#include "rdf-int.h"

char* error_string = NULL;
int lineNumber = 0;

static HashTable resourceHash = NULL;

RDF_Resource 
getResource (char* key, int createp) {
  RDF_Resource existing = HashLookup(resourceHash, key);
  if (existing) {
    return existing;
  } else if (createp){
    existing = (RDF_Resource)getMem(sizeof(RDF_ResourceStruct));
    existing->url = copyString(key);
    HashAdd(resourceHash, existing->url, existing);
    return existing;
  } else return NULL;
}

void readRDFFile (char* file) {
  FILE* f = fopen(file, "r");	
  if (f) {
    char* buff  = malloc(100 * 1024);
    int len ;
    memset(buff, '\0', (100 * 1024));
    len = fread(buff, 1, (100 * 1024) -1, f);
    buff[len] = '\0';
    if (!RDF_Consume(file, buff, len)) {
      printf("Error in RDF File\n");
    } else {
      printf("Finished reading %s\n", file);
    }
    free(buff);
  }
}

static HashTable rdftHash = NULL;

RDFT
getRDFT (char* key, int createp) {
  RDFT existing = HashLookup(rdftHash, key);
  if (existing) {
    return existing;
  } else if (createp){
    existing = (RDFT)getMem(sizeof(RDF_FileStruct));
    existing->url = copyString(key);
    HashAdd(rdftHash, existing->url, existing);
    return existing;
  } else return NULL;
}

void 
rdf_init () {
  error_string = getMem(1000);
  resourceHash = NewHashTable((int)0x00000FFF);
  rdftHash = NewHashTable((int)0x00000FFF);
}

int
rdf_DigestNewStuff (char* url, char* data, int len) {
  RDFT rf = (RDFT)getRDFT(url, 1) ; 
  int ok = 1;
  unloadRDFT(rf);
  memset(rf, '\0', sizeof(RDF_FileStruct));
  rf->line = (char*)getMem(RDF_BUF_SIZE);
  rf->holdOver = (char*)getMem(RDF_BUF_SIZE);
  rf->depth = 1;
  rf->lastItem = rf->stack[0] ;
  ok = parseNextRDFXMLBlobInt(rf, data, len);    
  if (!ok) unloadRDFT(rf);
  freeMem(rf->line);
  rf->line = NULL;
  freeMem(rf->holdOver);
  rf->holdOver = NULL;
  return ok;
}


int
startsWith (const char* pattern, const char* uuid) {
  int l1 = strlen(pattern);
  int l2 = strlen(uuid);
  int n;
  if (l2 < l1) return 0;
  for (n = 0; n < l1; n++) {
    if (pattern[n] != uuid[n]) return 0;
  } 
  return 1;
}

char* 
getMem (size_t n) {
  char* ans = (char*) malloc(n);
  if (ans) memset(ans, '\0', n);
  return ans;
}

void 
freeMem(void* item) {
  free(item);
}

char 
decodeEntityRef (char* string, int* stringIndexPtr, int len) {
  if (startsWith("lt;", string)) {
    *stringIndexPtr = *stringIndexPtr + 3;
    return '<';
  } else if (startsWith("gt;", string)) {
    *stringIndexPtr = *stringIndexPtr + 3;
    return '>';
  } else  if (startsWith("amp;", string)) {
    *stringIndexPtr = *stringIndexPtr + 4;
    return '&';
  } else return '&';
}

char *
copyStringIgnoreWhiteSpace(char* string)
{
   int len = strlen(string);
   char* buf = (char*)getMem(len + 1);
   int inWhiteSpace = 1;
   int buffIndex = 0;
   int stringIndex = 0;

   while (stringIndex < len) {
     char nextChar = *(string + stringIndex);
     int wsp = wsCharp(nextChar);
     if (!wsp) {
       if (nextChar == '&') {
         *(buf + buffIndex++) = decodeEntityRef(&string[stringIndex+1], 
                                                &stringIndex, len-stringIndex);
       } else {
         *(buf + buffIndex++) = nextChar;
       }
       inWhiteSpace = 0;
     } else if (!inWhiteSpace) {
       *(buf + buffIndex++) = ' ';
       inWhiteSpace = 1;
     } else {
       inWhiteSpace = 1;
     }
     stringIndex++;
   }

   return buf;
}

char *
getHref(char** attlist)
{
	char* ans = getAttributeValue(attlist, "resource");
	if (!ans) ans = getAttributeValue(attlist, "rdf:resource");
	return ans;
}


char *
getID(char** attlist)
{
	char* ans = getAttributeValue(attlist, "id");
	if (!ans) ans = getAttributeValue(attlist, "about"); 
	if (!ans) ans = getAttributeValue(attlist, "rdf:about");
	return ans;
}
 

int 
parseNextRDFXMLBlobInt(RDFT f, char* blob, int size) {
  int n, last, m;
  int somethingseenp = 0;
  n = last = 0; 
  while (n < size) {
    char c = blob[n];
    if ((c == '\n') || (c == '\r')) lineNumber++;
    m = 0;
    somethingseenp = 0;
    memset(f->line, '\0', RDF_BUF_SIZE-1);
    if (f->holdOver[0] != '\0') {
      memcpy(f->line, f->holdOver, strlen(f->holdOver));
      m = strlen(f->holdOver);
      somethingseenp = 1;
      memset(f->holdOver, '\0', RDF_BUF_SIZE-1);
    }   
    while ((n < size) && (wsCharp(c))) {
      c = blob[++n]; 
      if ((c == '\n') || (c == '\r')) lineNumber++;
    }
    while ((m < RDF_BUF_SIZE) && (c != '<') && (c != '>')) {
      f->line[m] = c;
      m++;
      somethingseenp = (somethingseenp || (!(wsCharp(c))));
      n++;	
      if (n < size) c = blob[n]; 
      else break;
      if ((c == '\n') || (c == '\r')) lineNumber++;
    }
    if (c == '>') f->line[m] = c;
    n++;
    if (m > 0) {
      if ((c == '<') || (c == '>')) {
        last = n;
        if (c == '<') f->holdOver[0] = '<'; 
        if (somethingseenp == 1) {
          int ok = parseNextRDFToken(f, f->line);
          if (!ok) 
			  return 0;
        }
      } else if (size > last) {
        memcpy(f->holdOver, f->line, m);
      }
    } else if (c == '<') f->holdOver[0] = '<';
  }
  return(1);
}

char *
getAttributeValue (char** attlist, char* elName)
{
  size_t n = 0;
  if (!attlist) return NULL;
  while ((n < 2*MAX_ATTRIBUTES) && (*(attlist + n) != NULL)) {
    if (strcmp(*(attlist + n), elName) == 0) return *(attlist + n + 1);
    n = n + 2;
  }
  return NULL;
}

char* 
copyString (char* str) {
  char* ans = getMem(strlen(str)+1);
  if (ans) {
    memcpy(ans, str, strlen(str));
    return ans;
  } else return NULL;
}

void
addElementProps (char** attlist, char* elementName, RDFT f, RDF_Resource obj)
{
  int count = 0;
  while (count < 2*MAX_ATTRIBUTES) {
    char* attName = attlist[count++];
    char* attValue = attlist[count++];
    if ((attName == NULL) || (attValue == NULL)) break;
    if (!stringEquals(attName, "resource") && 
        !stringEquals(attName, "rdf:resource")  && 
        !stringEquals(attName, "about") && 
        !stringEquals(attName, "rdf:about") && 
        !stringEquals(attName, "tv") &&
        !stringEquals(attName, "id")) {
      remoteStoreAdd(f, obj, getResource(attName, 1), 
                   copyStringIgnoreWhiteSpace(attValue), 
                   RDF_STRING_TYPE, 1);
    }
  }
}
        
int
parseNextRDFToken (RDFT f, char* token)
{
  char* attlist[2*MAX_ATTRIBUTES+1];
  char* elementName;
  if (token[0] == '<') {
    size_t len = strlen(token);
    if (token[len-2] != '/') {
      if (token[1] == '/') {
        char* tok = getMem(len);
        memcpy(tok, &token[2], len-3);
        if (!stringEquals(tok, f->tagStack[f->tagDepth-1])) {
          sprintf(error_string, "Unbalanced tags : Expecting </%s>, found %s", 
                  f->tagStack[f->tagDepth-1], token);
          return 0;
        } else {
          f->tagDepth--;
          freeMem(tok);
        }
      } 
    }
  }
        
  if (token[0] != '<')   {
    if ((f->status == EXPECTING_OBJECT) && (f->depth > 1)) {
      RDF_Resource u = f->stack[f->depth-2];
      RDF_Resource s = f->stack[f->depth-1];
      char* val      = copyStringIgnoreWhiteSpace(token);
      remoteStoreAdd(f, u, s, val , RDF_STRING_TYPE, 1);
    } else  {
      sprintf(error_string, "Did not expect \n\"%s\".\n Was expecting a tag.", token);
      return 0;
    } 
  } else if  (startsWith("<!--", token)) {
    return 1;
  } else if (token[1] == '?')  {
    return 1;
  } else if (token[1] == '/') {
    if ((f->status != EXPECTING_OBJECT) && (f->status != EXPECTING_PROPERTY)) {
      sprintf(error_string, "Did not expect %s. Something pretty screwed up", token);
      return 0;
    }
    if (f->depth > 0) f->depth--;
    f->status = (f->status == EXPECTING_OBJECT ? EXPECTING_PROPERTY : EXPECTING_OBJECT);
    return 1;
  } else if ((f->status == 0) && (startsWith("<RDF:RDF", token) || 
                                  startsWith("<RDF>", token))) {
    f->tagStack[f->tagDepth++] = copyString("RDF"); 
    f->status = EXPECTING_OBJECT;
    return 1;
  } else {
    int emptyElementp = (token[strlen(token)-2] == '/');  
    if ((f->status != EXPECTING_OBJECT) && (f->status != EXPECTING_PROPERTY)) return 1;
    if (!tokenizeElement(token, attlist, &elementName)) return 0;
    if (!emptyElementp) 
		f->tagStack[f->tagDepth++] = copyString(elementName);
    if (f->status == EXPECTING_OBJECT) {
      char* url = NULL;
      RDF_Resource obj;
      int count = 0;    
      url = getID(attlist);
      if (!url) {
        if (f->tagDepth > 2) {
          sprintf(error_string, "Unbalanced tags : Expecting </%s>, found %s", 
                  f->tagStack[f->tagDepth-2], token);
        } else {
          sprintf(error_string, "Require a \"about\" attribute on %s", token);
        }
        return 0;
      }
      obj =  getResource(url, 1);
      addElementProps (attlist, elementName, f, obj) ;
      if (!stringEquals(elementName, "RDF:Description")) {
          RDF_Resource eln = getResource(elementName, 1);
          remoteStoreAdd(f, obj, getResource("instanceOf", 1), 
                       eln, RDF_RESOURCE_TYPE, 
                       1);        
      }
      if (f->depth > 1) {
        remoteStoreAdd(f, f->stack[f->depth-2], f->stack[f->depth-1], obj, 
                     RDF_RESOURCE_TYPE, 1);
      }
      if (!emptyElementp) {
        f->stack[f->depth++] = obj;
        f->status = EXPECTING_PROPERTY;
      }
    } else if (f->status == EXPECTING_PROPERTY) {
      char* url;
      RDF_Resource obj;
      int count = 0;
      url = getHref(attlist) ;      
      if (url) {
        RDF_Resource eln = getResource(elementName, 1);      
        obj =  getResource(url, 1);        
        freeMem(url);
        addElementProps (attlist, elementName, f, obj) ;     
        remoteStoreAdd(f, f->stack[f->depth-1], eln, obj, RDF_RESOURCE_TYPE,  1);
      } 
      if (!emptyElementp) {
        f->stack[f->depth++] = getResource(elementName, 1);
        f->status = EXPECTING_OBJECT;
      }
    }
  }
}	



int
tokenizeElement (char* attr, char** attlist, char** elementName)
{
  size_t n = 1;
  size_t s = strlen(attr); 
  char c ;
  size_t m = 0;
  size_t atc = 0;
  int emptyTagp =  (attr[s-2] == '/');
  int inAttrNamep = 1;
  c = attr[n++]; 
  while (wsCharp(c)) {
    c = attr[n++];
  }
  *elementName = &attr[n-1];
  while (n < s) {
    if (wsCharp(c)) break;
    c = attr[n++];
  }
  attr[n-1] = '\0';
  while (atc < 2*MAX_ATTRIBUTES+1) {*(attlist + atc++) = NULL;}
  atc = 0;
  s = (emptyTagp ? s-2 : s-1);
  while (n < s) {
    int attributeOpenStringSeenp = 0;
    m = 0;
    c = attr[n++];
    while ((n <= s) && (atc < 2*MAX_ATTRIBUTES)) {
      if (inAttrNamep && (m > 0) && (wsCharp(c) || (c == '='))) {
	attr[n-1] = '\0';
	*(attlist + atc++) = &attr[n-m-1];
	break;
      }
      if  (!inAttrNamep && attributeOpenStringSeenp && (c == '"')) {
	attr[n-1] = '\0';
	*(attlist + atc++) = &attr[n-m-1];
	break;
      }
      if (inAttrNamep) {
	if ((m > 0) || (!wsCharp(c))) m++;
      } else {
	if (c == '"') {
	  attributeOpenStringSeenp = 1;
	} else {
	  if ((m > 0) || (!(wsCharp(c)))) m++;
	}
      }
      c = attr[n++];
    }
    inAttrNamep = (inAttrNamep ? 0 : 1);
  }
  return 1;
}
