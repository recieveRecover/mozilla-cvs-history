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
# Copyright (C) 1998 Netscape Communications Corporation.  All Rights
# Reserved.
#

#
# Configuration common to all versions of Windows NT
# and Windows 95
#

DEFAULT_COMPILER = cl

CC           = cl
CCC          = cl
LINK         = link
AR           = lib
AR          += -NOLOGO -OUT:"$@"
RANLIB       = echo
BSDECHO      = echo
NSINSTALL_DIR  = $(GDEPTH)/gconfig/nsinstall
NSINSTALL      = nsinstall
INSTALL      = $(NSINSTALL)
MAKE_OBJDIR  = mkdir
MAKE_OBJDIR += $(OBJDIR)
RC           = rc.exe
GARBAGE     += $(OBJDIR)/vc20.pdb $(OBJDIR)/vc40.pdb
MAPFILE      = $(PROGRAM:.exe=.map)
MAP          = /MAP:$(MAPFILE)
PDBFILE      = $(PROGRAM:.exe=.pdb)
PDB          = /PDB:$(PDBFILE)
IMPFILE      = $(PROGRAM:.exe=.lib)
IMP          = /IMPLIB:$(IMPFILE)
XP_DEFINE   += -DXP_PC
LIB_SUFFIX   = lib
DLL_SUFFIX   = dll
OUT_NAME     = -out:
ARCHIVE_SUFFIX = _s
NATIVE_PLATFORM=win
NATIVE_RAPTOR_WIDGET =
NATIVE_RAPTOR_GFX = raptorgfxwin
NATIVE_RAPTOR_WEB=raptorweb
RAPTOR_GFX=
VERSION_NUMBER=50
NATIVE_JULIAN_DLL=jul$(MOZ_BITS)$(VERSION_NUMBER)
PLATFORM_DIRECTORY=windows
NATIVE_ZLIB_DLL=zip$(MOZ_BITS)$(VERSION_NUMBER)
NATIVE_XP_DLL=xplib
XP_PREF_DLL=xppref$(MOZ_BITS)

ifdef RCFILE
RCFILE       := $(RCFILE).rc
RESFILE      = $(OBJDIR)/$(RCFILE:.rc=.res)
endif

OS_LIBS = gdi32.lib kernel32.lib advapi32.lib user32.lib
MATH_LIB=

GUI_LIBS = 
NSPR_LIBS = libplds21 libplc21 libnspr21 libmsgc21
OPT_SLASH = /
LIB_PREFIX      =
XP_REG_LIB      = libreg$(MOZ_BITS)


ifdef BUILD_OPT
	OS_CFLAGS  += -MD
	OPTIMIZER  += -O2
	DEFINES    += -UDEBUG -U_DEBUG -DNDEBUG
	DLLFLAGS   += -OUT:"$@"
	LDFLAGS    += /SUBSYSTEM:WINDOWS /NOLOGO
else
	#
	# Define USE_DEBUG_RTL if you want to use the debug runtime library
	# (RTL) in the debug build
	#
	ifdef USE_DEBUG_RTL
		OS_CFLAGS += -MDd
	else
		OS_CFLAGS += -MD
	endif
	OPTIMIZER  += -Od -Z7
	#OPTIMIZER += -Zi -Fd$(OBJDIR)/ -Od
	DEFINES    += -DDEBUG -D_DEBUG -UNDEBUG
	DLLFLAGS   += -DEBUG -DEBUGTYPE:CV -OUT:"$@"
	LDFLAGS    += -DEBUG -DEBUGTYPE:BOTH  /SUBSYSTEM:WINDOWS /NOLOGO
endif

# XXX FIXME: I doubt we use this.  It is redundant with
# SHARED_LIBRARY.
ifdef DLL
	DLL := $(addprefix $(OBJDIR)/, $(DLL))
endif

DEFINES += -DWIN32

#
#  The following is NOT needed for the NSPR 2.0 library.
#

DEFINES += -D_WINDOWS

#
#  The default is no MFC anywhere
#

DEFINES += -DWIN32_LEAN_AND_MEAN


