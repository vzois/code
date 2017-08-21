#ifndef DSKY_SINGLE_DPU_H
#define DSKY_SINGLE_DPU_H

uint32_t test = 0;

void seq_test(){
	uint8_t id = me();
	init_v2(id);
	barrier_wait(id);
	int32_t i = 0 , j = 0;
	uint32_t p = P;

	uint8_t stop=0;
	if(D==4){
		//cmp_part_4d(id,0,P-1);
		for(i = 0;i<p;i++){ if(cmp_part_4d(id,i,i) == 1) break; }
		p = i;

		for(i = 0;i<p;i++){
			for(j = i+1;j<p;j++){
				cmp_part_4d(id,i,j);
			}
		}

//		for(i = 0;i<p;i++){
//			//if(stop_p[id] <= i) break;
//			for(j = 0;j<i;j++){
//				//if(stop_p[id] <= j) break;
//				if((stop=cmp_part_4d(id,j,i)) == 1) break;
//			}
//			if (stop==1) break;
//			cmp_part_4d(id,i,i);
//
//		}
//		//cmp_partb_4d(id,0);
	}
	else if(D==8){
		for(i = 0;i<p;i++){ if(cmp_part_8d(id,i,i) == 1) break; }
		p = i;

		for(i = 0;i<p;i++){
			for(j = i+1;j<p;j++){
				cmp_part_8d(id,i,j);
			}
		}
//		for(i = 0;i<p;i++){
//			if(stop_p[id] <= i) break;
//			for(j = 0;j<i;j++){
//				if(stop_p[id] <= j) break;
//				cmp_part_8d(id,j,i);
//				//if(stop_p[id] == i) break;
//			}
//			cmp_part_8d(id,i,i);
//		}
	}
	else if(D==16){
		for(i = 0;i<p;i++){ if(cmp_part_16d(id,i,i) == 1) break; }
		p = i;

		for(i = 0;i<p;i++){
			for(j = i+1;j<p;j++){
				cmp_part_16d(id,i,j);
			}
		}

//		for(i = 0;i<p;i++){
//			if(stop_p[id] <= i) break;
//			for(j = 0;j<i;j++){
//				if(stop_p[id] <= j) break;
//				cmp_part_16d(id,j,i);
//				//if(stop_p[id] == i) break;
//			}
//			cmp_part_16d(id,i,i);
//		}

	}
}

void batch_test(){
	uint8_t id = me();
	int32_t i,j;
	uint32_t p;

	if(test == 0){
		init_v2(id);
		i = 0;
		j = 0;
		p = 0;
		barrier_wait(id);
	}

	if (D==4){
		i = test * 1; // test * 32
		j = 0;
		uint32_t end = MIN(i + 1,P);
		barrier_wait(id);
		while((cmp_part_4d(id,j,i) == 1) && (j <= i) ){
			j++;
		}
	}else if (D==8){
		i = test * 1; // test * 32
		j = 0;
		uint32_t end = MIN(i + 1,P);
		barrier_wait(id);
		while((cmp_part_8d(id,j,i) == 1) && (j <= i) ){
			j++;
		}
	}else if (D==16){
		i = test * 1; // test * 32
		j = 0;
		uint32_t end = MIN(i + 1,P);
		barrier_wait(id);
		while((cmp_part_16d(id,j,i) == 1) && (j <= i) ){
			j++;
		}
	}
	barrier_wait(id);
	if(id == 0) test++;
}

#endif
