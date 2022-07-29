/**
 * @file ccmain.cpp
 * @author Sunip K. Mukherjee (sunipkmukherjee@gmail.com)
 * @author Mit Bailey (mitbailey@outlook.com)
 * @brief
 * @version See Git tags for version information.
 * @date 2022.07.05
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <time.h>
#include "meb_print.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <limits.h>

#define TRIG_TIMEOUT 1
#define TRIALS 32

volatile sig_atomic_t done = 0;
volatile sig_atomic_t ready = 0;

void get_current_fname(char *ret)
{
    if (ret == NULL)
        return;
    char *tmp = (char *)malloc(strlen(__FILE__) + 1);
    if (tmp == NULL)
        return;
    strcpy(tmp, __FILE__);
    char *tok = strtok(tmp, "/");
    dbprintlf("%s", tok);
    while (tok != NULL)
    {
        strcpy(ret, tok);
        tok = strtok(NULL, "/");
    }
    free(tmp);
}

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0)
    {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000L;
    }
    else
    {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

void *thread_fcn_rl(void *_mutex) // remote lock, on both recurse and non recurse mutex
{
    FILE *fp = fopen("ccmain_rl.data", "a");
    if (fp == NULL)
    {
        dbprintlf(FATAL "Failed to open file.");
        exit(1);
    }

    uint64_t count = 0;
    struct timespec start, end, diff;
    pthread_mutex_t *mutex = (pthread_mutex_t *)_mutex;
    if (mutex == NULL)
    {
        done = 1;
        return NULL;
    }
    ready = 1; // indicate main thread that we are ready to proceed
    // here, main thread will lock the mutex
    while (ready)
        ;                                  // wait until main thread unsets ready
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (!done)                          // keep going until main stops you
    {
        pthread_mutex_trylock(mutex);
        count++;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Lock Attempts: %" PRIu64 " in %ld.%09ld s", count, diff.tv_sec, diff.tv_nsec);
    fprintf(fp, "%ld.%09ld, %" PRIu64 ", %ld.%09ld\n", start.tv_sec, start.tv_nsec, count, diff.tv_sec, diff.tv_nsec);
    return NULL;
}

void *thread_fcn_sl(void *_mutex) // self lock, only on recursive
{
    FILE *fp_locks = fopen("cmain_sl_locks.data", "a");
    if (fp_locks == NULL)
    {
        dbprintlf(FATAL "Failed to open file.");
        exit(2);
    }
    FILE *fp_unlocks = fopen("cmain_sl_unlocks.data", "a");
    if (fp_unlocks == NULL)
    {
        dbprintlf(FATAL "Failed to open file.");
        exit(3);
    }

    uint32_t i_ = INT32_MAX / 100, i = i_;
    struct timespec start, end, diff;
    pthread_mutex_t *mutex = (pthread_mutex_t *)_mutex;
    if (mutex == NULL)
    {
        done = 1;
        return NULL;
    }
    pthread_mutex_lock(mutex);             // lock your recursive mutex
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (i--)                            // keep going until main stops you
    {
        pthread_mutex_trylock(mutex);
    }
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Lock Attempts: %" PRIu32 " in %ld.%09ld s", i_, diff.tv_sec, diff.tv_nsec);
    fprintf(fp_locks, "%ld.%09ld, %" PRIu64 ", %ld.%09ld\n", start.tv_sec, start.tv_nsec, i_, diff.tv_sec, diff.tv_nsec);

    i = i_;
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (i--)
    {
        pthread_mutex_unlock(mutex);
    }
    pthread_mutex_unlock(mutex);
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Unlock Attempts: %" PRIu32 " in %ld.%09ld s", i_, diff.tv_sec, diff.tv_nsec);
    fprintf(fp_unlocks, "%ld.%09ld, %" PRIu64 ", %ld.%09ld\n", start.tv_sec, start.tv_nsec, i_, diff.tv_sec, diff.tv_nsec);
    return NULL;
}

int main()
{
    char fname[512];
    get_current_fname(fname);
    bprintlf(GREEN_FG "Program: %s", fname);
    struct timespec start, stop, result;
    pthread_mutex_t m;
    pthread_mutexattr_t attr;
    pthread_t thread;
    clock_gettime(CLOCK_REALTIME, &start);

    dbprintlf(UNDER_ON "CASE ONE");
    for (int i = 0; i < TRIALS; i++)
    {
        pthread_mutexattr_init(&attr);

        // CASE 1
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_DEFAULT);
        pthread_mutex_init(&m, &attr);

        pthread_create(&thread, NULL, &thread_fcn_rl, &m);
        while (!ready)
            ; // wait until slave signals ready
        pthread_mutex_lock(&m);
        ready = 0;
        sleep(TRIG_TIMEOUT);
        done = 1;
        pthread_join(thread, NULL);
        pthread_mutex_unlock(&m);
        pthread_mutex_destroy(&m);

        ready = 0;
        done = 0;
    }

    dbprintlf(UNDER_ON "CASE TWO");
    for (int i = 0; i < TRIALS; i++)
    {
        // CASE 2
        done = 0;
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m, &attr);

        pthread_create(&thread, NULL, &thread_fcn_rl, &m);
        while (!ready)
            ; // wait until slave signals ready
        pthread_mutex_lock(&m);
        ready = 0;
        sleep(TRIG_TIMEOUT);
        done = 1;
        pthread_join(thread, NULL);
        pthread_mutex_unlock(&m);
        pthread_mutex_destroy(&m);
        ready = 0;
        done = 0;
    }

    // CASE 3
    dbprintlf(UNDER_ON "CASE THREE");
    for (int i = 0; i < TRIALS; i++)
    {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m, &attr);

        pthread_create(&thread, NULL, &thread_fcn_sl, &m);
        pthread_join(thread, NULL);
        pthread_mutex_destroy(&m);
        ready = 0;
        done = 0;
    }

    // CLEANUP

    clock_gettime(CLOCK_REALTIME, &stop);
    timespec_diff(&start, &stop, &result);
    bprintlf(BLUE_FG "[%s] Program Elapsed Time: %ld.%09ld ", fname, result.tv_sec, result.tv_nsec);

    return 0;
}