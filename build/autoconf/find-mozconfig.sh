#! /bin/sh
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1999 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#

# find-mozconfig.sh - Loads options from .mozconfig onto configure's
#    command-line. The .mozconfig file is searched for in the 
#    order:
#       if $MOZCONFIG is set, use that.
#       Otherwise, use $TOPSRCDIR/.mozconfig
#       Otherwise, use $HOME/.mozconfig
#
topsrcdir=`cd \`dirname $0\`/../..; pwd`

for _config in $MOZCONFIG \
               $MOZ_MYCONFIG \
               $topsrcdir/.mozconfig \
               $topsrcdir/mozconfig \
               $topsrcdir/mozconfig.sh \
               $topsrcdir/myconfig.sh \
               $HOME/.mozconfig \
               $HOME/.mozconfig.sh \
               $HOME/.mozmyconfig.sh
do
  if test -f $_config; then
    echo $_config;
    exit 0
  fi
done
