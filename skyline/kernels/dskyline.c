#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"


uint32_t *buffers[TASKLETS];//1024 * 16 = 16384 bytes
uint32_t *pflags[TASKLETS];// 32 * 16 = 512 bytes
uint32_t C_i[(PSIZE * D)]; // 256 * 16 * 4 = 16384 bytes
uint32_t C_j[(PSIZE * D)];

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

void init(uint8_t id){
	uint32_t pflag_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	pflags[id] = dma_alloc(32);//256 point bit vector
	uint32_t i = 0;
	uint32_t iter = (N / TASKLETS) / 256;
	for(i=0;i<8;i++) pflags[id][i] = 0xFFFFFFFF;

	for(i = 0;i<iter;i++){
		mram_ll_write32(pflags[id],pflag_offset);
		pflag_offset+=32;
	}
	buffers[id] = dma_alloc(POINTS_PER_T_BYTES);//Preallocated buffer for reading
}

void load_part(uint8_t id, uint32_t *st, uint32_t i_addr){
	uint32_t i = 0;
	uint32_t *buffer = buffers[id];
	if( POINTS_PER_T_BYTES==256 ){
		mram_ll_read256(i_addr, buffer);
		for(i = 0 ; i < 64 ; i++) st[i] = buffer[i];
	}
	else if( POINTS_PER_T_BYTES==512 ){
		mram_ll_read256(i_addr, buffer);
		for(i = 0 ; i < 64 ; i++) st[i] = buffer[i];
		mram_ll_read256(i_addr+256, buffer);
		for(i = 0 ; i < 64 ; i++) st[i] = buffer[i];
	}
	else if( POINTS_PER_T_BYTES==1024 ){
		mram_ll_read1024_new(i_addr, buffer);
		for(i = 0 ; i < 256 ; i++) st[i] = buffer[i];
	}
}

void self_cmp_part(uint8_t id, uint16_t cpart_i){

}

void cmp_part(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	//uint32_t i = 0;

	uint32_t tasklet_offset = (id * PSIZE)/TASKLETS;
	uint32_t ppos_i = cpart_i * PSIZE;//Position of partition i
	uint32_t ppos_j = cpart_j * PSIZE;//Position of partition j

	uint32_t i_addr = POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );
	uint32_t j_addr = POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );

	uint32_t *buffer = buffers[id];
	uint32_t t_offset = POINTS_PER_T_VALUES * id;

	/*if( POINTS_PER_T_BYTES==256 ){
		mram_ll_read256(i_addr, buffer);
		for(i = 0 ; i < 64 ; i++) C_i[t_offset + i] = buffer[i];
	}
	else if( POINTS_PER_T_BYTES==512 ){
		mram_ll_read256(i_addr, buffer);
		for(i = 0 ; i < 64 ; i++) C_i[t_offset + i] = buffer[i];
		mram_ll_read256(i_addr+256, buffer);
		for(i = 0 ; i < 64 ; i++) C_i[t_offset + 64 + i] = buffer[i];
	}
	else if( POINTS_PER_T_BYTES==1024 ){
		mram_ll_read1024_new(i_addr, buffer);
		for(i = 0 ; i < 256 ; i++) C_i[t_offset+ i] = buffer[i];
	}*/

	load_part(id,&C_i[t_offset],i_addr);
	load_part(id,&C_j[t_offset],j_addr);

	uint32_t pflag_offset = MASK_ADDR + id * (N >> 7);
	uint32_t *pflag = pflags[id];
	mram_ll_read32(pflag_offset,pflag);

	uint32_t c = 0;
	uint8_t pos = 0;
	uint8_t bit = 0;

	uint32_t i = 0, j =0;
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){//For each point that i need to check
		uint32_t *q = &C_j[t_offset + j];
		uint8_t dt = 1;
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
			uint32_t *p = &C_i[i];

			//dt = (pflag[pos] >> bit) & 0x1;
			dt = (pflag[pos] & (0x1 << bit));
			if (dt != 0){
				if(DT(p,q) == 1){
					pflag[pos] = pflag[pos] ^ (0x1 << bit);
					break;
				}
			}

		}
		c++;
		bit = c & 0x1F;
		pos = (c & 0xE0) >> 5;
	}
	mram_ll_write32(pflag,pflag_offset);

}

int main(){
	uint8_t id = me();

	init(id);
	//barrier_wait(id);
	cmp_part(id,0,0);

	uint16_t i = 0 , j = 0;
	/*cmp_part(id,0,0);

	for(i = 1; i < P;i++){
		cmp_part(id,0,i);
	}*/

	for(i = 0 ; i < P; i++){
		cmp_part(id,i,i);
		for(j = i+1 ; j < P; j++){
			cmp_part(id,i,j);
		}
	}

	//cmp_part(id,0,0);
	//cmp_part(id,0,1);


	return 0;
}
