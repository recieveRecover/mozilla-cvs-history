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
 * Client QA Team, St. Petersburg, Russia
 */


 
package org.mozilla.webclient.test.basic.api;

/*
 * History_forward.java
 */

import org.mozilla.webclient.test.basic.*;
import org.mozilla.webclient.*;
import java.util.StringTokenizer;
import java.net.URL;
import java.net.MalformedURLException;

public class History_forward implements Test
{
    
    private TestContext context = null;
    private BrowserControl browserControl = null;
    private History history = null;
    private int currentArg = 0;
    private CurrentPage curPage = null;
    private String[] pages = null;

    public boolean initialize(TestContext context) {
        this.context = context;
        this.browserControl = context.getBrowserControl();
        this.pages = context.getPagesToLoad();
        this.currentArg = (new Integer(context.getCurrentTestArguments())).intValue();
        try {
            history = (History)
                browserControl.queryInterface(BrowserControl.HISTORY_NAME);
        }catch (Exception e) {
            TestContext.registerFAILED("Exception: " + e.toString());
            return false;
		}
        return true;        
    }
    public void execute() {      
        if(context.getExecutionCount()==1){ 
            try {
                if(pages.length==0){
                    context.setDefaultResult(TestContext.PASSED);
                    context.setDefaultComment("Do FORWARD when history is empty.");
                    history.forward();
                    return ;
                }else{
                    context.setDefaultResult(TestContext.FAILED);
                    context.setDefaultComment("Hangs when load page N " + (currentArg + 1) + 
                                              " of " + pages.length);
                    history.setCurrentHistoryIndex(currentArg);
                    return;
                }
            }catch(Exception e) {
                 TestContext.registerFAILED("Exception during execution: " + e.toString());
                 return;
            }
        }
        if(context.getExecutionCount()==2){
            try{
                if(currentArg<pages.length-1){
                    context.setDefaultResult(TestContext.FAILED);
                    context.setDefaultComment("Hangs when do forward and forward list isn't empty.");
                    history.forward();
                }else{
                    context.setDefaultResult(TestContext.PASSED);
                    context.setDefaultComment("Try to do forward when forward list is empty.");
                    history.forward();
                }
                return;
            }catch(Exception e){
                TestContext.registerFAILED("Exception during execution: " + e.toString());
                return;
            }
        }
        if(context.getExecutionCount()==3){
             try{
                 curPage = (CurrentPage)browserControl.queryInterface(BrowserControl.CURRENT_PAGE_NAME);
                 try{
                     if(new URL(curPage.getCurrentURL()).equals(new URL(pages[currentArg+1]))){
                         TestContext.registerPASSED("forward load correct URL when it's exist");
                         return ;
                     }else{
                         TestContext.registerFAILED("back load incorrect URL when it's exist");
                         return ;
                     }
                 }catch(MalformedURLException e) {
                        TestContext.registerFAILED("Exception when constructing URL from: " + curPage.getCurrentURL() + 
                                                   " and " + pages[currentArg+1]);
                 }
             }catch(Exception e){
                TestContext.registerFAILED("Exception during execution: " + e.toString());
                return;
            }
        }
    }
   
}




