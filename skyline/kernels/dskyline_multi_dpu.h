#ifndef DSKY_MULTI_DPU_H
#define DSKY_MULTI_DPU_H

uint32_t phase = 0;
uint8_t step = 0;
uint32_t part = 0;

void phase_I(){
	uint8_t id = me();
	init_v2(id);
	barrier_wait(id);
	uint32_t i = 0;

	if(D == 4){ for(i = 0;i<P;i++) cmp_part_4d(id,i,i); }
	else if(D == 8){ for(i = 0;i<P;i++) cmp_part_8d(id,i,i); }
	else if(D == 16){ for(i = 0;i<P;i++) cmp_part_16d(id,i,i); }
	barrier_wait(id);
	if(id == 0){
		param[id][0] = g_ps;
		mram_write8(param[id],DSKY_PSTOP_ADDR);
		phase = 1;
		part = 0;
	}
}

void phase_II(){
	//Single Buffer Implementation
	uint8_t id = me();

	//Always read from single buffer addr 1
	if( id == 0 ){
		mram_read8(DSKY_REMOTE_PSTOP_ADDR_1,param[id]);
		g_ps = MIN(param[id][0],g_ps);//Load global g_ps
		mram_read8(VAR_0_ADDR,param[id]);//Host signal to skip to next partitition
		step = param[id][0];
		if ( step  == 1 ){
			part++;
			param[id][0] = 0;
			mram_write8(param[id],VAR_0_ADDR);
		}
	}
	barrier_wait(id);

	int32_t j = 0;
	if(D == 4){ for( j = part + 1 ; j < P ; j++ ) cmp_part_4d(id,P,j); }
	if(D == 8){ for( j = part + 1 ; j < P ; j++ ) cmp_part_8d(id,P,j); }
	if(D == 16){ for( j = part + 1 ; j < P ; j++ ) cmp_part_16d(id,P,j); }

	barrier_wait(id);
	param[id][0] = g_ps;
	mram_write8(param[id],DSKY_PSTOP_ADDR);
}

/*void dbuffer_phase_II(){
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
}*/

void multi_dpu(){
	uint8_t id = me();
	if(phase == 0) phase_I();
	else phase_II();
}

#endif
