#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>

void printCurrentCPU() {
#ifndef DELIRIUM
	struct rusage myUsage;

	if (getrusage(RUSAGE_SELF, &myUsage)) {
		perror("rusage() failed: ");
		return;
	}

	fprintf(stderr, "Usr[%lu.%06lus]  Sys[%lu.%06lus]  VCS[%lu]  ICS[%lu]\n", 
			myUsage.ru_utime.tv_sec, myUsage.ru_utime.tv_usec,
			myUsage.ru_stime.tv_sec, myUsage.ru_stime.tv_usec,
			myUsage.ru_nvcsw, myUsage.ru_nivcsw);
#endif
}
