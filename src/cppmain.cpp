/**
 * @file cppmain.cpp
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

#include <thread>
#include <mutex>

#define TRIG_TIMEOUT 1

volatile sig_atomic_t done = 0;
volatile sig_atomic_t ready = 0;

void get_current_fname(char *ret)
{
    if (ret == NULL)
        return;
    char *tmp = (char *) malloc(strlen(__FILE__) + 1);
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

void thread_fcn_rl(std::mutex mutex) // remote lock, on both recurse and non recurse mutex
{
    uint64_t count = 0;
    struct timespec start, end, diff;
    ready = 1; // indicate main thread that we are ready to proceed
    // here, main thread will lock the mutex
    while (ready); // wait until main thread unsets ready
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (!done) // keep going until main stops you
    {
        mutex.try_lock();
        count++;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Lock Attempts: %" PRIu64 " in %ld.%09ld s", count, diff.tv_sec, diff.tv_nsec);
    return;
}

void thread_fcn_rlr(std::recursive_mutex mutex) // remote lock, on both recurse and non recurse mutex
{
    uint64_t count = 0;
    struct timespec start, end, diff;
    ready = 1; // indicate main thread that we are ready to proceed
    // here, main thread will lock the mutex
    while (ready); // wait until main thread unsets ready
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (!done) // keep going until main stops you
    {
        mutex.try_lock();
        count++;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Lock Attempts: %" PRIu64 " in %ld.%09ld s", count, diff.tv_sec, diff.tv_nsec);
    return;
}

void thread_fcn_sl(std::recursive_mutex mutex) // self lock, only on recursive
{
    uint32_t i_ = INT32_MAX / 100, i = i_;
    struct timespec start, end, diff;
    mutex.lock();
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (i--) // keep going until main stops you
    {
        mutex.try_lock();
    }
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Lock Attempts: %" PRIu32 " in %ld.%09ld s", i_, diff.tv_sec, diff.tv_nsec);
    i = i_;
    clock_gettime(CLOCK_REALTIME, &start); // get current time
    while (i--)
    {
        mutex.unlock();
    }
    mutex.unlock();
    clock_gettime(CLOCK_REALTIME, &end);
    timespec_diff(&start, &end, &diff);
    bprintlf(BLUE_FG "Unlock Attempts: %" PRIu32 " in %ld.%09ld s", i_, diff.tv_sec, diff.tv_nsec);
    return;
}

int main()
{
    char fname[512];
    get_current_fname(fname);
    bprintlf(GREEN_FG "Program: %s", fname);
    struct timespec start, stop, result;

    // CASE 1
    std::mutex m;
    std::thread thr(thread_fcn_rl, m);
    while (!ready); // wait until slave signals ready
    m.lock();
    ready = 0;
    sleep(TRIG_TIMEOUT);
    done = 1;
    thr.join();
    m.unlock();
    // CASE 2
    std::recursive_mutex m_;
    done = 0;
    std::thread thr2(thread_fcn_rlr, m_);
    while (!ready); // wait until slave signals ready
    m_.lock();
    ready = 0;
    sleep(TRIG_TIMEOUT);
    thr2.join();
    m_.unlock();

    // CASE 3
    std::thread thr3(thread_fcn_sl, m_);
    thr3.join();

    // CLEANUP

    clock_gettime(CLOCK_REALTIME, &stop);
    timespec_diff(&start, &stop, &result);
    bprintf(BLUE_FG "[%s] Program Elapsed Time: %ld.%09ld ", fname, result.tv_sec, result.tv_nsec);

    return 0;
}