#ifndef RSELECT_H
#define RSELECT_H

#include "common/init.h"
#include "common/time.h"

static uint32_t num = 0;

void extract_window(){
	uint32_t i = 0;
	uint32_t dpu_count[DPUS];
	uint32_t start, sum = 0;
	uint32_t arg;

	arg = CWINDOW;
	for(i = 0; i < DPUS;i++){ copy_to_dpu(dpu_group[i], &arg, VAR_0_ADDR, sizeof(arg)); }
	arg = num;
	for(i = 0; i < DPUS;i++){ copy_to_dpu(dpu_group[i], &arg, VAR_1_ADDR, sizeof(arg)); }

	for(i = 0; i < DPUS;i++){
		dpu_boot(dpu_group[i],WAIT_UNTIL_PROGRAM_IS_COMPLETE);
		dpu_status[i] = 1;
	}

	for(i = 0; i < DPUS;i++){
		copy_from_dpu(dpu_group[i], VAR_0_ADDR, &dpu_count[i], sizeof(uint32_t));
		//printf("count: %d\n",dpu_count[i]);
		sum+=dpu_count[i];
		dpu_status[i] = 0;
	}

	//printf("sum: %d\n",sum);

	start=0;
	sum = 0;
	for(i = 0;i<DPUS;i++){
		//printf("b: %d\n",sum);
		copy_from_dpu(dpu_group[i], DATA_ADDR, &window[sum], sizeof(uint32_t)*dpu_count[i]);
		start=dpu_count[i];
		dpu_count[i] = sum;
		sum+=start;
		//dpu_count[i+1]+=dpu_count[i];
	}

	/*uint32_t j=0;
	for(i = 0;i<DPUS-1;i++){
		for(j = dpu_count[i];j<dpu_count[i+1];j++){
			printf("%x ",window[j] + i * N);
		}
		printf("\n");
	}*/


}

void singleDPU_rselect(){
	init_dpu(single_dpu_rselect);
	loadMramRank();

	uint32_t i = 0;
	for(i = 0; i < DPUS;i++){
		dpu_boot(dpu_group[i],WAIT_UNTIL_PROGRAM_IS_COMPLETE);
	}
	copy_from_dpu(dpu_group[0], DATA_ADDR, &num, sizeof(num));
	printf("k-th largest element: 0x%x\n",num);

	free_dpu();
}

void multiDPU_rselect(){
	init_dpu(multi_dpu_rselect);
	loadMramRank();

	uint32_t tmpK = K;
	int32_t d=0x0;
	uint8_t shf=0x0;//shift amount
	uint32_t mD=0x0;//digit being examined mask
	uint32_t mN=0x0;//digits discovered mask

	uint32_t buffer[4];
	uint32_t sumBins[17];
	uint32_t tmpBins[17];
	uint32_t i = 0, j = 0 , k = 0;
	uint32_t arg;

	printf("Computing k-th Largest Element!!!\n");
	arg = CSELECT;
	for(i = 0; i < DPUS;i++){ copy_to_dpu(dpu_group[i], &arg, VAR_0_ADDR, sizeof(arg)); }
	for(d = 7; d > -1;d--){
		shf = d << 2;
		mD = 0xF << shf;
		buffer[0] = num; buffer[1] = shf; buffer[2] = mD; buffer[3] = mN;

		//copy to dpu configurations for current iterations//
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		for(i = 0; i < DPUS;i++){ copy_to_dpu(dpu_group[i], buffer, BUCKET_ADDR, sizeof(buffer)); }
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		elapsed(start,end);
		//Execute Kernel
		for(i = 0; i < DPUS;i++){
			dpu_boot(dpu_group[i],WAIT_UNTIL_PROGRAM_IS_COMPLETE);
			//dpu_boot(dpu_group[i],NO_WAIT);
			dpu_status[i] = 1;
		}

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
		for(i = 0; i < 17; i++) sumBins[i] = 0;
		k = 0;
		//while(k < DPUS){//Wait for DPUs to finish
			for(i = 0; i < DPUS;i++){
				//if(dpu_get_status(dpu_group[i]) == STATUS_IDLE && dpu_status[i] == 1){
					copy_from_dpu(dpu_group[i], BUCKET_ADDR, tmpBins, sizeof(tmpBins));
					for(j = 0; j < 17; j++){ sumBins[j] += tmpBins[j]; }//Gather results from each DPU
					dpu_status[i] = 0;
					k++;
				//}
			}
		//}
		//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		//elapsed(start,end);

		//Accumulate Results & Extract Digit From Candidate
		for(i = 1;i<16;i++){ sumBins[i+1]+=sumBins[i]; }
		uint8_t digit = 0x0;
		for(j = 1;j < 17;j++){
			if (tmpK <= sumBins[j]){
				digit = j;
				break;
			}
		}
		if (tmpK != sumBins[j-1]){ tmpK = tmpK - sumBins[j-1]; }
		num = num | ((digit-1) << shf );
		mN = mN | mD;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		elapsed(start,end);
	}
	printf("k-th largest element: %d 0x%x\n",num,num);

	printf("Extracting Windows Indices!!!\n");
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	extract_window();
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
	elapsed(start,end);

	free_dpu();
}

#endif
