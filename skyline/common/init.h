#ifndef INIT_H
#define INIT_H

#include "config.h"
#include "dpu.h"

static const char single_dpu_rselect[] = "./radixselect.bin";
static const char multi_dpu_rselect[] = "./pradixselect.bin";
static const char multi_dpu_skyline[] = "./kss.bin";

dpu_t dpu_group[DPUS];

static uint8_t dpu_status[DPUS];
static uint32_t window[K];

static const char runtime_sum[] = "runtime_data/sum.bin";
static const char runtime_points[] = "runtime_data/points.bin";

void loadMramRank(){
	FILE *f = fopen(runtime_sum, "rb");
	if (f == NULL) {fputs ("File error",stderr); exit (1);}

	uint32_t dpu_part = N;
	uint32_t tasklet_part = N / TASKLETS;
	uint32_t *buffer = (uint32_t*) malloc (sizeof(uint32_t)*dpu_part);

	printf("dpu_count: %d, data: %d, dpu chunk: %d, tasklet chunk: %d\n",DPUS,DATA_N,N, tasklet_part);
	uint32_t bytes = 0;
	uint32_t dpu_i = 0;
	fseek ( f , 4096 , SEEK_SET );
	while(bytes < DATA_N){
		bytes+=fread(buffer,sizeof(uint32_t), dpu_part, f);
		printf("Copying to DPU (%d)\n",dpu_i);
		copy_to_dpu(dpu_group[dpu_i], buffer, DATA_ADDR, sizeof(uint32_t)*dpu_part);
		dpu_i++;
	}

	free(buffer);
	printf("Copy Data Completed!!!\n");
	fclose(f);
}

void loadMramPoints(){
	FILE *f = fopen(runtime_points, "rb");
	if (f == NULL) {fputs ("File error",stderr); exit (1);}

	uint32_t bytes = 0;
	fseek ( f , 4096 , SEEK_SET );// First 4096 bytes are reserved
	uint32_t *buffer = (uint32_t*) malloc (sizeof(uint32_t)*N*D);
	uint32_t dpu_i = 0;

	//Copy Points//
	while(bytes < (DATA_N * D)){
		bytes+=fread(buffer,sizeof(uint32_t), N*D, f);
		//printf("%d\n",bytes);
		printf("Copying points to DPU (%d)\n",dpu_i);
		copy_to_dpu(dpu_group[dpu_i], buffer, POINTS_ADDR, sizeof(uint32_t)*N*D);
		dpu_i++;
	}

	printf("Finished Transferring Point Partitions!!!\n");
	free(buffer);
	buffer = (uint32_t*) malloc (sizeof(uint32_t)*K*D);
	bytes = fread(buffer,sizeof(uint32_t), K*D, f);
	dpu_i=0;
	//Copy window to DPU
	while(dpu_i < DPUS){
		printf("Copying window to DPU (%d)\n",dpu_i);
		copy_to_dpu(dpu_group[dpu_i], buffer, WINDOW_ADDR, sizeof(uint32_t)*K*D);
		dpu_i++;
	}
	//Copy rank also to improve processing//
	free(buffer);
	fclose(f);
}

void init_dpu(const char dpu_bin[]){
	uint32_t i = 0;
	for(i = 0; i < DPUS;i++){
		dpu_group[i] = dpu_get();

		if(dpu_group[i] != NO_DPU_AVAILABLE){
			if(!dpu_load(dpu_group[i], dpu_bin)){
				printf("Failed loading program\n");
				dpu_free(dpu_group[i]);
				exit(1);
			}
		}else{
			printf("Failed Acquiring DPU (%d) \n",i);
			dpu_free(dpu_group[i]);
			exit(1);
		}
	}
}

void free_dpu(){
	uint32_t i = 0;
	for(i = 0; i < DPUS;i++){
		dpu_free(dpu_group[i]);
	}
}



#endif
