/*
 * Prune false candidates from skyline.
 */

#include<defs.h>
#include<alloc.h>
#include <mram_ll.h>

#include "../common/config.h"
#include "../common/tools.h"

uint32_t DT_[TASKLETS];

uint8_t DT(uint32_t *p, uint32_t *q){
	uint8_t i = 0;
	uint8_t pb = 0;
	uint8_t qb = 0;

	for(i=0;i<D;i++){
		pb = ( p[i] < q[i] ) | pb;//At least one dimension better
		qb = ( q[i] < p[i] ) | qb;//No dimensions better
		if(qb == 1) break;//Optional//
	}
	return ~qb & pb;//p dominates q 0xf0d6a 0xb4d08 0xf7243 0xe0e71 point
								  //0x  30f 0x6084c 0x86bb6 0x8f1b3 window
}

void kskyline_merge(uint8_t id){
	uint32_t *buffer=dma_alloc(WRAM_BUFFER);
	uint32_t *cmp = dma_alloc(WRAM_BUFFER);
	uint32_t *qrank = dma_alloc(256);
	uint32_t *prank = dma_alloc(256);
	uint32_t low = 0, high = 0, size = 0, count = 0;
	uint32_t i = 0, j = 0 , k = 0, m = 0;

	mram_ll_read32(0, buffer);
	low = buffer[0];//low point to check for given DPU
	high = buffer[1];//high point to check for given DPU
	count = buffer[2];//number of points in the candidate set
	size = (high - low);//number of point this DPU is responsible for

	uint32_t llow = (id * size / TASKLETS);//low point for given tasklet
	uint32_t lhigh = ((id+1) * size / TASKLETS);//high point for given tasklet
	uint32_t psize_bytes = (count * D) << 2;//size of candidate points
	uint32_t qsize_bytes = ((low + llow) * D) << 2;//bytes in DPU partition
	uint32_t qrank_bytes = (low + llow)<<2;

	uint32_t p_offset = POINTS_ADDR;//starting address of the corresponding candidate set
	uint32_t q_offset = POINTS_ADDR + qsize_bytes;//starting point for given tasklet of the corresponding DPU
	uint32_t prank_offset = POINTS_ADDR + psize_bytes;//starting point of rank for candidate set
	uint32_t qrank_offset = POINTS_ADDR + psize_bytes + qrank_bytes;//starting point of rank for given partition

	uint8_t dt = 0;
	//uint32_t *mask = dma_alloc(32);
	//uint32_t mask_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t *alive = dma_alloc((MERGE_CHUNK/TASKLETS)>>3);//(((MERGE_CHUNK/TASKLETS)/32)*4) >>4 >>5 <<2
	uint8_t pos = 0;
	uint8_t bit = 0;

	uint8_t qcount = 64;
	uint8_t pcount = 64;
	//for(i = 0;i<8;i++) mask[i] = 0xFFFFFFFF;
	for(k = 0 ; k < (MERGE_CHUNK>>5); k++) alive[k]= 0xFFFFFFFF;
	for(i = llow; i < lhigh; i+=POINTS_PER_BUFFER){//For group of q points in the part this tasklet is checking
		mram_ll_read1024_new(q_offset,buffer);//Load part of the q points, we need to check

		for(j = 0; j < VALUES_PER_BUFFER; j+=D){//For each q point in the buffer
			uint32_t *q = &buffer[j];
			if( qcount == 64){
				mram_ll_read256(qrank_offset,qrank);//Load 64 L(q) values
				qrank_offset+=256;//increment offset by 256 bytes
				qcount = 0;//reset counter
			}
			uint32_t lq = qrank[qcount];

			p_offset = POINTS_ADDR + psize_bytes;
			prank_offset = POINTS_ADDR + psize_bytes;
			dt = 0; pcount = 64;
			for(k = 0; k < count; k+=POINTS_PER_BUFFER){//For each group of p points in the candidate set
				mram_ll_read1024_new(p_offset,cmp);//Load part of the p points in the candidate set

				for(m = 0; m <VALUES_PER_BUFFER;m+=D){//For each p point in the group
					if(pcount == 64){
						mram_ll_read256(prank_offset,prank);//Load 64 L(p) values
						prank_offset+=256;//increment offset by 256 bytes
						pcount = 0;//reset counter
					}
					uint32_t lp = prank[pcount];

					if(lp < lq){//If L(q) <= L(p) , p does not dominate q
						uint32_t *p = &cmp[m];
						if(DT(p,q) == 0x1){//Check if p from candidate set dominates q from tasklet chunk set
							dt=1;
							alive[pos] = alive[pos] ^ (0x1 << bit);
							break;
						}
					}
					pcount++;
				}
				if(dt == 1) break;
				p_offset+=WRAM_BUFFER;
			}
			qcount++;
			bit = qcount & 0x1F;//calculate relative position inside bit vector
			pos = (qcount & 0xFFFFFFE0) >> 5;//calculate the bit vector being used
		}

		q_offset+=WRAM_BUFFER;
	}
}

void kskyline_merge2(uint8_t id){
	uint32_t *buffer=dma_alloc(WRAM_BUFFER);
	uint32_t *cmp = dma_alloc(WRAM_BUFFER);
	uint32_t *qrank = dma_alloc(256);
	uint32_t *prank = dma_alloc(256);
	uint32_t low = 0, high = 0, size = 0, count = 0;
	uint32_t i = 0, j = 0 , k = 0, m = 0;

	mram_ll_read32(0, buffer);
	low = buffer[0];//low point to check for given DPU
	high = buffer[1];//high point to check for given DPU
	count = buffer[2];//number of points in the candidate set
	size = (high - low);//number of point this DPU is responsible for

	uint32_t llow = (id * size / TASKLETS);//low point for given tasklet
	uint32_t lhigh = ((id+1) * size / TASKLETS);//high point for given tasklet
	uint32_t psize_bytes = (count * D) << 2;//size of candidate points
	uint32_t qsize_bytes = ((low + llow) * D) << 2;//bytes in DPU partition
	uint32_t qrank_bytes = (low + llow)<<2;

	uint32_t p_offset = POINTS_ADDR;//starting address of the corresponding candidate set
	uint32_t q_offset = POINTS_ADDR + qsize_bytes;//starting point for given tasklet of the corresponding DPU
	uint32_t prank_offset = POINTS_ADDR + psize_bytes;//starting point of rank for candidate set
	uint32_t qrank_offset = POINTS_ADDR + psize_bytes + qrank_bytes;//starting point of rank for given partition

	uint8_t dt = 0;
	uint32_t *mask = dma_alloc(32);
	uint32_t mask_offset = MASK_ADDR + id * (N >> 7);//offset by 1 byte for each point
	uint32_t *alive = dma_alloc((MERGE_CHUNK/TASKLETS)>>3);//(((MERGE_CHUNK/TASKLETS)/32)*4) >>4 >>5 <<2
	uint32_t pos = 0;
	uint8_t bit = 0;

	uint8_t qcount = 64;
	uint8_t pcount = 64;


	for(k = 0 ; k < (MERGE_CHUNK>>5); k++) alive[k]= 0xFFFFFFFF;
	for(k = 0; k < count; k+=POINTS_PER_BUFFER){//For each batch of p points
		mram_ll_read1024_new(p_offset,cmp);

		for(m = 0; m <VALUES_PER_BUFFER;m+=D){//For each p point in the group
			if(pcount == 64 ){
				mram_ll_read256(prank_offset,prank);//Load 64 L(p) values
				prank_offset+=256;//increment offset by 256 bytes
				pcount = 0;//reset counter
			}
			uint32_t lp = prank[pcount];

			qrank_offset = POINTS_ADDR + psize_bytes + qrank_bytes;
			q_offset = POINTS_ADDR + qsize_bytes;
			qcount = 64;
			for(i = llow; i < lhigh; i+=POINTS_PER_BUFFER){//For each batch of q points
				mram_ll_read1024_new(q_offset,buffer);//Load part of the q points, we need to check

				for(j = 0; j < VALUES_PER_BUFFER; j+=D){//For each q point in the buffer
					if( qcount == 64 ){
						mram_ll_read256(qrank_offset,qrank);//Load 64 L(q) values
						qrank_offset+=256;//increment offset by 256 bytes
						qcount = 0;//reset counter
					}
					uint32_t lq = qrank[qcount];

					dt = 0;
					if(lp < lq && ((alive[pos] & (0x1 << bit)) != 0) ){//If L(q) <= L(p) , p does not dominate q
						uint32_t *q = &buffer[j];
						uint32_t *p = &cmp[m];
						if(DT(p,q) == 0x1 ){//Check if p from candidate set dominates q from DPU/tasklet chunk set
							dt=1;
							alive[pos] = alive[pos] ^ (0x1 << bit);
						}
					}
					qcount++;
					bit = qcount & 0x1F;
					pos = (qcount & 0xFFFFFFE0) >> 5;
				}
				q_offset+=WRAM_BUFFER;
			}
			pcount++;
		}
		p_offset+=WRAM_BUFFER;
	}

}

int main(){
	uint8_t id = 0;
	kskyline_merge(id);
}
