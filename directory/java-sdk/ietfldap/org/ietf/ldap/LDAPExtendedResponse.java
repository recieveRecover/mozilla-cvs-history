/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
package org.ietf.ldap;

import java.io.Serializable;

import org.ietf.ldap.client.opers.JDAPExtendedResponse;

/**
 * Represents a server response to an extended operation request.
 * 
 * @version 1.0
 */
public class LDAPExtendedResponse extends LDAPResponse
                                  implements Serializable {

    static final long serialVersionUID = -3813049515964705320L;

    /**
     * Constructor
     * 
     * @param msgid message identifier
     * @param rsp extended operation response
     * @paarm controls array of controls or null
     */
    LDAPExtendedResponse( int msgid,
                          JDAPExtendedResponse rsp,
                          LDAPControl controls[] ) {
        super(msgid, rsp, controls);
    }
    
    /**
     * Returns the OID of the response
     *
     * @return the response OID
     */
    public String getID() {
        JDAPExtendedResponse result = (JDAPExtendedResponse)getProtocolOp();
        return result.getID();
    }

    /**
     * Returns the raw bytes of the value part of the response
     *
     * @return response as a raw array of bytes
     */
    public byte[] getValue() {
        JDAPExtendedResponse result = (JDAPExtendedResponse)getProtocolOp();
        return result.getValue();
    }
}
