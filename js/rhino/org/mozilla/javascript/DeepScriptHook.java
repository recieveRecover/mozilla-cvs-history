/* -*- Mode: java; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Rhino code, released
 * May 6, 1998.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1997-1999 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * John Bandhauer
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the NPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the NPL or the GPL.
 */

package org.mozilla.javascript;

/**
 * This interface supports low-level script load/unload debug hooks.
 * <p>
 * This interface can be implemented and used to hook script loading 
 * and unloading. org.mozilla.javascript.debug.IDebugManager uses this 
 * system and provides a higher level abstraction more appropriate 
 * as a debugger API.
 *
 * @see org.mozilla.javascript.Context#setScriptHook
 * @see org.mozilla.javascript.debug.IDebugManager
 * @author John Bandhauer
 */

public interface DeepScriptHook
{
    /**
     * Notification that a script is being loaded.
     *
     * @param cx current context
     * @param obj script or function being loaded
     */
    public void loadingScript(Context cx, NativeFunction obj);

    /**
     * Notification that a script is being unloaded.
     * <p>
     * NOTE: this currently happens as part of the Java garbage 
     * collection process; i.e. it is called by the finalize method
     * of the script and is thus likely to be called on a system 
     * thread. Also, this is thus not likely to be called if there are 
     * any outstanding references to the script.
     *
     * @param cx context which originally loaded the script
     * @param obj script or function being unloaded
     */
    public void unloadingScript(Context cx, NativeFunction obj);
}
