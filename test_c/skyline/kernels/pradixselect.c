#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

uint32_t bins[TASKLETS][BINS];

void count_select(uint8_t id){
	uint32_t low = id<<6;
	uint32_t *buffer = dma_alloc(BUFFER);
	uint32_t i = 0;
	uint8_t j = 0;

	mram_ll_read32(BUCKET_ADDR,buffer);
	uint32_t num = buffer[0];//k-th value digits
	uint8_t shf = buffer[1];//shift amount
	uint32_t mD = buffer[2];//digit being examined mask
	uint32_t mN = buffer[3];//digits discovered mask

	/*num = 0;
	shf = 4 << 2;
	mD = 0xF << shf;
	mN = 0x0;*/

	for(j = 0 ; j < BINS; j++) bins[id][j] = 0;
	for(i=low;i<N;i+=1024){//count digit for part of input// Read 1024 elements per iteration (64 per tasklet)//
		mram_ll_read256(DATA_ADDR + ( i<<2 ),buffer);//Does this improve memory read <?>
		for(j=0;j<64;j++){
			if((buffer[j] & mN) == num ){
				uint8_t digit = (buffer[j] & mD) >> shf;
				bins[id][digit+1]+=1;
			}
		}
	}
	barrier_wait(id);
	for(j = 1;j < 16;j++){
		bins[ACC_BUCKET][id+1]+=bins[j][id+1];
	}
	barrier_wait(id);
	if(id < 1){
		for(i = 0;i<17;i++) buffer[i]=bins[ACC_BUCKET][i];
		mram_ll_write128(buffer,BUCKET_ADDR);
	}
}

void count_select_1024(uint8_t id){
	uint32_t low = id<<8;
	uint32_t *buffer = dma_alloc(BUFFER);
	uint32_t i = 0;
	uint16_t j = 0;

	mram_ll_read32(BUCKET_ADDR,buffer);
	uint32_t num = buffer[0];//k-th value digits
	uint8_t shf = buffer[1];//shift amount
	uint32_t mD = buffer[2];//digit being examined mask
	uint32_t mN = buffer[3];//digits discovered mask

	/*num = 0;
	shf = 4 << 2;
	mD = 0xF << shf;
	mN = 0x0;*/

	for(j = 0 ; j < BINS; j++) bins[id][j] = 0;
	for(i=low;i<N;i+=4096){//count digit for part of input// Read 1024 elements per iteration (64 per tasklet)//
		mram_ll_read1024_new(DATA_ADDR + ( i<<2 ),buffer);//Does this improve memory read <?>
		for(j=0;j<256;j++){
			if((buffer[j] & mN) == num ){
				uint8_t digit = (buffer[j] & mD) >> shf;
				bins[id][digit+1]+=1;
			}
		}
	}
	barrier_wait(id);
	for(j = 1;j < 16;j++){
		bins[ACC_BUCKET][id+1]+=bins[j][id+1];
	}
	barrier_wait(id);
	if(id < 1){
		for(i = 0;i<17;i++) buffer[i]=bins[ACC_BUCKET][i];
		mram_ll_write128(buffer,BUCKET_ADDR);
	}
}

void make_window(uint32_t num){//Extract Indices to form the window
	uint32_t *buffer = dma_alloc(256);
	uint32_t *window = dma_alloc(256);
	uint32_t *tmp = dma_alloc(4);

	uint32_t addr = DATA_ADDR;
	uint32_t i = 0 , j = 0;
	uint32_t k;

	*tmp = num;

	for(i=0;i<N;i+=64){
		mram_ll_read256(DATA_ADDR + ( i<<2 ),buffer);
		for(j=0;j<64;j++){
			if(buffer[j] < num){
				window[k & 0x3F] = i+j;
				k++;
				if ((k & 0x3F) == 0){//write indices using a round robin buffer
					mram_ll_write256(window,addr);
					addr+=(k << 2);
				}
			}
		}
	}

	mram_ll_write256(window,addr);
	*tmp = k;
	mram_ll_write8(tmp,VAR_0_ADDR);
}

int main(){
	uint8_t id = me();
	uint32_t *arg = dma_alloc(32);
	mram_ll_read32(VAR_0_ADDR,arg);

	if( arg[0] == CSELECT){//Choose radixselect counting
		if(BUFFER == 256) count_select(id);
		if(BUFFER == 1024) count_select_1024(id);
	}else if(arg[0] == CWINDOW){//Create window with indices
		if(id < 1){
			make_window(arg[1]);
			//if(BUFFER == 1024) make_window_1024(arg[1]);
		}
	}

	return 0;
}
