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
	uint32_t pflag_offset = FLAGS_ADDR + id * (N >> 7);//offset by 1 byte for each point
	pflags[id] = dma_alloc(32);//256 point bit vector
	uint32_t i = 0;
	uint32_t iter = (N / TASKLETS) / PSIZE;
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

void batch_cmp_part(uint8_t id, uint16_t cpart_i){
	uint32_t tasklet_offset = (id * PSIZE)/TASKLETS;
	uint32_t ppos_i = cpart_i * PSIZE;//Position of partition i
	uint32_t ppos_j = (cpart_i+1) * PSIZE;//Position of partition j

	uint32_t i_addr = POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );
	uint32_t j_addr = POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );

	uint32_t *buffer = buffers[id];
	uint32_t t_offset = POINTS_PER_T_VALUES * id;

	load_part(id,&C_i[t_offset],i_addr);

	uint32_t i = 0, j = 0, k = 0;
	for(k=cpart_i+1;k<P;k++){
		load_part(id,&C_j[t_offset],j_addr);
	}

}
uint32_t flag_1 = 0;

void cmp_part(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	uint32_t tasklet_offset = (id * PSIZE)/TASKLETS;
	uint32_t ppos_i = cpart_i * PSIZE;//Position of partition i
	uint32_t ppos_j = cpart_j * PSIZE;//Position of partition j

	uint32_t i_addr = POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );
	uint32_t j_addr = POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );

	uint32_t *buffer = buffers[id];
	uint32_t t_offset = POINTS_PER_T_VALUES * id;
	uint32_t i = 0, j =0;

	load_part(id,&C_i[t_offset],i_addr);
	load_part(id,&C_j[t_offset],j_addr);

	uint32_t *pflag = pflags[0];
	uint32_t *qflag = pflags[1];
	uint32_t tflag = 0;
	uint32_t pflag_offset = FLAGS_ADDR + (cpart_i << 5);
	uint32_t qflag_offset = FLAGS_ADDR + (cpart_j << 5);
	if ( id == 0 ) mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
	if ( id == 1 ) mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet
	barrier_wait(id);
	tflag = (id & 0x1) ? (qflag[id>>1])>>POINTS_PER_T : qflag[id>>1];//extract flags of point corresponding to given tasklet

	uint8_t qbit = 0;//16 points per tasklet
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){//For each point that i need to check
		uint32_t *q = &C_j[t_offset + j];
		uint8_t alive_p = 1;
		uint8_t alive_q = 1;

		uint8_t pbit = 0;//32 bits
		uint32_t ppos = 0;// 256 / 32 = 8 positions
		uint32_t c = 0;//
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
			uint32_t *p = &C_i[i];

			alive_p = (pflag[ppos] >> pbit) & 0x1;
			alive_q = (tflag >> qbit) & 0x1;
			if (alive_p == 1 & alive_q == 1){
				if(DT(p,q) == 1){
					tflag = tflag ^ (0x1 << qbit);
					if(id == 1) flag_1++;
					break;
				}
			}
			pbit = c & 0x1F;
			ppos = (c & 0xE0) >> 5;
		}
		qbit++;
	}
	barrier_wait(id);
	pflags[id][0] = tflag;
	if(id < 1){
		for(i = 0;i<16;i+=2){
			pflags[0][i>>1]=(pflags[i][0] & POINTS_PER_T_MSK) | (pflags[i+1][0] << POINTS_PER_T);
		}
		mram_ll_write32(pflags[0],qflag_offset);
	}
}

int main(){
	uint8_t id = me();
	init(id);
	uint16_t i = 0 , j = 0;
	//cmp_part(id,0,0);

	cmp_part(id,0,0);
	for(j = i+1 ; j < P; j++){
		cmp_part(id,i,j);
	}


	return 0;
}
