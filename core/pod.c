/*****************************************************************************
 * Copyright (c) OpenLoop, 2016
 *
 * This material is proprietary of The OpenLoop Alliance and its members.
 * All rights reserved.
 * The methods and techniques described herein are considered proprietary
 * information. Reproduction or distribution, in whole or in part, is
 * forbidden except by express written permission of OpenLoop.
 *
 * Source that is published publicly is for demonstration purposes only and
 * shall not be utilized to any extent without express written permission of
 * OpenLoop.
 *
 * Please see http://www.opnlp.co for contact information
 ****************************************************************************/

#include "pod.h"

// Returns the current RealTime Clock value in microseconds
uint64_t get_time_usec() {
  struct timespec tc;
  clock_gettime(CLOCK_REALTIME, &tc);
  
  return (tc.tv_sec * USEC_PER_SEC) + (tc.tv_nsec / NSEC_PER_USEC);
}
void get_timespec(struct timespec *t) {
  int rc = clock_gettime(CLOCK_REALTIME, t);
  if (rc < 0) {
    DECLARE_EMERGENCY("Clock Failure: %d", rc);
  }
}

void timespec_add_us(struct timespec *t, long us) {
  t->tv_nsec += us*1000;
  if (t->tv_nsec > 1000000000) {
    t->tv_nsec = t->tv_nsec - 1000000000;// + ms*1000000;
    t->tv_sec += 1;
  }
}

int timespec_cmp(struct timespec *a, struct timespec *b) {
  if (a->tv_sec > b->tv_sec) return 1;
  else if (a->tv_sec < b->tv_sec) return -1; else if (a->tv_sec == b->tv_sec) {
    if (a->tv_nsec > b->tv_nsec) return 1;
    else if (a->tv_nsec == b->tv_nsec) return 0; else return -1;
  }
  
  return -1;
}

int64_t timespec_to_nsec(struct timespec *t) {
  if (t->tv_sec >= (INT32_MAX - 1) / (long)NSEC_PER_SEC) {
    return -1;
  }
  
  return (t->tv_sec * NSEC_PER_SEC) + t->tv_nsec;
}


void pod_calibrate() {
  pod_t *pod = get_pod();

  set_value_f(&(pod->imu_calibration_x), get_value_f(&(pod->accel_x)));
  set_value_f(&(pod->imu_calibration_y), get_value_f(&(pod->accel_y)));
  set_value_f(&(pod->imu_calibration_z), get_value_f(&(pod->accel_z)));
}

void pod_reset() {
  pod_t *pod = get_pod();

  set_value_f(&(pod->accel_x), 0.0);
  set_value_f(&(pod->accel_y), 0.0);
  set_value_f(&(pod->accel_z), 0.0);
  set_value_f(&(pod->velocity_x), 0.0);
  set_value_f(&(pod->velocity_z), 0.0);
  set_value_f(&(pod->velocity_y), 0.0);
  set_value_f(&(pod->position_x), 0.0);
  set_value_f(&(pod->position_y), 0.0);
  set_value_f(&(pod->position_z), 0.0);
}
