#define V_1 true

#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

#ifdef V_1
uint32_t *buffers[TASKLETS];//1024 * 16 = 16384 bytes
//uint32_t *pflags[TASKLETS];// 32 * 16 = 512 bytes

uint32_t *pflag;
uint32_t *qflag;

uint32_t mflags[TASKLETS];//merge flag buffer;
uint32_t C_i[(PSIZE * D)]; // 256 * 16 * 4 = 16384 bytes
uint32_t C_j[(PSIZE * D)];
//uint32_t *C_i;
//uint32_t *C_j;

uint32_t t_ps[TASKLETS];//buffer to computer Pstop
uint32_t g_ps;
uint32_t *p_dt;
uint32_t *level_v;
uint32_t debug;

uint8_t DT(uint32_t *p, uint32_t *q){
	uint8_t i = 0;
	uint8_t pb = 0;
	uint8_t qb = 0;

	/*pb = ( p[0] < q[0] ) | pb; qb = ( q[0] < p[0] ) | qb; if(qb == 1) return 0;
	pb = ( p[1] < q[1] ) | pb; qb = ( q[1] < p[1] ) | qb; if(qb == 1) return 0;
	pb = ( p[2] < q[2] ) | pb; qb = ( q[2] < p[2] ) | qb; if(qb == 1) return 0;
	pb = ( p[3] < q[3] ) | pb; qb = ( q[3] < p[3] ) | qb; if(qb == 1) return 0;*/
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
	//pflags[id] = dma_alloc(32);//256 point bit vector
	uint32_t i = 0;
	uint32_t iter = (N / TASKLETS) / PSIZE;

	if(id < 1){
		pflag = dma_alloc(32);
		qflag = dma_alloc(32);

		debug = 0;
		g_ps = 0xFFFFFFFF;
		level_v = dma_alloc(32);
		p_dt = dma_alloc(32);
		for(i = 0;i<8;i++) p_dt[i] = 0x0;
	}
	barrier_wait(id);

	//for(i=0;i<8;i++) pflags[id][i] = 0xFFFFFFFF;
	if (id < 8) pflag[id] = 0xFFFFFFFF;
	barrier_wait(id);

	for(i = 0;i<iter;i++){
		//mram_ll_write32(pflags[id],pflag_offset);
		mram_ll_write32(pflag,pflag_offset);
		pflag_offset+=32;
	}
	buffers[id] = dma_alloc(POINTS_PER_T_BYTES);//Preallocated buffer for reading
}

void part_dt(uint32_t cpart_j){
	uint32_t rank_offset = DSKY_RANK_ADDR + ((cpart_j << PSIZE_SHF) << 2);
	mram_ll_read32(rank_offset,level_v);
}

void kill_part(uint8_t id, uint32_t cpart_j){
	uint32_t qflag_offset = FLAGS_ADDR + (cpart_j << 5);
	//barrier_wait(id);
	//p_dt[id] = 0;

	if(id == 0){
		uint32_t i = 0;
		for(i = 0;i<8;i++) p_dt[i] = 0x0;
		mram_ll_write32(p_dt,qflag_offset);
	}
}

void minimax(uint8_t id, uint32_t *q){
	switch(D){
		case 4:
			t_ps[id] = MIN(max_vec4(q),t_ps[id]);
			break;
		case 8:
			t_ps[id] = MIN(max_vec8(q),t_ps[id]);
			break;
		case 16:
			t_ps[id] = MIN(max_vec16(q),t_ps[id]);
			break;
	}
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
		for(i = 0 ; i < 64 ; i++) st[i+64] = buffer[i];
	}
	else if( POINTS_PER_T_BYTES==1024 ){
		mram_ll_read1024_new(i_addr, buffer);
		for(i = 0 ; i < 256 ; i++) st[i] = buffer[i];
	}
}

void load_point(uint32_t *st, uint32_t addr){
	switch(D){
		case 4:
			mram_ll_read32(addr,st);
			break;
		case 8:
			mram_ll_read32(addr,st);
			break;
		case 16:
			mram_ll_read64(addr,st);
			break;
		default:
			break;
	}
}

void cmp_part(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	//Check if partition can be pruned
	/*t_ps[id] = 0xFFFFFFFF;
	if(id == 0) part_dt(cpart_j);
	barrier_wait(id);
	if(g_ps <= level_v[0]){//If global pstop is less than level of fist point prune the whole partition
		if(id < 4) kill_part(id,cpart_j);// this method is not optimal <?>
		return;
	}*/

	uint32_t tasklet_offset = (id << PSIZE_SHF) >> TASKLETS_SHF;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t ppos_j = cpart_j << PSIZE_SHF;//Position of partition j
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );//Partition i starting addr
	uint32_t j_addr = DSKY_POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );//Partition j starting addr

	uint32_t t_offset = POINTS_PER_T_VALUES * id;//offset for given tasklet to write in window// <<4 => 16 tasklets
	uint32_t i = 0, j =0;
	uint8_t wid = id >> 1;//group tasklets in pairs to extract flags for partition j

	load_part(id,&C_i[t_offset],i_addr);
	load_part(id,&C_j[t_offset],j_addr);

	uint32_t tflag = 0;//local tflag for points that this tasklet is processing
	uint32_t pflag_offset = FLAGS_ADDR + (cpart_i << 5);
	uint32_t qflag_offset = FLAGS_ADDR + (cpart_j << 5);
	if ( id == 0 ) mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
	if ( id == 1 ) mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet
	barrier_wait(id);
	tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);//extract flags of point corresponding to given tasklet

	uint8_t qbit = 0;//16 points per tasklet
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){//For each point in j partition
		uint32_t *q = &C_j[t_offset + j];
		uint8_t alive_q = (tflag >> qbit) & 0x1;

		if(alive_q == 1){//If point has not been pruned yet
			uint8_t pbit = 0;//32 bits
			uint32_t ppos = 0;// 256 / 32 = 8 positions
			uint32_t c = 0;//
			uint8_t alive_p = 1;
			uint8_t dt = 0;
			for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){//Check against each point in i partition
				uint32_t *p = &C_i[i];

				alive_p = (pflag[ppos] >> pbit) & 0x1;
				if (alive_p == 1){//If point is still alive check against q
					if(DT(p,q) == 1){
						//tflag = tflag ^ (0x1 << qbit);
						tflag &= ~(0x1 << qbit);//Set flag for dominace
						dt = 1;
						break;
					}
				}
				c++;
				pbit = c & 0x1F;
				ppos = (c & 0xE0) >> 5;
			}
			if (dt == 0) minimax(id,q);//compute new Pstop if q is not dominated => argmin_i max(p_i[j])) from all points in Sky, min (max(dimension))
		}
		qbit++;
	}
	barrier_wait(id);

	reduce_min(id,t_ps);//Gather all Pstop candidates
	if(id == 0) g_ps = MIN(t_ps[0],g_ps);//Update Pstop
	barrier_wait(id);

	//Write bitvectors into mram// Not optimal // Having some problems//
	mflags[id] = tflag;
	barrier_wait(id);
	if(id < 1){
		for(i = 0 ; i < 16;i+=2){
			pflag[(i >> 1)] = (mflags[i] & POINTS_PER_T_MSK) | (mflags[i+1] << POINTS_PER_T);
			mram_ll_write32(pflag,qflag_offset);
		}
	}
	barrier_wait(id);
}

#endif
