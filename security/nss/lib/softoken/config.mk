#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code is the Netscape security libraries.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are 
# Copyright (C) 1994-2000 Netscape Communications Corporation.  All
# Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
#

#
#  Override TARGETS variable so that only static libraries
#  are specifed as dependencies within rules.mk.
#

#TARGETS        = $(LIBRARY)
#SHARED_LIBRARY =
#IMPORT_LIBRARY =
#PROGRAM        =


ifdef MOZILLA_CLIENT
DEFINES += -DMOZ_CLIENT
endif

# can't do this in manifest.mn because OS_ARCH isn't defined there.
ifeq ($(OS_ARCH), WINNT)

# don't want the 32 in the shared library name
SHARED_LIBRARY = $(OBJDIR)/$(LIBRARY_NAME)$(LIBRARY_VERSION).dll
IMPORT_LIBRARY = $(OBJDIR)/$(LIBRARY_NAME)$(LIBRARY_VERSION).lib

RES = $(OBJDIR)/$(LIBRARY_NAME).res
RESNAME = $(LIBRARY_NAME).rc

# $(PROGRAM) has explicit dependencies on $(EXTRA_LIBS)
CRYPTOLIB=$(DIST)/lib/freebl.lib
CRYPTODIR=../freebl
ifdef MOZILLA_SECURITY_BUILD
	CRYPTOLIB=$(DIST)/lib/crypto.lib
	CRYPTODIR=../crypto
endif

EXTRA_LIBS += \
	$(CRYPTOLIB) \
	$(DIST)/lib/secutil.lib \
	$(DIST)/lib/dbm.lib \
	$(NULL)

ifdef MOZILLA_BSAFE_BUILD
	EXTRA_LIBS+=$(DIST)/lib/bsafe$(BSAFEVER).lib
endif

EXTRA_SHARED_LIBS += \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plc4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)plds4.lib \
	$(DIST)/lib/$(NSPR31_LIB_PREFIX)nspr4.lib \
	$(NULL)
else

# $(PROGRAM) has explicit dependencies on $(EXTRA_LIBS)
CRYPTOLIB=$(DIST)/lib/libfreebl.$(LIB_SUFFIX)
CRYPTODIR=../freebl
ifdef MOZILLA_SECURITY_BUILD
	CRYPTOLIB=$(DIST)/lib/libcrypto.$(LIB_SUFFIX)
	CRYPTODIR=../crypto
endif
EXTRA_LIBS += \
	$(CRYPTOLIB) \
	$(DIST)/lib/libsecutil.$(LIB_SUFFIX) \
	$(DIST)/lib/libdbm.$(LIB_SUFFIX) \
	$(NULL)
ifdef MOZILLA_BSAFE_BUILD
	EXTRA_LIBS+=$(DIST)/lib/libbsafe.$(LIB_SUFFIX)
endif

# $(PROGRAM) has NO explicit dependencies on $(EXTRA_SHARED_LIBS)
# $(EXTRA_SHARED_LIBS) come before $(OS_LIBS), except on AIX.
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib/ \
	-lplc4 \
	-lplds4 \
	-lnspr4 \
	$(NULL)
endif

ifeq ($(OS_ARCH),SunOS)
ifndef USE_64
ifeq ($(CPU_ARCH),sparc)
# The -R '$ORIGIN' linker option instructs libnss3.so to search for its
# dependencies (libfreebl_*.so) in the same directory where it resides.
MKSHLIB += -R '$$ORIGIN'
endif
endif
endif
