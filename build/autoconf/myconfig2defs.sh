#! /bin/sh
#
# The contents of this file are subject to the Netscape Public License
# Version 1.0 (the "NPL"); you may not use this file except in
# compliance with the NPL.  You may obtain a copy of the NPL at
# http://www.mozilla.org/NPL/
#
# Software distributed under the NPL is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
# for the specific language governing rights and limitations under the
# NPL.
#
# The Initial Developer of this code under the NPL is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1999 Netscape Communications Corporation.  All Rights
# Reserved.
#

# myconfig2defs.sh - Translates myconfig.sh into options for client.mk.
#    Prints defines to stdout.
#
# See load-myconfig.sh for more details
#
# Send comments, improvements, bugs to Steve Lamm (slamm@netscape.com).


# This functions needs to be kept in sync with the code in altoptions.sh.
# Eventually I will put it in some common place.
find_myconfig() {
  for _config in $MOZ_MYCONFIG \
                 ./myconfig.sh \
                 $_topsrcdir/myconfig.sh \
                 $HOME/.mozmyconfig.sh
  do
    if test -f $_config; then
      MOZ_MYCONFIG=$_config;
      break
    fi
  done
}

ac_add_options() {
  for _opt
  do
    # Escape shell characters, space, tab, dollar, quote, backslash.
    _opt=`echo $_opt | sed -e 's/\([\ \	\$\"\\]\)/\\\\\1/g;'`

    myconfig_ac_options="$myconfig_ac_options $_opt"
  done
}

mk_add_options() {
  for _opt
  do
    # Escape shell characters, space, tab, dollar, quote, backslash.
    _opt=`echo $_opt | sed -e 's/\([\ \	\$\"\\]\)/\\\\\1/g;'`

    myconfig_mk_options="$myconfig_mk_options $_opt"
  done
}

ac_echo_options() {
  echo "# gmake"
  echo "# This file is automatically generated for client.mk."
  echo "# Do not edit. Edit $MOZ_MYCONFIG instead."
  echo
  eval "set -- $myconfig_ac_options"
  for _opt
  do
    case "$_opt" in
      --with-nspr=* )     echo MOZ_WITH_NSPR=`expr $_opt : ".*=\(.*\)"` ;;
      --with-pthreads* )  echo MOZ_WITH_PTHREADS=1 ;;
      --*-* )             echo "# $_opt is not used by client.mk" ;;
    esac
  done
  eval "set -- $myconfig_mk_options"
  for _opt
  do
    _opt=`echo $_opt | sed -e 's/@\([^@]*\)@/\$(\1)/g'`
    echo $_opt
  done
}

#
# Define load the options
#
myconfig_ac_options=
myconfig_mk_options=
out_file=$1
tmp_file="$out_file-tmp$$"

trap "rm -f $tmp_file; exit 1" 1 2 15

find_myconfig

if [ ! -f $out_file ]; then
  echo "# This file is automatically generated for client.mk." > $out_file
fi

if [ "$MOZ_MYCONFIG" ]
then
  . $MOZ_MYCONFIG
  ac_echo_options > $tmp_file
  if cmp -s $tmp_file $out_file; then
    rm $tmp_file
  else
    mv $tmp_file $out_file
  fi
fi
