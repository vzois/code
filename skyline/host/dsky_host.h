#ifndef DSKY_HOST_H
#define DSKY_HOST_H

#include "../common/init.h"

//Distribute Partitions to available DPUS
void dsky_init_copy(){
	uint64_t i = 0;
	uint64_t j = 0;
	uint64_t  k = 0;

	uint32_t points_size = sizeof(uint32_t) * PSIZE * D;
	uint32_t points_addr = DSKY_POINTS_ADDR;
	uint32_t points_offset = 0;

	uint32_t rank_size = sizeof(uint32_t) * PSIZE;
	uint32_t rank_addr = DSKY_RANK_ADDR;
	uint32_t rank_offset = 0;

	uint32_t bvec_size = sizeof(uint32_t) * PSIZE;
	uint32_t bvec_addr = DSKY_BVECS_ADDR;
	uint32_t bvec_offset = 0;

	for(i = 0;i< P;i+=DPUS){
		if((k & 0x1) == 0){
			for(j = 0; j < DPUS; j++){
				copy_to_dpu(dpu_group[j], &points[points_offset], points_addr, points_size);//copy point partition
				copy_to_dpu(dpu_group[j], &rank[rank_offset], rank_addr, rank_size);//copy rank partition
				copy_to_dpu(dpu_group[j], &bvec[bvec_offset], bvec_addr, bvec_size);//copy bvec partition

				points_offset+= PSIZE*D;
				rank_offset+=PSIZE;
				bvec_offset+=PSIZE;
			}

		}else{
			for(j = DPUS; j >0; j--){
				copy_to_dpu(dpu_group[j-1], &points[points_offset], points_addr, points_size);//copy point partition
				copy_to_dpu(dpu_group[j-1], &rank[rank_offset], rank_addr, rank_size);//copy rank partition
				copy_to_dpu(dpu_group[j-1], &bvec[bvec_offset], bvec_addr, bvec_size);//copy bvec partition

				points_offset+=PSIZE*D;
				rank_offset+=PSIZE;
				bvec_offset+=PSIZE;
			}
		}

		//Skip to next partition address for each DPU
		points_addr+= PSIZE * D * 4;
		rank_addr+=PSIZE * 4;
		bvec_addr+=PSIZE * 4;
		k++;
	}

}

void dsky_phaseI(){
	uint64_t i = 0;
	printf("Executing Phase I\n");
	for(i = 0; i < DPUS;i++){
		//dpu_boot(dpu_group[i],NO_WAIT);
		dpu_boot(dpu_group[i],WAIT_UNTIL_PROGRAM_IS_COMPLETE);
		//dpu_status[i] = 1;
	}

	/*while(i<DPUS){
		if(dpu_get_status(dpu_group[i]) == STATUS_IDLE){
			i++;
		}
	}*/
}

void dsky_phaseII(){
	uint64_t i = 0;
	uint64_t j = 0;
	uint64_t k = 0;

	uint64_t points_offset = 0;
	uint32_t rank_offset = 0;
	uint32_t bvec_offset = 0;

	uint32_t part_point_size = PSIZE * D * 4;
	uint32_t part_rank_size = PSIZE * 4;
	uint32_t part_bvec_size = PSIZE * 4;
	uint32_t part_flags_size = (PSIZE / 32) * 4;
	uint32_t flags = (uint32_t *) malloc(part_flags_size);

	uint32_t dpu = 0;
	uint32_t round = 0;
	uint32_t flags_addr = DSKY_FLAGS_ADDR;
	uint32_t pstop = 0xFFFFFFFF;
	uint32_t skip = 1;
	uint8_t reverse = 0;

	for(i = 0; i<P;i++ ){
		copy_from_dpu(dpu_group[dpu], flags_addr, flags, part_flags_size);//Retrieve current partition flags
		copy_from_dpu(dpu_group[dpu], DSKY_PSTOP_ADDR, &pstop, sizeof(uint32_t));//Retrieve current partition pstop
		copy_to_dpu(dpu_group[dpu], VAR_0_ADDR,&skip,sizeof(uint32_t));//Signal this DPU to start processing from the next of the current partition

		for(j = 0; j<DPUS;j++){
			copy_to_dpu(dpu_group[j], &points[points_offset], DSKY_REMOTE_POINTS_ADDR_1, part_point_size);
			copy_to_dpu(dpu_group[j], &rank[rank_offset], DSKY_REMOTE_RANK_ADDR_1, part_rank_size);//copy rank partition
			copy_to_dpu(dpu_group[j], &bvec[bvec_offset], DSKY_REMOTE_BVECS_ADDR_1, part_bvec_size);//copy bvec partition
			copy_to_dpu(dpu_group[j],&flags, DSKY_REMOTE_FLAGS_ADDR_1,part_flags_size);//copy flags for current pruned partition
			copy_to_dpu(dpu_group[j],&pstop,DSKY_REMOTE_PSTOP_ADDR_1,sizeof(uint32_t));//copy current remote partition to update it on all DPUs
		}

		//Call Kernel Here


		//Update Offsets and ADDR
		dpu = (reverse==0) ? dpu+1 : dpu-1;//iterate dpus forward and backward
		if(dpu == DPUS){//if reached the last dpu // reverse
			reverse^=1;
			dpu = DPUS - 1;//Start from last DPU
			flags_addr+=4;//step to next partition flag
		}else if(dpu == -1){
			reverse^=1;
			dpu = 0;//Start from DPU zero
			flags_addr+=4;//step to next partition flag
		}

		points_offset+= PSIZE*D;
		rank_offset+=PSIZE;
		bvec_offset+=PSIZE;
	}
}

void dsky_host(){
	init_dpu(multi_dpu_dsky);
	loadDSkyMramData();

	dsky_init_copy();
	dsky_phaseI();



	freeDSkyMem();
	free_dpu();
}

#endif
