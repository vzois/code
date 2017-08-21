#ifndef DSKY_V2_H
#define DSKY_V2_H

#include<defs.h>
#include<alloc.h>
//#include <mram_ll.h>
#include <mram.h>
#include <built_ins.h>

#include "../common/config.h"
#include "../common/tools.h"

#define V_2 true

#ifdef V_2

#define CHECK_ONLY_ACTIVE_POINTS
#define COMPUTE_GLOBAL_PSTOP
#define USE_BIT_VECTORS
#define USE_INTRINSIC_FUNCTION

uint32_t *window;//At most 16KB
uint32_t *Cj;//At most 16KB

uint32_t *pbvec;// At most 1KB // 256 points with at most 32 bit bit vectors each
uint32_t *qbvec;// At most 1KB // 256 points with at most 32 bit bit vectors each

uint32_t *pflag;// At most 8 , 32-bit wide flags = 32 bytes
uint32_t *qflag;// At most 8 , 32-bit wide flags = 32 bytes
uint32_t mflags[TASKLETS];// At most 16 tasklets, 64 bytes
uint8_t pflag_a[256];//
uint8_t qflag_a[256];

uint32_t debug;
uint32_t *qdebug;
uint32_t *pdebug;
uint8_t pc[256];//At most 256 bytes, used to precount the bit vectors

uint32_t g_ps;//global pstop
uint32_t ps[TASKLETS];//At most 1KB, 16 * 16 * 4 => 1024 bytes
uint32_t *qrank;
uint32_t *qclear;
uint32_t stop_p[TASKLETS];
uint32_t *qrank_p[TASKLETS];

uint32_t *param[TASKLETS];

void popc_8b(uint8_t v, uint8_t *c){
	//c[0] = v;
	c[0] = v - ((v >> 1) & 0x55);
	c[0] = ((c[0] >> 2) & 0x33) + (c[0] & 0x33);
	c[0] = ((c[0] >> 4) + c[0]) & 0x0F;
}

uint8_t DT_(uint32_t *p, uint32_t *q){
	uint8_t i = 0;
	uint8_t pb = 0;
	uint8_t qb = 0;

	for(i=0;i<D;i++){
		pb = ( p[i] < q[i] ) | pb;//At least one dimension better
		qb = ( q[i] < p[i] ) | qb;//No dimensions better
		if(qb == 1) return 0;
	}
	return ~qb & pb;//p dominates q 0xf0d6a 0xb4d08 0xf7243 0xe0e71 point
								  //0x  30f 0x6084c 0x86bb6 0x8f1b3 window
}

void init_v2(uint8_t id){
	uint32_t pflag_offset = DSKY_FLAGS_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t i = 0;

	if(id < 1){
		pflag = mem_alloc_dma(32);
		qflag = mem_alloc_dma(32);
		window = mem_alloc_dma(PSIZE_BYTES);//at most 16*256*4 = 16384 bytes
		Cj = mem_alloc_dma(PSIZE_BYTES);//at most 16*256*4 = 16384 bytes
		pbvec = mem_alloc_dma(PSIZE << 2);//1024 bytes
		qbvec = mem_alloc_dma(PSIZE << 2);//1024 bytes
		qrank = mem_alloc_dma(32);
		qclear = mem_alloc_dma(32);

		g_ps = 0xFFFFFFFF;
	}
	qrank_p[id] = mem_alloc_dma(32);
	ps[id] = 0xFFFFFFFF;
	param[id] = mem_alloc_dma(8);
	stop_p[id] = P;
	barrier_wait(id);

	if (id < 8) pflag[id] = 0xFFFFFFFF;
	if (id < 8) qclear[id] = 0x0;
	barrier_wait(id);

	uint32_t low = (id * P)/TASKLETS;
	uint32_t high = ((id+1) * P)/TASKLETS;
	for(i = low;i<high;i++){
		mram_write32(pflag,pflag_offset);
		pflag_offset+=32;
	}

	#ifndef USE_INTRINSIC_FUNCTION
	for(i=id;i<256;i+=TASKLETS){
		popc_8b(i,&pc[i]);//bit count map
	}
	#endif
}

//Compute points that are alive in order to consider them for prunning
void calc_active_p(uint8_t id){
	uint32_t i = 0;
	for(i=id;i<256;i+=16){
		uint8_t pos = (i & 0xE0) >> 5;
		uint8_t bit = (i & 0x1F);
		pflag_a[i]=(pflag[pos] >> bit) & 0x1;//mark active points
	}
	barrier_wait(id);
}

//After comparing two partitions find global p_stop and store update flags for alive points into MRAM
void acc_results(uint8_t id, uint32_t qflag_addr){

	#ifdef COMPUTE_GLOBAL_PSTOP
	if(id < 8) ps[id] = MIN(ps[id],ps[id+8]);//Reduce pstop values
	#endif
	barrier_wait(id);

	#ifdef COMPUTE_GLOBAL_PSTOP
	if(id < 4) ps[id] = MIN(ps[id],ps[id+4]);//Reduce pstop values
	#endif
	if(id < 8) mflags[id << 1] = mflags[id << 1] | (mflags[(id << 1)+1] << 16); //Merge flags
	barrier_wait(id);

	#ifdef COMPUTE_GLOBAL_PSTOP
	if(id < 2) ps[id] = MIN(ps[id],ps[id+2]);//Reduce pstop values
	#endif
	if(id < 8) qflag[id] = mflags[id << 1];//Merge flags
	barrier_wait(id);

	if(id == 0){
	#ifdef COMPUTE_GLOBAL_PSTOP
		ps[id] = MIN(ps[id],ps[id+1]);//Reduce pstop values
		g_ps = MIN(ps[id],g_ps);//Reduce pstop values
	#endif
		mram_write32(qflag,qflag_addr);//Write compute flags to MRAM
	}
}

void load_part_t(uint16_t cpart_i, uint32_t *buffer, uint8_t id){
	uint32_t offset = id << 10;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_read1024(i_addr + offset, &buffer[offset >> 2]);
}

void load_part_4d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_read1024(i_addr, &buffer[0]);
	mram_read1024(i_addr + 1024, &buffer[256]);
	mram_read1024(i_addr + 2048, &buffer[512]);
	mram_read1024(i_addr + 3072, &buffer[768]);
}

void load_part_8d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_read1024(i_addr, &buffer[0]);//0
	mram_read1024(i_addr + 1024, &buffer[256]);//1
	mram_read1024(i_addr + 2048, &buffer[512]);//2
	mram_read1024(i_addr + 3072, &buffer[768]);//3
	mram_read1024(i_addr + 4096, &buffer[1024]);//4
	mram_read1024(i_addr + 5120, &buffer[1280]);//5
	mram_read1024(i_addr + 6144, &buffer[1536]);//6
	mram_read1024(i_addr + 7168, &buffer[1792]);//7
}

void load_part_8d_t(uint16_t cpart_i, uint32_t *buffer, uint8_t id){
	uint32_t offset = id * 1024;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_read1024(i_addr + offset, &buffer[offset >> 2]);
}

void load_part_16d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_read1024(i_addr, &buffer[0]);//0
	mram_read1024(i_addr + 1024, &buffer[256]);//1
	mram_read1024(i_addr + 2048, &buffer[512]);//2
	mram_read1024(i_addr + 3072, &buffer[768]);//3
	mram_read1024(i_addr + 4096, &buffer[1024]);//4
	mram_read1024(i_addr + 5120, &buffer[1280]);//5
	mram_read1024(i_addr + 6144, &buffer[1536]);//6
	mram_read1024(i_addr + 7168, &buffer[1792]);//7

	mram_read1024(i_addr + 8192, &buffer[2048]);//8
	mram_read1024(i_addr + 9216, &buffer[2304]);//9
	mram_read1024(i_addr + 10240, &buffer[2560]);//10
	mram_read1024(i_addr + 11264, &buffer[2816]);//11
	mram_read1024(i_addr + 12288, &buffer[3072]);//12
	mram_read1024(i_addr + 13312, &buffer[3328]);//13
	mram_read1024(i_addr + 14336, &buffer[3584]);//14
	mram_read1024(i_addr + 15360, &buffer[3840]);//15
}

uint8_t cmp_part_4d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
#ifdef COMPUTE_GLOBAL_PSTOP
	if(id == 0){//Load level of first points in the partition
		uint32_t qrank_addr = DSKY_RANK_ADDR + (cpart_j * PSIZE * 4);
		mram_read32(qrank_addr,qrank);
	}
	barrier_wait(id);

	if( g_ps <= qrank[0] ){// If level is less than the global stop point you can stop and prune all points in the partition
		stop_p[id]=MIN(cpart_j,stop_p[id]);
		if (id == 0){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
			mram_write32(qclear,qflag_addr);
		}
		barrier_wait(id);
		return 1;
	}
#endif

	uint32_t qflag_addr;
	if(id < 4){//Each tasklet reads 1024 bytes// Fixed PSIZE requires 4 tasklets to read 4096 bytes (256 * 4 * 4)
		load_part_t(cpart_i,window,id);
		load_part_t(cpart_j,Cj,id);
	}
	if(id == 0){
		//load_part_4d(cpart_i,window);
		//load_part_4d(cpart_j,Cj);

		uint32_t pflag_addr = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_read32(pflag_addr,pflag); // read flags of window set//used by all threads//Indicate which points are alieve
		mram_read32(qflag_addr,qflag); // read flags of C_j set//need to extract portion of tasklet//Indicate which points are alive

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + (cpart_i * PSIZE * 4);//Bit vector for partition i // Used for cheap DTs
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + (cpart_j * PSIZE * 4);//Bit vector for partition j // Used for cheap DTs
		mram_read1024(pbvec_addr,pbvec);
		mram_read1024(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0, k = 0;
	uint8_t qbit = 0;

#ifdef COMPUTE_GLOBAL_PSTOP
	uint32_t mx;
	ps[id] = 0xFFFFFFFF;
#endif

#ifdef CHECK_ONLY_ACTIVE_POINTS
	//uint8_t qi = id * TASKLETS;
	calc_active_p(id);
#endif

	//Prune points from Cj partition using Ci <Ci,Cj>
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){//For each point in Cj partition
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration//
#endif
		uint32_t *q = &Cj[work_offset + j];//Pointer to point being tested
		uint8_t dt = 0;//Set if point is dominated
#ifdef USE_BIT_VECTORS
		uint16_t vi_offset = (work_offset + j) >> 2;//load bit vector for q point// divide by 4 since j increases by 4
		uint8_t Mi = (qbvec[vi_offset] & 0xF);//Media level bit vector
		uint8_t Qi = (qbvec[vi_offset] & 0xF0) >> 4;//Quartile level bit vector
#endif
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){//For each point in window partition//The partition we check against
#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>2] == 0){ continue; } //Check if point is active// divide index by 4
#endif
			uint32_t *p = &window[i];
#ifdef USE_BIT_VECTORS//Use Bit vectors for cheap DTs// Points are incomparable if any of the following holds//
			uint16_t vj_offset = i >> 2;//index i inc by D// divide by 4
			uint8_t Mj = (pbvec[vj_offset] & 0xF);
			uint8_t Qj = (pbvec[vj_offset] & 0xF0) >> 4;

			#ifndef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			else if(pc[Mi] < pc[Mj]) continue;
			else if((pc[Mi] == pc[Mj]) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
			#ifdef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			uint32_t ci,cj;
			__builtin_cao_rr(ci,Mi);
			__builtin_cao_rr(cj,Mj);
			if(ci < cj) continue;
			else if((ci == cj) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
#endif
			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt = 1;
				break;
			}
		}
#ifdef COMPUTE_GLOBAL_PSTOP//minimax rule to update global pstop
		if(dt == 0){//Each tasklet local pstop//merge at acc_results
			mx = MAX(MAX(q[0],q[1]),MAX(q[2],q[3]));
			ps[id] = MIN(mx,ps[id]);
		}
#endif
		qbit++;
	}
	barrier_wait(id);
	mflags[id] = tflag;
	acc_results(id,qflag_addr);//accumulate pstop // merge flags to update alive points
	barrier_wait(id);
	return 0;
}

uint8_t cmp_part_8d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
#ifdef COMPUTE_GLOBAL_PSTOP
	if(id == 0){//Load level of first points in the partition
		uint32_t qrank_addr = DSKY_RANK_ADDR + (cpart_j * PSIZE * 4);
		mram_read32(qrank_addr,qrank);
	}
	barrier_wait(id);

	if( g_ps <= qrank[0] ){// If level is less than the global stop point you can stop and prune all points in the partition
		stop_p[id]=MIN(cpart_j,stop_p[id]);
		if (id == 0){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
			mram_write32(qclear,qflag_addr);
		}
		barrier_wait(id);
		return 1;
	}
#endif
	uint32_t qflag_addr;
	if(id < 8){
		load_part_t(cpart_i,window,id);
		load_part_t(cpart_j,Cj,id);
	}
	if(id == 0){
		//load_part_8d(cpart_i,window);
		//load_part_8d(cpart_j,Cj);

		uint32_t pflag_addr = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_read32(pflag_addr,pflag); // read flags of window set//used by all threads
		mram_read32(qflag_addr,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + ((cpart_i << 8) << 2);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + ((cpart_j << 8) << 2);
		mram_read1024(pbvec_addr,pbvec);
		mram_read1024(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint32_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0;
	uint8_t qbit = 0;

#ifdef COMPUTE_GLOBAL_PSTOP
	uint32_t mx_0,mx_1;
	ps[id] = 0xFFFFFFFF;
#endif

#ifdef CHECK_ONLY_ACTIVE_POINTS
	calc_active_p(id);
#endif

	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration
#endif
		uint32_t *q = &Cj[work_offset + j];
		uint8_t dt = 0;
#ifdef USE_BIT_VECTORS
		uint16_t vi_offset = (work_offset + j) >> 3;//divide index by 8
		uint8_t Mi = (qbvec[vi_offset] & 0xFF);
		uint8_t Qi = (qbvec[vi_offset] & 0xFF00) >> 8;
#endif

		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>3] == 0){ continue; } //Check if point is active// divide index by 8
#endif
			uint32_t *p = &window[i];
#ifdef USE_BIT_VECTORS
			uint16_t vj_offset = i >> 3;//divide index by 8
			uint16_t Mj = (pbvec[vj_offset] & 0xFF);
			uint16_t Qj = (pbvec[vj_offset] & 0xFF00) >> 8;

			#ifndef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			else if(pc[Mi] < pc[Mj]) continue;
			else if((pc[Mi] == pc[Mj]) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
			#ifdef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			uint32_t ci,cj;
			__builtin_cao_rr(ci,Mi);
			__builtin_cao_rr(cj,Mj);
			if(ci < cj) continue;
			else if((ci == cj) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
#endif
			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt=1;
				break;
			}
		}
#ifdef COMPUTE_GLOBAL_PSTOP
		if(dt == 0){
			mx_0 = MAX(q[0],q[1]);
			mx_1 = MAX(q[2],q[3]);
			mx_0 = MAX(mx_0,mx_1);
			mx_1 = MAX(q[4],q[5]);
			mx_0 = MAX(mx_0,mx_1);
			mx_1 = MAX(q[6],q[7]);
			ps[id] = MIN(ps[id],MAX(mx_0,mx_1));
		}
#endif
		qbit++;
	}
	barrier_wait(id);
	mflags[id] = tflag;
	acc_results(id,qflag_addr);
	barrier_wait(id);
	return 0;
}

uint8_t cmp_part_16d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
#ifdef COMPUTE_GLOBAL_PSTOP
	if(id == 0){//Load level of first points in the partition
		uint32_t qrank_addr = DSKY_RANK_ADDR + (cpart_j * PSIZE * 4);
		mram_read32(qrank_addr,qrank);
	}
	barrier_wait(id);

	if( g_ps <= qrank[0] ){// If level is less than the global stop point you can stop and prune all points in the partition
		stop_p[id]=MIN(cpart_j,stop_p[id]);
		if (id == 0){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
			mram_write32(qclear,qflag_addr);
		}
		barrier_wait(id);
		return 1;
	}
#endif
	uint32_t qflag_addr;

	load_part_t(cpart_i,window,id);
	load_part_t(cpart_j,Cj,id);

	if(id == 0){
		//load_part_16d(cpart_i,window);
		//load_part_16d(cpart_j,Cj);

		uint32_t pflag_addr = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_read32(pflag_addr,pflag); // read flags of C_i set//used by all threads
		mram_read32(qflag_addr,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + ((cpart_i << 8) << 2);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + ((cpart_j << 8) << 2);
		mram_read1024(pbvec_addr,pbvec);
		mram_read1024(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0;
	uint8_t qbit = 0;

#ifdef COMPUTE_GLOBAL_PSTOP
	ps[id] = 0xFFFFFFFF;
#endif

#ifdef CHECK_ONLY_ACTIVE_POINTS
	calc_active_p(id);
#endif

	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration
#endif
		uint32_t *q = &Cj[work_offset + j];
		uint8_t dt = 0;
#ifdef USE_BIT_VECTORS
		uint16_t vi_offset = (work_offset + j) >> 4;//divide index by 16
		uint16_t Mi = (qbvec[vi_offset] & 0xFFFF);//16 bit vector
		uint16_t Qi = (qbvec[vi_offset] & 0xFFFF0000) >> 16;
#endif
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>4] == 0){ continue; } //Check if point is active// divide index by 16
#endif
			uint32_t *p = &window[i];
#ifdef USE_BIT_VECTORS
			uint16_t vj_offset = i >> 4;//divide index by 16
			uint16_t Mj = (pbvec[vj_offset] & 0xFFFF);//16 bit vector
			uint16_t Qj = (pbvec[vj_offset] & 0xFFFF0000) >> 16;

			#ifndef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			else if(pc[Mi] < pc[Mj]) continue;
			else if((pc[Mi] == pc[Mj]) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
			#ifdef USE_INTRINSIC_FUNCTION
			if ((Mj | Mi) > Mi) continue;
			uint32_t ci,cj;
			__builtin_cao_rr(ci,Mi);
			__builtin_cao_rr(cj,Mj);
			if(ci < cj) continue;
			else if((ci == cj) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;
			#endif
#endif

			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt=1;
				break;
			}
		}
#ifdef COMPUTE_GLOBAL_PSTOP
		if(dt == 0){
			uint32_t mx_0,mx_1,mx_2,mx_3,mx_4,mx_5,mx_6,mx_7;
			mx_0 = MAX(q[0],q[1]);
			mx_1 = MAX(q[2],q[3]);
			mx_2 = MAX(q[4],q[5]);
			mx_3 = MAX(q[6],q[7]);
			mx_4 = MAX(q[8],q[9]);
			mx_5 = MAX(q[10],q[11]);
			mx_6 = MAX(q[12],q[13]);
			mx_7 = MAX(q[14],q[15]);

			mx_0 = MAX(mx_0,mx_1);
			mx_2 = MAX(mx_2,mx_3);
			mx_4 = MAX(mx_4,mx_5);
			mx_6 = MAX(mx_6,mx_7);

			mx_0 = MAX(mx_0,mx_2);
			mx_4 = MAX(mx_4,mx_6);

			ps[id] = MIN(ps[id],MAX(mx_0,mx_4));
		}
#endif

		qbit++;
	}
	barrier_wait(id);
	mflags[id] = tflag;
	acc_results(id,qflag_addr);
	barrier_wait(id);
	return 0;
}

#endif

#endif
