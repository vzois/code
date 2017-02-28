//#include "dskyline_v1.h"
#include "dskyline_v2.h"

void count_sky_points(uint32_t p){//Debug Only
	uint32_t i = 0, j=0;
	uint32_t pflag_offset = FLAGS_ADDR;

	//uint32_t *pflag = pflags[0];
	uint32_t *r = dma_alloc(4);
	uint32_t *rp = dma_alloc(P << 2);
	r[0] = 0;
	for(i = 0;i<p;i++){
		rp[i] = r[0];
		mram_ll_read32(pflag_offset,pflag);
		for(j = 0; j < 8;j++){

			popc(pflag[j],r);
		}
		rp[i] = r[0] - rp[i];

		pflag_offset+=32;
	}
	mram_ll_write128(rp,0);
	debug = r[0];
}

/*int main2(){
	uint8_t id = me();
	init(id);
	barrier_wait(id);

	//cmp_part(id,0,0);

	uint32_t p = P;
	cmp_part(id,0,0);
	for(i = 1;i<p;i++){
		for(j = 0;j<i;j++){
			//if(i == 2 && j == 1) break;
			cmp_part(id,j,i);
		}
		cmp_part(id,i,i);
	}
	if (id == 0) count_sky_points(p);

	return 0;
}*/

int main(){
	uint8_t id = me();
	init_v2(id);
	barrier_wait(id);
	int16_t i = 0 , j = 0;

	cmp_part_4d(id,0,0);


	return 0;
}
