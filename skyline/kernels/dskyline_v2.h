#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

#define V_2 true

#ifdef V_2

uint32_t *window;
uint32_t *Cj;

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
	uint32_t pflag_offset = FLAGS_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t iter = (N / TASKLETS) / PSIZE;
	uint32_t i = 0;

	if(id < 1){
		pflag = dma_alloc(32);
		qflag = dma_alloc(32);
		window = dma_alloc(PSIZE_BYTES);
		Cj = dma_alloc(PSIZE_BYTES);
	}
	barrier_wait(id);

	if (id < 8) pflag[id] = 0xFFFFFFFF;
	//if (id < 8) pflag[id] = 0x0;
	barrier_wait(id);

	for(i = 0;i<iter;i++){
		mram_ll_write32(pflag,pflag_offset);
		pflag_offset+=32;
	}
}

void cmp_part_4d(uint8_t id, uint16_t cpart_i, uint16_t cpart_j){
	/*uint32_t tasklet_offset = (id << PSIZE_SHF) >> TASKLETS_SHF;
	uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
	uint32_t ppos_j = cpart_j << PSIZE_SHF;//Position of partition j
	uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );//Partition i starting addr
	uint32_t j_addr = DSKY_POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );//Partition j starting addr*/

	if(id == 0){
		uint32_t tasklet_offset = (id << PSIZE_SHF) >> TASKLETS_SHF;

		uint32_t ppos_i = cpart_i << PSIZE_SHF;//Position of partition i
		uint32_t i_addr = DSKY_POINTS_ADDR + ( (( ppos_i + tasklet_offset ) * D)<<2 );//Partition i starting addr
		mram_ll_read1024_new(i_addr, &window[0]);
		mram_ll_read1024_new(i_addr + 1024, &window[256]);
		mram_ll_read1024_new(i_addr + 2048, &window[512]);
		mram_ll_read1024_new(i_addr + 3072, &window[768]);

		uint32_t ppos_j = cpart_j << PSIZE_SHF;//Position of partition j
		uint32_t j_addr = DSKY_POINTS_ADDR + ( (( ppos_j + tasklet_offset ) * D)<<2 );
		mram_ll_read1024_new(j_addr, &Cj[0]);
		mram_ll_read1024_new(j_addr + 1024, &Cj[256]);
		mram_ll_read1024_new(j_addr + 2048, &Cj[512]);
		mram_ll_read1024_new(j_addr + 3072, &Cj[768]);

		uint32_t pflag_offset = FLAGS_ADDR + (cpart_i << 5);
		uint32_t qflag_offset = FLAGS_ADDR + (cpart_j << 5);
		mram_ll_read32(pflag_offset,pflag); // read flags of C_i set//used by all threads
		mram_ll_read32(qflag_offset,qflag); // read flags of C_j set//need to extract portion of tasklet

	}
	barrier_wait(id);

	uint8_t wid = id >> 1;
	uint16_t tflag = (id & 0x1) ? (qflag[wid])>>POINTS_PER_T : (qflag[wid] & 0xFFFF);
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
				//tflag &= ~(0x1 << qbit);//Set flag for dominace
				tflag = tflag ^ (0x1 << qbit);
				//count++;
				//dt = 1;
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
	if(id < 8) mflags[id] = mflags[id << 1];
}

#endif
