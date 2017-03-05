#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

#define V_2 true

#ifdef V_2

#define CHECK_ONLY_ACTIVE_POINTS

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
uint8_t pc[256];//At most 256 bytes, used to precount the bit vectors

uint32_t g_ps;//global pstop
uint32_t ps[TASKLETS][D];//At most 1KB, 16 * 16 * 4 => 1024 bytes

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
	uint32_t iter = (N / TASKLETS) / PSIZE;
	uint32_t i = 0;

	if(id < 1){
		pflag = dma_alloc(32);
		qflag = dma_alloc(32);
		window = dma_alloc(PSIZE_BYTES);
		Cj = dma_alloc(PSIZE_BYTES);
		pbvec = dma_alloc(PSIZE << 2);
		qbvec = dma_alloc(PSIZE << 2);

		g_ps = 0xFFFFFFFF;
	}
	barrier_wait(id);

	if (id < 8) pflag[id] = 0xFFFFFFFF;
	barrier_wait(id);

	for(i = 0;i<iter;i++){
		mram_ll_write32(pflag,pflag_offset);
		pflag_offset+=32;
	}

	for(i=id;i<256;i+=TASKLETS){
		popc_8b(i,&pc[i]);//bit count map
	}
}

void part_dt(uint16_t cpart_i, uint16_t cpart_j){

}

void calc_active_p(uint8_t id){
	uint32_t i = 0;
	for(i=id;i<256;i+=16){
		uint8_t pos = (i & 0xE0) >> 5;
		uint8_t bit = (i & 0x1F);
		pflag_a[i]=(pflag[pos] >> bit) & 0x1;//mark active points
		//qflag_a[i]=(qflag[pos] >> bit) & 0x1;
	}
	barrier_wait(id);
}

void load_part_t(uint16_t cpart_i, uint32_t *buffer, uint8_t id){
	uint32_t offset = id << 10;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr + offset, &buffer[offset >> 2]);
}

void load_part_4d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr, &buffer[0]);
	mram_ll_read1024_new(i_addr + 1024, &buffer[256]);
	mram_ll_read1024_new(i_addr + 2048, &buffer[512]);
	mram_ll_read1024_new(i_addr + 3072, &buffer[768]);
}

void load_part_8d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr, &buffer[0]);//0
	mram_ll_read1024_new(i_addr + 1024, &buffer[256]);//1
	mram_ll_read1024_new(i_addr + 2048, &buffer[512]);//2
	mram_ll_read1024_new(i_addr + 3072, &buffer[768]);//3
	mram_ll_read1024_new(i_addr + 4096, &buffer[1024]);//4
	mram_ll_read1024_new(i_addr + 5120, &buffer[1280]);//5
	mram_ll_read1024_new(i_addr + 6144, &buffer[1536]);//6
	mram_ll_read1024_new(i_addr + 7168, &buffer[1792]);//7
}

void load_part_8d_t(uint16_t cpart_i, uint32_t *buffer, uint8_t id){
	uint32_t offset = id * 1024;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr + offset, &buffer[offset >> 2]);
}

void load_part_16d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr, &buffer[0]);//0
	mram_ll_read1024_new(i_addr + 1024, &buffer[256]);//1
	mram_ll_read1024_new(i_addr + 2048, &buffer[512]);//2
	mram_ll_read1024_new(i_addr + 3072, &buffer[768]);//3
	mram_ll_read1024_new(i_addr + 4096, &buffer[1024]);//4
	mram_ll_read1024_new(i_addr + 5120, &buffer[1280]);//5
	mram_ll_read1024_new(i_addr + 6144, &buffer[1536]);//6
	mram_ll_read1024_new(i_addr + 7168, &buffer[1792]);//7

	mram_ll_read1024_new(i_addr + 8192, &buffer[2048]);//8
	mram_ll_read1024_new(i_addr + 9216, &buffer[2304]);//9
	mram_ll_read1024_new(i_addr + 10240, &buffer[2560]);//10
	mram_ll_read1024_new(i_addr + 11264, &buffer[2816]);//11
	mram_ll_read1024_new(i_addr + 12288, &buffer[3072]);//12
	mram_ll_read1024_new(i_addr + 13312, &buffer[3328]);//13
	mram_ll_read1024_new(i_addr + 14336, &buffer[3584]);//14
	mram_ll_read1024_new(i_addr + 15360, &buffer[3840]);//15
}

void cmp_part_4d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	uint32_t qflag_addr;

	if(id < 4){
		load_part_t(cpart_i,window,id);
		load_part_t(cpart_j,Cj,id);
	}
	if(id == 0){
		//load_part_4d(cpart_i,window);
		//load_part_4d(cpart_j,Cj);

		uint32_t pflag_addr = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_addr,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_addr,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + (cpart_i * 256 * 4);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + (cpart_j * 256 * 4);
		mram_ll_read1024_new(pbvec_addr,pbvec);
		mram_ll_read1024_new(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0, k = 0;
	uint8_t qbit = 0;


#ifdef CHECK_ONLY_ACTIVE_POINTS
	uint8_t qi = id * TASKLETS;
	calc_active_p(id);
#endif

	//Prune points from Cj partition using Ci <Ci,Cj>
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){//For each point in Cj partition
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration//
#endif
		uint32_t *q = &Cj[work_offset + j];
		uint16_t vi_offset = (work_offset + j) >> 2;
		uint8_t Mi = (qbvec[vi_offset] & 0xF);
		uint8_t Qi = (qbvec[vi_offset] & 0xF0) >> 4;
		uint8_t dt = 0;

		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){//For each point in Ci partition
#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>2] == 0){ continue; } //Check if point is active// divide index by 4
#endif

			uint32_t *p = &window[i];
			uint16_t vj_offset = i >> 2;
			uint8_t Mj = (pbvec[vj_offset] & 0xF);
			uint8_t Qj = (pbvec[vj_offset] & 0xF0) >> 4;

			if ((Mj | Mi) > Mi) continue;
			else if(pc[Mi] < pc[Mj]) continue;
			else if((pc[Mi] == pc[Mj]) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;

			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt = 1;
				break;
			}
		}
		qbit++;
	}
	barrier_wait(id);

	mflags[id] = tflag;
	barrier_wait(id);
	if(id < 8) mflags[id << 1] = mflags[id << 1] | (mflags[(id << 1)+1] << 16);
	barrier_wait(id);
	if(id < 8) qflag[id] = mflags[id << 1];
	barrier_wait(id);
	if(id==0) mram_ll_write32(qflag,qflag_addr);
}

void cmp_part_8d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	uint32_t qflag_offset;

	if(id < 8){
		load_part_t(cpart_i,window,id);
		load_part_t(cpart_j,Cj,id);
	}
	if(id == 0){
		//load_part_8d(cpart_i,window);
		//load_part_8d(cpart_j,Cj);

		uint32_t pflag_offset = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_offset = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + (cpart_i * 256 * 4);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + (cpart_j * 256 * 4);
		mram_ll_read1024_new(pbvec_addr,pbvec);
		mram_ll_read1024_new(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0;
	uint8_t qbit = 0;

#ifdef CHECK_ONLY_ACTIVE_POINTS
	calc_active_p(id);
#endif

	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration
#endif
		uint32_t *q = &Cj[work_offset + j];
		uint16_t vi_offset = (work_offset + j) >> 3;//divide index by 8
		uint8_t Mi = (qbvec[vi_offset] & 0xFF);
		uint8_t Qi = (qbvec[vi_offset] & 0xFF00) >> 8;
		uint8_t dt = 0;

		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>3] == 0){ continue; } //Check if point is active// divide index by 8
#endif
			uint32_t *p = &window[i];
			uint16_t vj_offset = i >> 3;//divide index by 8
			uint8_t Mj = (pbvec[vj_offset] & 0xFF);
			uint8_t Qj = (pbvec[vj_offset] & 0xFF00) >> 8;

			if ((Mj | Mi) > Mi) continue;
			else if(pc[Mi] < pc[Mj]) continue;
			else if((pc[Mi] == pc[Mj]) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;

			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt=1;
				break;
			}
		}

		qbit++;
	}
	mflags[id] = tflag;
	barrier_wait(id);
	if(id < 8) mflags[id << 1] = mflags[id << 1] | (mflags[(id << 1)+1] << 16);
	barrier_wait(id);
	if(id < 8) qflag[id] = mflags[id << 1];
	barrier_wait(id);
	if(id==0) mram_ll_write32(qflag,qflag_offset);
	//barrier_wait(id);
}

void cmp_part_16d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	uint32_t qflag_offset;

	load_part_t(cpart_i,window,id);
	load_part_t(cpart_j,Cj,id);

	if(id == 0){
		//load_part_16d(cpart_i,window);
		//load_part_16d(cpart_j,Cj);

		uint32_t pflag_offset = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_offset = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + (cpart_i * 256 * 4);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + (cpart_j * 256 * 4);
		mram_ll_read1024_new(pbvec_addr,pbvec);
		mram_ll_read1024_new(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0;
	uint8_t qbit = 0;

#ifdef CHECK_ONLY_ACTIVE_POINTS
	calc_active_p(id);
#endif

	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
#ifdef CHECK_ONLY_ACTIVE_POINTS
		if( ((tflag >> qbit) & 0x1) == 0x0 ){ qbit++; continue; }//prune point only if it is alive from previous iteration
#endif
		uint32_t *q = &Cj[work_offset + j];
		uint16_t vi_offset = (work_offset + j) >> 4;//divide index by 16
		uint8_t Mi = (qbvec[vi_offset] & 0xFFFF);//16 bit vector
		uint8_t Qi = (qbvec[vi_offset] & 0xFFFF0000) >> 16;
		uint8_t dt = 0;

		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){

#ifdef CHECK_ONLY_ACTIVE_POINTS
			if (pflag_a[i>>4] == 0){ continue; } //Check if point is active// divide index by 16
#endif

			uint32_t *p = &window[i];
			uint16_t vj_offset = i >> 4;//divide index by 16
			uint8_t Mj = (pbvec[vj_offset] & 0xFFFF);//16 bit vector
			uint8_t Qj = (pbvec[vj_offset] & 0xFFFF0000) >> 16;

			if ((Mj | Mi) > Mi) continue;
			uint8_t ci = pc[Mi & 0xFF] + pc[Mi>>8];
			uint8_t cj = pc[Mj & 0xFF] + pc[Mj>>8];

			if(ci < cj) continue;
			else if((ci == cj) & (Mj != Mi)) continue;
			else if((Mi == Mj) & ((Qj | Qi) > Qi)) continue;
			else if( (((Mj | ~Mi) & Qj) | Qi) > Qi) continue;

			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				dt=1;
				break;
			}
		}

		qbit++;
	}
	mflags[id] = tflag;
	barrier_wait(id);
	if(id < 8) mflags[id << 1] = mflags[id << 1] | (mflags[(id << 1)+1] << 16);
	barrier_wait(id);
	if(id < 8) qflag[id] = mflags[id << 1];
	barrier_wait(id);
	if(id==0) mram_ll_write32(qflag,qflag_offset);
}

#endif
