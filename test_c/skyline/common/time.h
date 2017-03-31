#ifndef CTIME_H
#define CTIME_H

#include <time.h>

static double nsec = 0;
static double sec = 0;

struct timespec start,end;

struct timespec elapsed(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}

	nsec+= (double)temp.tv_nsec;
	sec+= (double)temp.tv_sec;
	return temp;
}

#endif
