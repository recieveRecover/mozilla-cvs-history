/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 * 
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nspr.h"
#include "plgetopt.h"

#include <stdio.h>

#define SEM_NAME1 "/tmp/foo.sem"
#define SEM_NAME2 "/tmp/bar.sem"
#define SEM_MODE  0666
#define ITERATIONS 1000

static PRBool debug_mode = PR_FALSE;
static PRIntn iterations = ITERATIONS;
static PRIntn counter;
static PRSem *sem1, *sem2;

/*
 * Thread 2 waits on semaphore 2 and posts to semaphore 1.
 */
void ThreadFunc(void *arg)
{
    PRIntn i;

    for (i = 0; i < iterations; i++) {
        if (PR_WaitSemaphore(sem2) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
        if (counter == 2*i+1) {
            if (debug_mode) printf("thread 2: counter = %d\n", counter);
        } else {
            fprintf(stderr, "thread 2: counter should be %d but is %d\n",
                    2*i+1, counter);
            exit(1);
        }
        counter++;
        if (PR_PostSemaphore(sem1) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
    }
}

static void Help(void)
{
    fprintf(stderr, "sema test program usage:\n");
    fprintf(stderr, "\t-d           debug mode         (FALSE)\n");
    fprintf(stderr, "\t-c <count>   loop count         (%d)\n", ITERATIONS);
    fprintf(stderr, "\t-h           this message\n");
}  /* Help */

int main(int argc, char **argv)
{
    PRThread *thred;
    PRIntn i;
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "dc:h");

    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt))) {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option) {
            case 'd':  /* debug mode */
                debug_mode = PR_TRUE;
                break;
            case 'c':  /* loop count */
                iterations = atoi(opt->value);
                break;
            case 'h':
            default:
                Help();
                return 2;
        }
    }
    PL_DestroyOptState(opt);

    if (PR_DeleteSemaphore(SEM_NAME1) == PR_SUCCESS) {
        fprintf(stderr, "warning: removed semaphore %s left over "
                "from previous run\n", SEM_NAME1);
    }
    if (PR_DeleteSemaphore(SEM_NAME2) == PR_SUCCESS) {
        fprintf(stderr, "warning: removed semaphore %s left over "
                "from previous run\n", SEM_NAME2);
    }

    sem1 = PR_OpenSemaphore(SEM_NAME1, PR_SEM_CREATE, SEM_MODE, 1);
    if (NULL == sem1) {
        fprintf(stderr, "PR_OpenSemaphore failed (%d, %d)\n",
                PR_GetError(), PR_GetOSError());
        exit(1);
    }
    sem2 = PR_OpenSemaphore(SEM_NAME2, PR_SEM_CREATE, SEM_MODE, 0);
    if (NULL == sem2) {
        fprintf(stderr, "PR_OpenSemaphore failed\n");
        exit(1);
    }
    thred = PR_CreateThread(PR_USER_THREAD, ThreadFunc, NULL,
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (NULL == thred) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }

    /*
     * Thread 1 waits on semaphore 1 and posts to semaphore 2.
     */
    for (i = 0; i < iterations; i++) {
        if (PR_WaitSemaphore(sem1) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
        if (counter == 2*i) {
            if (debug_mode) printf("thread 1: counter = %d\n", counter);
        } else {
            fprintf(stderr, "thread 1: counter should be %d but is %d\n",
                    2*i, counter);
            exit(1);
        }
        counter++;
        if (PR_PostSemaphore(sem2) == PR_FAILURE) {
            fprintf(stderr, "PR_WaitSemaphore failed\n");
            exit(1);
        }
    }

    if (PR_JoinThread(thred) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }

    if (PR_CloseSemaphore(sem1) == PR_FAILURE) {
        fprintf(stderr, "PR_CloseSemaphore failed\n");
    }
    if (PR_CloseSemaphore(sem2) == PR_FAILURE) {
        fprintf(stderr, "PR_CloseSemaphore failed\n");
    }
    if (PR_DeleteSemaphore(SEM_NAME1) == PR_FAILURE) {
        fprintf(stderr, "PR_DeleteSemaphore failed\n");
    }
    if (PR_DeleteSemaphore(SEM_NAME2) == PR_FAILURE) {
        fprintf(stderr, "PR_DeleteSemaphore failed\n");
    }
    printf("PASS\n");
    return 0;
}
