/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Netscape Security Services for Java.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2002
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

package org.mozilla.jss.tests;

import org.mozilla.jss.CryptoManager;
import org.mozilla.jss.crypto.CryptoToken;
import org.mozilla.jss.crypto.SecretDecoderRing;
import org.mozilla.jss.util.ConsolePasswordCallback;
import java.io.*;

public class SDR {

    public static void main(String[] args) {

      try {
        CryptoManager.initialize(".");

        String cmd = args[0];
        String infile = args[1];
        String outfile = args[2];

        CryptoManager cm = CryptoManager.getInstance();
        CryptoToken token = cm.getInternalKeyStorageToken();
        token.login(new ConsolePasswordCallback());

        SecretDecoderRing sdr = new SecretDecoderRing();

        FileInputStream fis = new FileInputStream(infile);
        ByteArrayOutputStream bos = new ByteArrayOutputStream();

        int numread;
        byte[] data = new byte[1024];
        while( (numread = fis.read(data)) != -1 ) {
            bos.write(data, 0, numread);
        }
        byte[] inputBytes = bos.toByteArray();

        byte[] outputBytes;
        if( cmd.equalsIgnoreCase("encrypt") ) {
               outputBytes = sdr.encrypt(inputBytes);
        } else {
                outputBytes = sdr.decrypt(inputBytes);
        }

        FileOutputStream fos = new FileOutputStream(outfile);
        fos.write(outputBytes);

      } catch(Exception e) {
        e.printStackTrace();
        System.exit(1);
      }
    }

    private static char[] hex = new char[]
        { '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    private static void dumpBytes(byte[] bytes) {
        for(int i=0; i < bytes.length; ++i) {
            System.out.print(hex[(bytes[i] & 0xf0)>>4] +
                hex[(bytes[i] & 0xf)] + " ");
        }
        System.out.println();
    }
}
