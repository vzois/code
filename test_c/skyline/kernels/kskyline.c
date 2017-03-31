/*
 * Compute skyline using a window created from using k-selection.
 */

#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"


// WRAM_BUFFER_SHF = log2 (WRAM_BUFFER)
// window size = (K*D*4)
// WRAM_BUFFER IS STATIC - BASED ON ABILITY OF TASKLET TO RESERVE WRAM
// WRAM_PER_TASKLET = SHARED_WRAM + LOCAL_WRAM
// SHARED_WRAM = WRAM_BUFFER <?> How large without causing errors? This is the maximum window_size
// LOCAL_WRAM = Local variables, buffers, counters
// TASKLET_LOAD = How much of the shared ram is going to be reserved? How many tasklets participate in the load?
// TASKLET_LOAD = (WINDOW_SIZE / WRAM_BUFFER)
uint32_t window[TASKLET_LOAD << WRAM_BUFFER_SHF]; // WINDOW BUFFER = NUMBER OF TASKLETS PARTICIPATING IN LOAD * 256 BYTES

uint8_t DT(uint32_t *p, uint32_t *q){
	uint8_t i = 0;
	uint8_t pb = 0;
	uint8_t qb = 0;

	for(i=0;i<D;i++){
		pb = ( p[i] < q[i] ) | pb;//At least one dimension better
		qb = ( q[i] < p[i] ) | qb;//No dimensions better
		//if(qb == 1) break;//Optional//
		if(qb == 1) return 0;
	}
	return ~qb & pb;//p dominates q 0xf0d6a 0xb4d08 0xf7243 0xe0e71 point
								  //0x  30f 0x6084c 0x86bb6 0x8f1b3 window
}

void kskyline_w256(){
	uint8_t id = me();
	uint32_t i = 0, j = 0, k = 0;
	uint32_t *buffer = dma_alloc(WRAM_BUFFER);
	uint32_t count = 0;

	//Load window from mram
	uint32_t window_offset = WINDOW_ADDR + (id << WRAM_BUFFER_SHF);
	if( id < TASKLET_LOAD ){//Cooperative load of the window in wram // Maximum window = 4096 bytes = 4096/4 = 1024 elements => 1024/D points
		mram_ll_read256(window_offset, buffer);// buffer = 256 // each tasklet responsible of loading 256 bytes = 64 elements => 64/D points
		for(i = 0 ; i < 64 ; i++) window[(id << 6) + i] = buffer[i];
	}
	barrier_wait(id);

	//Process skyline
	uint32_t low = (id * N) / TASKLETS;// low processing point
	uint32_t high = ((id+1) * N) / TASKLETS;// high processing point
	uint32_t points_offset = POINTS_ADDR + low * D * 4;//offset by 4 bytes for each dimension of a point
	uint32_t mask_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t *mask = dma_alloc(32);
	uint32_t c = 0;
	uint8_t pos = 0;
	uint8_t bit = 0;

	for(i = 0;i<8;i++) mask[i] = 0xFFFFFFFF;
	for( i = low; i < high; i+=POINTS_PER_BUFFER ){
		mram_ll_read256(points_offset,buffer);

		for( j = 0 ; j < VALUES_PER_BUFFER; j+=D){// inc by D to group them in points
			uint32_t *q = &buffer[j];
			for( k = 0; k < WINDOW_SIZE; k+=D ){// inc by D to group them in points
				uint32_t *p=&window[k];
				if (DT(p,q) == 0x1){
					mask[pos] = mask[pos] ^ (0x1 << bit);//Toggle to zero if point is dominated
					count++;
					break;
				}
			}
			c++;
			bit = c & 0x1F;
			pos = (c & 0xE0) >> 5;
			if( pos == 0 && bit == 0 ){
				mram_ll_write32(mask,mask_offset);
				mask_offset += 32;
				for(pos = 0;pos<8;pos++) mask[pos] = 0xFFFFFFFF;
				pos = 0;
			}
		}
		points_offset += WRAM_BUFFER;
	}
	mram_ll_write32(mask,mask_offset);

	barrier_wait(id);
	window[id] = (high-low) - count;
	barrier_wait(id);
	if(id < 1 ){
		count=0;
		for(i = 0 ;i < TASKLETS;i++) count+=window[i];
		mask[0] = count;
		mram_ll_write8(mask,0);
	}
}

void kskyline_w1024(){
	uint8_t id = me();
	uint32_t i = 0, j = 0, k = 0;
	uint32_t *buffer = dma_alloc(WRAM_BUFFER);
	uint32_t count = 0;

	//Load window from mram
	uint32_t window_offset = WINDOW_ADDR + (id << WRAM_BUFFER_SHF);
	if( id < TASKLET_LOAD ){//Cooperative load of the window in wram // Maximum window = 4096 bytes = 4096/4 = 1024 elements => 1024/D points
		mram_ll_read1024_new(window_offset, buffer);// buffer = 256 // each tasklet responsible of loading 256 bytes = 64 elements => 64/D points
		for(i = 0 ; i < 256 ; i++) window[(id << 8) + i] = buffer[i];// initialize 256 values //
	}
	barrier_wait(id);

	//Process skyline
	uint32_t low = (id * N) / TASKLETS;// low processing point
	uint32_t high = ((id+1) * N) / TASKLETS;// high processing point
	uint32_t points_offset = POINTS_ADDR + low * D * 4;//offset by 4 bytes for each dimension of a point
	uint32_t mask_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t *mask = dma_alloc(32);
	uint32_t c = 0;
	uint8_t pos = 0;
	uint8_t bit = 0;

	for(i = 0;i<8;i++) mask[i] = 0xFFFFFFFF;
	for( i = low; i < high; i+=POINTS_PER_BUFFER ){
		mram_ll_read1024_new(points_offset,buffer);

		for( j = 0 ; j < VALUES_PER_BUFFER; j+=D){// inc by D to group them in points
			uint32_t *q = &buffer[j];
			for( k = 0; k < WINDOW_SIZE; k+=D ){// inc by D to group them in points
				uint32_t *p=&window[k];
				if (DT(p,q) == 0x1){
					mask[pos] = mask[pos] ^ (0x1 << bit);//Toggle to zero if point is dominated
					count++;
					break;
				}
			}
			c++;
			bit = c & 0x1F;
			pos = (c & 0xE0) >> 5;
			if( pos == 0 && bit == 0 ){
				mram_ll_write32(mask,mask_offset);
				mask_offset += 32;
				for(pos = 0;pos<8;pos++) mask[pos] = 0xFFFFFFFF;//After write is it initialized to zero by default?
				pos = 0;
			}
		}
		points_offset += WRAM_BUFFER;
	}

	barrier_wait(id);
	window[id] = (high-low) - count;
	barrier_wait(id);

	if(id < 1 ){
		count=0;
		for(i = 0 ;i < TASKLETS;i++) count+=window[i];
		mask[0] = count;
		mram_ll_write8(mask,0);
	}
}

void kskyline_w1024_2(){
	uint8_t id = me();
	uint32_t i = 0, j = 0, k = 0;
	uint32_t *buffer = dma_alloc(WRAM_BUFFER);
	uint32_t count = 0;

	//Load window from mram
	uint32_t window_offset = WINDOW_ADDR + (id << WRAM_BUFFER_SHF);
	if( id < TASKLET_LOAD ){//Cooperative load of the window in wram // Maximum window = 4096 bytes = 4096/4 = 1024 elements => 1024/D points
		mram_ll_read1024_new(window_offset, buffer);// buffer = 256 // each tasklet responsible of loading 256 bytes = 64 elements => 64/D points
		for(i = 0 ; i < 256 ; i++) window[(id << 8) + i] = buffer[i];// initialize 256 values //
	}
	barrier_wait(id);

	//Process skyline
	uint32_t low = (id * N) / TASKLETS;// low processing point
	uint32_t high = ((id+1) * N) / TASKLETS;// high processing point
	uint32_t points_offset = POINTS_ADDR + low * D * 4;//offset by 4 bytes for each dimension of a point
	uint32_t mask_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t *mask = dma_alloc(32);
	uint32_t c = 0;
	uint8_t pos = 0;
	uint8_t bit = 0;

	for(i = 0;i<8;i++) mask[i] = 0xFFFFFFFF;//Set bit vector to 1 for each point
	for( i = low; i < high; i+=POINTS_PER_BUFFER ){
		mram_ll_read1024_new(points_offset,buffer);//Read a chunk of points

		for( j = 0 ; j < VALUES_PER_BUFFER; j+=D){// for each point in the wram buffer
			uint32_t *q = &buffer[j];
			for( k = 0; k < WINDOW_SIZE; k+=D ){// check against window of points
				uint32_t *p=&window[k];
				if (DT(p,q) == 0x1){//if point is dominated marke it with zero
					mask[pos] = mask[pos] ^ (0x1 << bit);//Toggle to zero if point is dominated
					count++;
					break;
				}
			}
			c++;//increase point count
			bit = c & 0x1F;//calculate relative position inside bit vector
			pos = (c & 0xE0) >> 5;//calculate the bit vector being used
			if( pos == 0 && bit == 0 ){//once you filled each bit vector write it to memory
				mram_ll_write32(mask,mask_offset);
				mask_offset += 32;
				for(pos = 0;pos<8;pos++) mask[pos] = 0xFFFFFFFF;//After write is it initialized to zero by default?
				pos = 0;
			}
		}
		points_offset += WRAM_BUFFER;
	}
	mram_ll_write32(mask,mask_offset);//write last chunk in memory if it was written by the final iteration

	barrier_wait(id);
	window[id] = (high-low) - count;//calculate count of points not dominated// necessary to
	barrier_wait(id);

	if(id < 1 ){
		count=0;
		for(i = 0 ;i < TASKLETS;i++) count+=window[i];//aggregate number of not dominated points
		mask[0] = count;
		mram_ll_write8(mask,0);
	}

}

int main(){
	if(WRAM_BUFFER == 256) kskyline_w256();
	if(WRAM_BUFFER == 1024) kskyline_w1024();
	return 0;
}
