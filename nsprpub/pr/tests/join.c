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

/***********************************************************************
**
** Name: dbmalloc1.c
**
** Description: Tests PR_SetMallocCountdown PR_ClearMallocCountdown functions.
**
** Modification History:
** 
** 19-May-97 AGarcia - separate the four join tests into different unit test modules.
**		    AGarcia- Converted the test to accomodate the debug_mode flag.
**          The debug mode will print all of the printfs associated with this test.
**		    The regress mode will be the default mode. Since the regress tool limits
**          the output to a one line status:PASS or FAIL,all of the printf statements
**		    have been handled with an if (debug_mode) statement. 
***********************************************************************/

/***********************************************************************
** Includes
***********************************************************************/
/* Used to get the command line option */
#include "plgetopt.h"
#include "prttools.h"

#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef XP_MAC
#include "prlog.h"
#define printf PR_LogPrint
extern void SetupMacPrintfLog(char *logFile);
#endif
/***********************************************************************
** PRIVATE FUNCTION:    Test_Result
** DESCRIPTION: Used in conjunction with the regress tool, prints out the
**		        status of the test case.
** INPUTS:      PASS/FAIL
** OUTPUTS:     None
** RETURN:      None
** SIDE EFFECTS:
**      
** RESTRICTIONS:
**      None
** MEMORY:      NA
** ALGORITHM:   Determine what the status is and print accordingly.
**      
***********************************************************************/


static void Test_Result (int result)
{
    if (result == PASS)
        printf ("PASS\n");
    else
        printf ("FAIL\n");
    exit (1);
}


/*
    Program to test joining of threads.  Two threads are created.  One
    to be waited upon until it has started.  The other to join after it has
    completed.
*/


static void PR_CALLBACK lowPriority(void *arg)
{
}

static void PR_CALLBACK highPriority(void *arg)
{
}

static void PR_CALLBACK unjoinable(void *arg)
{
    PR_Sleep(PR_INTERVAL_NO_TIMEOUT);
}

void runTest(PRThreadScope scope1, PRThreadScope scope2)
{
    PRThread *low,*high;

    /* create the low and high priority threads */
    
    low = PR_CreateThread(PR_USER_THREAD,
                     lowPriority, 0, 
                     PR_PRIORITY_LOW,
                     scope1,
                     PR_JOINABLE_THREAD,
                     0);
    if (!low) {
        if (debug_mode) printf("\tcannot create low priority thread\n");
        else Test_Result(FAIL);
        return;
    }

    high = PR_CreateThread(PR_USER_THREAD,
                     highPriority, 0, 
                     PR_PRIORITY_HIGH,
                     scope2,
                     PR_JOINABLE_THREAD,
                     0);
    if (!high) {
        if (debug_mode) printf("\tcannot create high priority thread\n");
        else Test_Result(FAIL);
        return;
    }

    /* Do the joining for both threads */
    if (PR_JoinThread(low) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot join low priority thread\n");
        else Test_Result (FAIL);
        return;
    } else {
        if (debug_mode) printf("\tjoined low priority thread\n");
    }
    if (PR_JoinThread(high) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot join high priority thread\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tjoined high priority thread\n");
    }
}

void joinWithUnjoinable(void)
{
    PRThread *thread;

    /* create the unjoinable thread */
    
    thread = PR_CreateThread(PR_USER_THREAD,
                     unjoinable, 0, 
                     PR_PRIORITY_NORMAL,
                     PR_GLOBAL_THREAD,
                     PR_UNJOINABLE_THREAD,
                     0);
    if (!thread) {
        if (debug_mode) printf("\tcannot create unjoinable thread\n");
        else Test_Result(FAIL);
        return;
    }

    if (PR_JoinThread(thread) == PR_SUCCESS) {
        if (debug_mode) printf("\tsuccessfully joined with unjoinable thread?!\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tcannot join with unjoinable thread, as expected\n");
        if (PR_GetError() != PR_INVALID_ARGUMENT_ERROR) {
            if (debug_mode) printf("\tWrong error code\n");
            else Test_Result(FAIL);
            return;
        }
    }
    if (PR_Interrupt(thread) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot interrupt unjoinable thread\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tinterrupted unjoinable thread\n");
    }
}

static PRIntn PR_CALLBACK RealMain(int argc, char **argv)
{
    /* The command line argument: -d is used to determine if the test is being run
    in debug mode. The regress tool requires only one line output:PASS or FAIL.
    All of the printfs associated with this test has been handled with a if (debug_mode)
    test.
    Usage: test_name -d
    */
    
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  /* debug mode */
            debug_mode = 1;
            break;
         default:
            break;
        }
    }
    PL_DestroyOptState(opt);

#ifdef XP_MAC
    SetupMacPrintfLog("join.log");
    debug_mode = 1;
#endif

    
    
 /* main test */
    printf("User-User test\n");
    runTest(PR_LOCAL_THREAD, PR_LOCAL_THREAD);
    printf("User-Kernel test\n");
    runTest(PR_LOCAL_THREAD, PR_GLOBAL_THREAD);
    printf("Kernel-User test\n");
    runTest(PR_GLOBAL_THREAD, PR_LOCAL_THREAD);
    printf("Kernel-Kernel test\n");
    runTest(PR_GLOBAL_THREAD, PR_GLOBAL_THREAD);
    printf("Join with unjoinable thread\n");
    joinWithUnjoinable();

    printf("PASSED\n");

    return 0;
}




PRIntn main(PRIntn argc, char *argv[])
{
    PRIntn rv;
    
    PR_STDIO_INIT();
    rv = PR_Initialize(RealMain, argc, argv, 0);
    return rv;
}  /* main */
