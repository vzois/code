//#include "dskyline_v1.h"
#include "dskyline_v2.h"

//#define BULK_BENCHMARK
//#define PAR_DSKY

void count_sky_points(uint32_t p){//Debug Only
	uint32_t i = 0, j=0;
	uint32_t pflag_offset = DSKY_FLAGS_ADDR;

	//uint32_t *pflag = pflags[0];
	uint32_t *r = mem_alloc_dma(4);
	uint32_t *rp = mem_alloc_dma(P << 2);
	r[0] = 0;
	for(i = 0;i<p;i++){
		rp[i] = r[0];
		mram_read32(pflag_offset,pflag);
		for(j = 0; j < 8;j++){

			popC(pflag[j],r);
		}
		rp[i] = r[0] - rp[i];

		pflag_offset+=32;
	}
	mram_write128(rp,0);
	debug = r[0];
}

uint8_t test = 0;
#ifndef PAR_DSKY

#ifdef BULK_BENCHMARK

int main(){
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
	return 0;
}

#else

int main(){
	uint8_t id = me();
	init_v2(id);
	barrier_wait(id);
	int32_t i = 0 , j = 0;
	uint32_t p = P;

	if(D==4){
		//cmp_part_4d(id,0,P/2);
		//cmp_part_4d(id,0,1);
		//cmp_part_4d(id,1,1);
		for(i = 0;i<p;i++){
			if(stop_p[id] <= i) break;
			for(j = 0;j<i;j++){
				if(stop_p[id] <= j) break;
				cmp_part_4d(id,j,i);
			}
			cmp_part_4d(id,i,i);
		}

		for(i = stop_p[id] + id; i<p;i+=TASKLETS){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (i << 5);
			mram_write32(qclear,qflag_addr);
		}
	}
	else if(D==8){
		//cmp_part_8d(id,0,P/2);
		//cmp_part_8d(id,0,1);
		//cmp_part_8d(id,1,1);
		for(i = 0;i<p;i++){
			if(stop_p[id] <= i) break;
			for(j = 0;j<i;j++){
				if(stop_p[id] <= j) break;
				cmp_part_8d(id,j,i);
				//if(stop_p[id] == i) break;
			}
			cmp_part_8d(id,i,i);
		}

		for(i = stop_p[id] + id; i<p;i+=TASKLETS){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (i << 5);
			mram_write32(qclear,qflag_addr);
		}
	}
	else if(D==16){
		//cmp_part_16d(id,0,P/2);
		//cmp_part_16d(id,0,1);
		//cmp_part_16d(id,1,1);
		for(i = 0;i<p;i++){
			if(stop_p[id] <= i) break;
			for(j = 0;j<i;j++){
				if(stop_p[id] <= j) break;
				cmp_part_16d(id,j,i);
				//if(stop_p[id] == i) break;
			}
			cmp_part_16d(id,i,i);
		}

		for(i = stop_p[id] + id; i<p;i+=TASKLETS){
			uint32_t qflag_addr = DSKY_FLAGS_ADDR + (i << 5);
			mram_write32(qclear,qflag_addr);
		}
	}

	//if(id==0) count_sky_points(P);
	return 0;
}

#endif
#else

uint32_t phase = 0;

void phase_I(){
	uint8_t id = me();
	init_v2(id);
	barrier_wait(id);
	int32_t i = 0;

	if(D == 4){ for(i = 0;i<P;i++) cmp_part_4d(id,i,i); }
	else if(D == 8){ for(i = 0;i<P;i++) cmp_part_8d(id,i,i); }
	else if(D == 16){ for(i = 0;i<P;i++) cmp_part_16d(id,i,i); }
	barrier_wait(id);
	if(id == 0) phase = 1;
}

void phase_II(){
	//Single Buffer Implementation
	uint8_t id = me();

	mram_read8(DSKY_REMOTE_PART_ID,param[id]);//Find which pruned partition i am processing
	uint32_t part_id = param[id][0];
	if( id == 0 ){
		mram_read8(DSKY_REMOTE_PSTOP_ADDR_1,param[id]);//Always read from single buffer addr 1
		g_ps = MIN(param[id][0],g_ps);//Load global g_ps
	}
	barrier_wait(id);

	int32_t j = 0;
	if(D == 4){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_4d(id,P,j); }
	if(D == 8){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_8d(id,P,j); }
	if(D == 16){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_16d(id,P,j); }
}

void dbuffer_phase_II(){
	//Single Buffer Implementation
	uint8_t id = me();

	if(((phase - 1) & 0x1) == 0){
		mram_read8(DSKY_REMOTE_PART_ID,param[id]);//Find which pruned partition i am processing
		uint32_t part_id = param[id][0];
		if( id == 0 ){
			mram_read8(DSKY_REMOTE_PSTOP_ADDR_1,param[id]);//Always read from single buffer addr 1
			g_ps = MIN(param[id][0],g_ps);//Load global g_ps
		}
		if(id==0) phase++;
		barrier_wait(id);
		if(part_id >= stop[id]) return;

		int32_t j = 0;
		if(D == 4){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_4d(id,P,j); }
		if(D == 8){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_8d(id,P,j); }
		if(D == 16){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_16d(id,P,j); }
	}else{
		mram_read8(DSKY_REMOTE_PART_ID,param[id]);//Find which pruned partition i am processing
		uint32_t part_id = param[id][0];
			if( id == 0 ){
			mram_read8(DSKY_REMOTE_PSTOP_ADDR_2,param[id]);//Always read from single buffer addr 1
			g_ps = MIN(param[id][0],g_ps);//Load global g_ps
		}
		if(id==0) phase++;
		barrier_wait(id);

		if(part_id >= stop[id]) return;

		int32_t j = 0;
		if(D == 4){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_4d(id,P+1,j); }
		if(D == 8){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_8d(id,P+1,j); }
		if(D == 16){ for( j = part_id + 1 ; j < P ; j++ ) cmp_part_16d(id,P+1,j); }
	}
}

int main(){
	uint8_t id = me();
	if(phase == 0) phase_I();
	else phase_II();

	return 0;
}

#endif
