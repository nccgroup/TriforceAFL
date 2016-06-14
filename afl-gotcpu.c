/*
   american fuzzy lop - free CPU gizmo
   -----------------------------------

   Written and maintained by Michal Zalewski <lcamtuf@google.com>

   Copyright 2015 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This tool provides a fairly accurate measurement of CPU preemption rate.
   It is meant to complement the quick-and-dirty load average widget shown
   in the afl-fuzz UI. See docs/parallel_fuzzing.txt for more info.

   For some work loads, the tool may actually suggest running more instances
   than you have CPU cores. This can happen if the tested program is spending
   a portion of its run time waiting for I/O, rather than being 100%
   CPU-bound.

   The idea for the getrusage()-based approach comes from Jakub Wilk.

 */

#define AFL_MAIN

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>

#include "types.h"
#include "debug.h"


/* Get unix time in microseconds. */

static u64 get_cur_time_us(void) {

  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);

  return (tv.tv_sec * 1000000ULL) + tv.tv_usec;

}


/* Get CPU usage in microseconds. */

static u64 get_cpu_usage_us(void) {

  struct rusage u;

  getrusage(RUSAGE_SELF, &u);

  return (u.ru_utime.tv_sec * 1000000ULL) + u.ru_utime.tv_usec +
         (u.ru_stime.tv_sec * 1000000ULL) + u.ru_stime.tv_usec;

}


/* Do the benchmark thing. */

int main(int argc, char** argv) {

  static volatile u32 v1, v2;

  s32 loop_repeats = 0, util_perc;
  u64 st_t, en_t, st_c, en_c, real_delta, slice_delta;

  SAYF(cCYA "afl-gotcpu " cBRI VERSION cRST " by <lcamtuf@google.com>\n");

  /* Run a busy loop for CTEST_TARGET_MS. */

  ACTF("Measuring preemption rate (this will take %0.02f sec)...",
       ((double)CTEST_TARGET_MS) / 1000);

  st_t = get_cur_time_us();
  st_c = get_cpu_usage_us();

repeat_loop:

  v1 = CTEST_BUSY_CYCLES;

  while (v1--) v2++;
  sched_yield();

  en_t = get_cur_time_us();

  if (en_t - st_t < CTEST_TARGET_MS * 1000) {
    loop_repeats++;
    goto repeat_loop;
  }

  /* Let's see what percentage of this time we actually had a chance to
     run, and how much time was spent in the penalty box. */

  en_c = get_cpu_usage_us();

  real_delta  = (en_t - st_t) / 1000;
  slice_delta = (en_c - st_c) / 1000;

  OKF("Busy loop hit %u times, real = %llu ms, slice = %llu ms.",
      loop_repeats, real_delta, slice_delta);

  util_perc = real_delta * 100 / slice_delta;

  /* Deliver the final verdict. */

  SAYF(cGRA "\n>>> ");

  if (util_perc < 105) {

    SAYF(cLGN "PASS: " cRST "You can probably run additional processes.");

  } else if (util_perc < 130) {

    SAYF(cYEL "CAUTION: " cRST "Your CPU may be somewhat overbooked (%u%%).",
         util_perc);

  } else {

    SAYF(cLRD "FAIL: " cRST "Your CPU is overbooked (%u%%).", util_perc);

  }

  SAYF(cGRA " <<<" cRST "\n\n");

  return (util_perc > 105) + (util_perc > 130);

}
