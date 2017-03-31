#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "host/rselect_host.h"
#include "host/kskyline_host.h"
#include "common/time.h"


int main(){
	multiDPU_rselect();
	compute_kskyline();

	printf("elapsed host time(sec): %f\n",sec);
	printf("elapsed host time(msec): %f\n",nsec/1000000);
	printf("elapsed host time(nsec): %f\n",nsec);


	return 0;
}
