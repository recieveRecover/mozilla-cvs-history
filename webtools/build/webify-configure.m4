dnl The contents of this file are subject to the Netscape Public License
dnl Version 1.0 (the "NPL"); you may not use this file except in
dnl compliance with the NPL.  You may obtain a copy of the NPL at
dnl http://www.mozilla.org/NPL/
dnl
dnl Software distributed under the NPL is distributed on an "AS IS" basis,
dnl WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
dnl for the specific language governing rights and limitations under the
dnl NPL.
dnl
dnl The Initial Developer of this code under the NPL is Netscape
dnl Communications Corporation.  Portions created by Netscape are
dnl Copyright (C) 1999 Netscape Communications Corporation.  All Rights
dnl Reserved.
dnl
divert(-1)dnl Throw away output until we want it
changequote([, ])

dnl webify-configure.m4 - Read in configure.in and spit out
dnl    html appropriate to configure the options.

dnl Send comments, improvements, bugs to Steve Lamm (slamm@netscape.com).


dnl MOZ_ARG_ENABLE_BOOL(           NAME, HELP, IF-YES [, IF-NO [, ELSE]])
dnl MOZ_ARG_DISABLE_BOOL(          NAME, HELP, IF-NO [, IF-YES [, ELSE]])
dnl MOZ_ARG_ENABLE_STRING(         NAME, HELP, IF-SET [, ELSE])
dnl MOZ_ARG_ENABLE_BOOL_OR_STRING( NAME, HELP, IF-YES, IF-NO, IF-SET[, ELSE]]])
dnl MOZ_ARG_WITH_BOOL(             NAME, HELP, IF-YES [, IF-NO [, ELSE])
dnl MOZ_ARG_WITHOUT_BOOL(          NAME, HELP, IF-NO [, IF-YES [, ELSE])
dnl MOZ_ARG_WITH_STRING(           NAME, HELP, IF-SET [, ELSE])
dnl MOZ_READ_MYCONFIG() - Read in 'myconfig.sh' file

define(field_separator,<<fs>>)

dnl MOZ_ARG_PRINT(TYPE, NAME, HELP)
define(MOZ_ARG_PRINT,
[divert(0)dnl
[$1]field_separator[$2]field_separator[$3]
divert(-1)])

dnl MOZ_ARG_ENABLE_BOOL(NAME, HELP, IF-YES [, IF-NO [, ELSE]])
define(MOZ_ARG_ENABLE_BOOL, 
[MOZ_ARG_PRINT(enable,[$1],[$2])])

dnl MOZ_ARG_DISABLE_BOOL(NAME, HELP, IF-NO [, IF-YES [, ELSE]])
define(MOZ_ARG_DISABLE_BOOL,
[MOZ_ARG_PRINT(disable,[$1],[$2])])

dnl MOZ_ARG_ENABLE_STRING(NAME, HELP, IF-SET [, ELSE])
define(MOZ_ARG_ENABLE_STRING,
[MOZ_ARG_PRINT(enable_string,[$1],[$2])])

dnl MOZ_ARG_ENABLE_BOOL_OR_STRING(NAME, HELP, IF-YES, IF-NO, IF-SET[, ELSE]]])
define(MOZ_ARG_ENABLE_BOOL_OR_STRING,
[MOZ_ARG_PRINT(enable_bool_or_string,[$1],[$2])])

dnl MOZ_ARG_WITH_BOOL(NAME, HELP, IF-YES [, IF-NO [, ELSE])
define(MOZ_ARG_WITH_BOOL,
[MOZ_ARG_PRINT(with,[$1],[$2])])

dnl MOZ_ARG_WITHOUT_BOOL(NAME, HELP, IF-NO [, IF-YES [, ELSE])
define(MOZ_ARG_WITHOUT_BOOL,
[MOZ_ARG_PRINT(without,[$1],[$2])])

dnl MOZ_ARG_WITH_STRING(NAME, HELP, IF-SET [, ELSE])
define(MOZ_ARG_WITH_STRING,
[MOZ_ARG_PRINT(with_string,[$1],[$2])])

