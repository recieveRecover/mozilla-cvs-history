/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1998-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

/*
 * SCO ODT 5.0 - originally created by mikep
 */
#include "primpl.h"

#include <setjmp.h>

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
}

#ifdef ALARMS_BREAK_TCP /* I don't think they do */

PRInt32 _MD_connect(PRInt32 osfd, PRNetAddr *addr, PRInt32 addrlen,
                        PRIntervalTime timeout)
{
    PRInt32 rv;

    _MD_BLOCK_CLOCK_INTERRUPTS();
    rv = _connect(osfd,addr,addrlen);
    _MD_UNBLOCK_CLOCK_INTERRUPTS();
}

PRInt32 _MD_accept(PRInt32 osfd, PRNetAddr *addr, PRInt32 addrlen,
                        PRIntervalTime timeout)
{
    PRInt32 rv;

    _MD_BLOCK_CLOCK_INTERRUPTS();
    rv = _accept(osfd,addr,addrlen);
    _MD_UNBLOCK_CLOCK_INTERRUPTS();
    return(rv);
}
#endif

/*
 * These are also implemented in pratom.c using NSPR locks.  Any reason
 * this might be better or worse?  If you like this better, define
 * _PR_HAVE_ATOMIC_OPS in include/md/unixware.h
 */
#ifdef _PR_HAVE_ATOMIC_OPS
/* Atomic operations */
#include  <stdio.h>
static FILE *_uw_semf;

void
_MD_INIT_ATOMIC(void)
{
    /* Sigh.  Sure wish SYSV semaphores weren't such a pain to use */
    if ((_uw_semf = tmpfile()) == NULL)
        PR_ASSERT(0);

    return;
}

void
_MD_ATOMIC_INCREMENT(PRInt32 *val)
{
    flockfile(_uw_semf);
    (*val)++;
    unflockfile(_uw_semf);
}

void
_MD_ATOMIC_ADD(PRInt32 *ptr, PRInt32 val)
{
    flockfile(_uw_semf);
    (*ptr) += val;
    unflockfile(_uw_semf);
}

void
_MD_ATOMIC_DECREMENT(PRInt32 *val)
{
    flockfile(_uw_semf);
    (*val)--;
    unflockfile(_uw_semf);
}

void
_MD_ATOMIC_SET(PRInt32 *val, PRInt32 newval)
{
    flockfile(_uw_semf);
    *val = newval;
    unflockfile(_uw_semf);
}
#endif

void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}

/* These functions should not be called for SCO */
void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for SCO.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for SCO.");
}
