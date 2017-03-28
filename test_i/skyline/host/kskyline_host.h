#ifndef KSKYLINE_HOST_H
#define KSKYLINE_HOST_H

#include "common/init.h"
#include "common/time.h"

void compute_kskyline(){
	init_dpu(multi_dpu_skyline);
	loadMramPoints();

	uint32_t i = 0 , j = 0;
	for(i = 0; i < DPUS;i++){
		printf("Executing kskyline on DPU (%d)\n",i);
		//dpu_boot(dpu_group[i],WAIT_UNTIL_PROGRAM_IS_COMPLETE);
		dpu_boot(dpu_group[i],NO_WAIT);
		dpu_status[i] = 1;
	}

	uint32_t bit_vectors[DATA_N/32];
	uint32_t dpu_part = (N>>5)*sizeof(uint32_t);
	uint32_t offset = 0;
	uint32_t count = 0 , k = 0;

	///////////////////////////////////////////
	//Gather Bit vectors from DPU
	while(k < DPUS){//Wait for DPUs to finish
		for(i = 0; i < DPUS;i++){
			if(dpu_get_status(dpu_group[i]) == STATUS_IDLE && dpu_status[i] == 1){
				offset=i*(N>>5);
				clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
				copy_from_dpu(dpu_group[i], MASK_ADDR, &bit_vectors[offset], dpu_part);//Gather results from prunning
				copy_from_dpu(dpu_group[i], 0, &offset, sizeof(uint32_t));//Get count of candidates
				count+=offset;
				clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
				elapsed(start,end);

				dpu_status[i] = 0;
				k++;
			}
		}
	}
	printf("count:%d %x\n",count,count);

	///////////////////////////////////////////
	//Compute Indices from bit vectors
	uint32_t *buffer, *indices, *rank;
	indices=(uint32_t*) malloc (sizeof(uint32_t)*count);
	count=0;
	for(i=0;i<DATA_N/32;i++){
		//printf("%x\n",bit_vectors[i]);
		for(j=0;j<32;j++){//Unpack bit vectors
			if((bit_vectors[i] & (0x1<<j))>>j == 1){
				indices[count]=i*32 + j;//Compute indices of points in candidate set
				if(count < 25) printf("%d ",indices[count]);
				count++;
			}
		}
	}
	printf("\n");
	printf("count:%d %x\n",count,count);

	///////////////////////////////////////////////////////
	//Seek Candidates from binary file
	FILE *f = fopen(runtime_points, "rb");
	FILE *f2 = fopen(runtime_sum, "rb");
	fseek ( f , 4096 , SEEK_SET );
	buffer = (uint32_t*) malloc (sizeof(uint32_t)*count*D);
	rank = (uint32_t*) malloc (sizeof(uint32_t)*count);
	offset = 0;
	uint32_t roffset = 0;
	for(i = 0; i < count;i++){//Load candidate set
		fseek ( f , 4096 + indices[i] * D * sizeof(uint32_t) , SEEK_SET );
		fseek ( f2 , 4096 + indices[i] * sizeof(uint32_t) , SEEK_SET );
		offset+=fread(&buffer[offset],sizeof(uint32_t), D, f);
		roffset+=fread(&rank[roffset],sizeof(uint32_t), 1, f2);
	}
	free(indices);
	fclose(f);
	fclose(f2);

	//Debug// Correct points loaded
	offset=0;
	for(j=0;j < 10;j++){
		roffset=0;
		for(k=0;k<D;k++){
			roffset+=buffer[offset + k];
			//printf("0x%x ",buffer[offset + k]);
		}
		//printf("<%d %d> %x",roffset,rank[j], roffset);
		//printf("\n");
		offset+=D;
	}


	///////////////////////////////////////////////
	//Distribute candidates among available DPUS
	uint32_t low,high;
	char file[] = {"candidates0.bin"};
	uint32_t empty[1021]={0};
	uint32_t dpus = MIN(((count -1) / MERGE_CHUNK) + 1,2048);
	printf("merge dpus: %d\n",dpus);
	for(i = 0; i < 1;i++){//split candidates among different DPUs
		FILE *fout = fopen(file, "wb");
		low = (i * count) / dpus;//low processing point
		high = ((i+1) * count) / dpus;//high processing point
		//file[10]++;
		//printf("%s\n",file);

		//Copy to dpu low, high, count and complete candidate set
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		copy_to_dpu(dpu_group[i], &low, VAR_0_ADDR, sizeof(uint32_t));//Copy Points to DPU
		copy_to_dpu(dpu_group[i], &high, VAR_1_ADDR, sizeof(uint32_t));//Copy Points to DPU
		copy_to_dpu(dpu_group[i], &count, VAR_2_ADDR, sizeof(uint32_t));//Copy Points to DPU
		copy_to_dpu(dpu_group[i], buffer, POINTS_ADDR, sizeof(uint32_t)*count*D);//Copy Points to DPU
		copy_to_dpu(dpu_group[i], rank, POINTS_ADDR + sizeof(uint32_t)*count*D, sizeof(uint32_t)*count);//Copy Points to DPU
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		elapsed(start,end);

		//Write Binary file with low,high,count, empty space and complete candidate set
		fwrite(&low , sizeof(uint32_t), 1, fout);
		fwrite(&high , sizeof(uint32_t), 1, fout);
		fwrite(&count , sizeof(uint32_t), 1, fout);
		fwrite(&empty , sizeof(uint32_t), 1021, fout);
		fwrite(buffer , sizeof(uint32_t), count*D, fout);//Create bin for benchmarking//
		fwrite(rank , sizeof(uint32_t), count, fout);
		fclose(fout);
	}

	//printf("Elapsed CPU Time %d milliseconds",total%1000);
	free(buffer);
	free(rank);
	free_dpu();
}

#endif
