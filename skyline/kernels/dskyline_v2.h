#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

#define V_2 true

#ifdef V_2

uint32_t *window;
uint32_t *Cj;

uint32_t *pbvec;
uint32_t *qbvec;

uint32_t *pflag;
uint32_t *qflag;
uint32_t mflags[TASKLETS];

uint32_t debug;


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
	}
	barrier_wait(id);

	if (id < 8) pflag[id] = 0xFFFFFFFF;
	barrier_wait(id);

	for(i = 0;i<iter;i++){
		mram_ll_write32(pflag,pflag_offset);
		pflag_offset+=32;
	}
}

void load_part_4d(uint16_t cpart_i, uint32_t *buffer){
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i ) * D)<<2 );//Partition i starting addr
	mram_ll_read1024_new(i_addr, &buffer[0]);
	mram_ll_read1024_new(i_addr + 1024, &buffer[256]);
	mram_ll_read1024_new(i_addr + 2048, &buffer[512]);
	mram_ll_read1024_new(i_addr + 3072, &buffer[768]);
}

void cmp_part_4d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	uint32_t qflag_addr;
	if(id == 0){
		load_part_4d(cpart_i,window);
		load_part_4d(cpart_j,Cj);

		uint32_t pflag_addr = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_addr = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_addr,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_addr,qflag); // read flags of C_j set//need to extract portion of tasklet

		uint32_t pbvec_addr = DSKY_BVECS_ADDR + (cpart_i << 2);
		uint32_t qbvec_addr = DSKY_BVECS_ADDR + (cpart_j << 2);
		mram_ll_read1024_new(pbvec_addr,pbvec);
		mram_ll_read1024_new(qbvec_addr,qbvec);
	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//
	uint32_t i = 0, j = 0, k = 0, m = 0;
	uint8_t qbit = 0;
	uint8_t count = 0;

	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
		uint32_t *q = &Cj[work_offset];
		uint8_t dt = 0;
		uint8_t Mi = (qbvec[j>>1] & 0xF);
		//uint8_t Qi = (qbvec[k] & 0xF0);

		m = 0;
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
			uint32_t *p = &window[i];
			uint8_t Mj = (pbvec[i>>1] & 0xF);
			//uint8_t Qj = (pbvec[i>>1] & 0xF0);

			//if ((Mi | Mj) > Mj) continue;

			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				break;
			}
		}
		qbit++;
		work_offset+=D;
	}
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
	if(id == 0){
		uint32_t tasklet_offset = (id << PSIZE_SHF) >> TASKLETS_SHF;

		uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
		uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );//Partition i starting addr
		mram_ll_read1024_new(i_addr, &window[0]);//0
		mram_ll_read1024_new(i_addr + 1024, &window[256]);//1
		mram_ll_read1024_new(i_addr + 2048, &window[512]);//2
		mram_ll_read1024_new(i_addr + 3072, &window[768]);//3
		mram_ll_read1024_new(i_addr + 4096, &window[1024]);//4
		mram_ll_read1024_new(i_addr + 5120, &window[1280]);//5
		mram_ll_read1024_new(i_addr + 6144, &window[1536]);//6
		mram_ll_read1024_new(i_addr + 7168, &window[1792]);//7

		uint32_t ppos_j = cpart_j << PSIZE_SHF;//Position of partition j
		uint32_t j_addr = DSKY_POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );
		mram_ll_read1024_new(j_addr, &Cj[0]);
		mram_ll_read1024_new(j_addr + 1024, &Cj[256]);
		mram_ll_read1024_new(j_addr + 2048, &Cj[512]);
		mram_ll_read1024_new(j_addr + 3072, &Cj[768]);
		mram_ll_read1024_new(j_addr + 4096, &Cj[1024]);
		mram_ll_read1024_new(j_addr + 5120, &Cj[1280]);
		mram_ll_read1024_new(j_addr + 6144, &Cj[1536]);
		mram_ll_read1024_new(j_addr + 7168, &Cj[1792]);

		uint32_t pflag_offset = DSKY_FLAGS_ADDR + (cpart_i << 5);
		qflag_offset = DSKY_FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet

	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);// each pair of tasklets assigned to a single 32-bit mask
	uint32_t work_offset = POINTS_PER_T_VALUES * id;//

	uint32_t i = 0, j = 0;
	uint8_t qbit = 0;

	uint8_t count = 0;
	for(j = 0 ; j < POINTS_PER_T_VALUES; j+=D){
		uint32_t *q = &Cj[work_offset + j];
		uint8_t dt = 0;
		for(i = 0;i<PSIZE_POINTS_VALUES; i+=D){
			uint32_t *p = &window[i];
			//uint8_t dt = DT(p,q);
			if(DT_(p,q) == 1){
				tflag &= ~(0x1 << qbit);//Set flag for dominance
				break;
			}
		}

		qbit++;
	}
	//barrier_wait(id);
	mflags[id] = tflag;
	barrier_wait(id);
	if(id < 8) mflags[id << 1] = mflags[id << 1] | (mflags[(id << 1)+1] << 16);
	barrier_wait(id);
	if(id < 8){
		//mflags[id] = mflags[id << 1];
		qflag[id] = mflags[id << 1];
		//qflag[id] = id;
	}
	barrier_wait(id);
	if(id==0) mram_ll_write32(qflag,qflag_offset);
	//barrier_wait(id);
}

#endif
