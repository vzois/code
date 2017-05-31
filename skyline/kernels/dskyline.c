//#include "dskyline_v1.h"
#include "dskyline_v2.h"
#include "dskyline_single_dpu.h"
#include "dskyline_multi_dpu.h"

//#define BULK_BENCHMARK

//#define PAR_DSKY

void count_sky_points(uint32_t p){//Debug Only
	uint32_t i = 0, j=0;
	uint32_t pflag_offset = DSKY_FLAGS_ADDR;

	//uint32_t *pflag = pflags[0];
	uint32_t *r = mem_alloc_dma(4);
	uint32_t *rp = mem_alloc_dma(P << 2);
	r[0] = 0;
	for(i = 0;i<p;i++){
		rp[i] = r[0];
		mram_read32(pflag_offset,pflag);
		for(j = 0; j < 8;j++){

			popC(pflag[j],r);
		}
		rp[i] = r[0] - rp[i];

		pflag_offset+=32;
	}
	mram_write128(rp,0);
	debug = r[0];
}

#ifndef PAR_DSKY

int main(){
#ifdef BULK_BENCHMARK
	batch_test();
#else
	seq_test();
#endif
	return 0;
}

#else
int main(){
	multi_dpu();

	return 0;
}

#endif
