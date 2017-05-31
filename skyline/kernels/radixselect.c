#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

uint32_t bins[TASKLETS][BINS];
uint32_t num;
uint8_t digit;

uint32_t rselect(uint8_t id){
	uint32_t low = (id * N) / TASKLETS;
	uint32_t high = ((id+1) * N) / TASKLETS;
	uint32_t *buffer = dma_alloc(BUFFER);

	uint32_t mN;
	uint32_t mD;
	uint8_t shf;
	//if(id < 1){
		num = 0;
		mN = 0;
		mD = 0;
	//}
	int8_t d;
	uint32_t tmpK = K;
	uint32_t count = 0;
	uint32_t i = 0;
	uint8_t j = 0;

	for( d = 7; d > -1; d-- ){//iterate through
		//if(id < 1){
		shf = d << 2;
		mD = (0xF << shf);
		//}
		//barrier_wait(id);

		for(j = 0 ; j < BINS; j++) bins[id][j] = 0;
		for(i=low;i<high;i+=64){//count digit for part of input
			mram_ll_read256(RANK_ADDR + ( i<<2 ),buffer);
			for(j=0;j<64;j++){
				digit = (buffer[j] & mD) >> shf;
				if((buffer[j] & mN) == num ){
					bins[id][digit+1]+=1;
				}
			}
		}

		//Accumulate Results
		barrier_wait(id);
		for(j = 1;j < 16;j++){
			bins[ACC_BUCKET][id+1]+=bins[j][id+1];
		}
		barrier_wait(id);

		//Identify digit of k-smallest number//
		if (id < 1){
			for(j = 1;j < 16;j++){
				bins[ACC_BUCKET][j+1]+=bins[ACC_BUCKET][j];
			}
			digit = 0x0;
			for(j = 1;j < 17;j++){
				if (tmpK <= bins[ACC_BUCKET][j]){
					digit = j;
					break;
				}
			}

			if (tmpK != bins[ACC_BUCKET][j-1]){ tmpK = tmpK - bins[ACC_BUCKET][j-1]; }
			num = num | ((digit-1) << shf );
		}
		mN = mN | mD;
		barrier_wait(id);
		for(j=1;j<digit;j++) count+=bins[id][j];// count numbers in my own partition
	}

	//Start Compacting elements//
	bins[0][id] = count;
	barrier_wait(id);
	if(id < 1){
		count=0;
		bins[0][16] = K-1;
		bins[0][0] = 0;
		for(j=15;j>0;j--){
			bins[0][j] = bins[0][j+1] - bins[0][j];
		}

		for(j = 0 ; j < 16; j++){
			buffer[j] = bins[0][j];
		}
		mram_ll_write64(buffer,BUCKET_ADDR);
		buffer[0] = num;
		buffer[1] = 0x0;
		mram_ll_write8(buffer,RANK_ADDR);
	}

	return count;
}


int main(){
	uint8_t id = me();
	//uint32_t count = rselect(id);
	count_select(id);

	return 0;
}
