/* 
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


package org.mozilla.pluglet.test.basic.api;

import java.util.*;
import java.io.*;
import java.net.*;
import org.mozilla.pluglet.test.basic.*;
import org.mozilla.pluglet.mozilla.*;
import org.mozilla.pluglet.*;

public class PlugletStreamInfo_getLength_0  implements Test
{
   private static int length = 0;
   public static boolean actionPerformed = false;
   /**
    *
    ***********************************************************
    *  Constructor
    ***********************************************************
    *
    */
   public PlugletStreamInfo_getLength_0()
   {
   }


   /**
    *
    ***********************************************************
    *  Starting point of application
    *
    *  @param   args    Array of command line arguments
    *
    ***********************************************************
    */
   public static void main(String[] args)
   {
   }

   /**
    ***********************************************************
    *
    *  Execute Method 
    *
    *  @param   contex  TestContext reference with properties and parameters    
    *  @return          void, the result set to true or false via context
    *
    ***********************************************************
    */
   public void execute(TestContext context)
   {	
	Properties paramProps = null;
	PlugletStreamInfo PlugletStreamInfo_obj = null;
	
	if( context == null ) {
	   TestContext.registerFAILED("ERROR:null context passed in execute(TestContext)" +
				      " of PlugletStreamInfo_getLength_0");
	   return;
	}
	if ( context.failedIsRegistered()) {
	   return;
	}
	PlugletStreamInfo_obj = context.getPlugletStreamInfo();
	if (PlugletStreamInfo_obj == null) {
	   TestContext.registerFAILED("ERROR:null PlugletStreamInfo passed in execute(TestContext)" +
				      " of PlugletStreamInfo_getLength_0");
	   return;
	}
	length = PlugletStreamInfo_obj.getLength();
	actionPerformed = true;
	System.err.println("DEBUG: PlugletStreamInfo_getLength returns \"" + length + "\" value");
   }
   public static void verifySrcLength(String src) 
   {
     System.err.println("Src is:" + src);
     if (src == null) {
       TestContext.registerFAILED("SORRY, but value of src URL isn't passed from JavaScript");
       return;
     }
     try {
       URL sURL = new URL(src);
       //Workaround for starnge JDK1.3.0G(Win32) behaviour when proceed
       //url with file:/// and different drive labels.
       if (sURL.getProtocol().equals("file")) {
	 String file =  sURL.getFile();
	 int index  = file.indexOf("|/");
         if (index != -1) {
	   file = file.charAt(index - 1) + ":" + file.substring(index+1);
	   sURL = new URL("file",null,-1,file);
	 }
       }
       int goodLength = sURL.openConnection().getContentLength();
       if (length!=goodLength) {
	 TestContext.registerFAILED("PlugletStreamInfo_getLength returns \"" + length + 
				    "\" value instead of \"" + goodLength + "\"");
	 return;
       } else {
	 TestContext.registerPASSED("PlugletStreamInfo_getLength returns \"" + length + 
				    "\" value ");
	 return;
       }
     } catch(Exception e) {
       e.printStackTrace();
       TestContext.registerFAILED("Exception " + e );
     }
   }
   /**
    *
    ***********************************************************
    *  Routine where OS specific checks are made. 
    *
    *  @param   os      OS Name (SunOS/Linus/MacOS/...)
    ***********************************************************
    *
    */
   private void osRoutine(String os)
   {
     if(os == null) return;

     os = os.trim();
     if(os.compareTo("SunOS") == 0) {}
     else if(os.compareTo("Linux") == 0) {}
     else if(os.compareTo("Windows") == 0) {}
     else if(os.compareTo("MacOS") == 0) {}
     else {}
   }
}







