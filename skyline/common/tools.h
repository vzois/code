
#ifndef TOOLS_H
#define TOOLS_H

#include<meetpoint.h>
#include"../common/config.h"

#define SUM(x,y) (x+y)

void barrier_wait(uint8_t id){
	meetpoint_t mtp = meetpoint_get(0);
	if(id < 1){
		meetpoint_notify(mtp);
	}else{
		meetpoint_subscribe(mtp);
	}
}

void reduce_min(uint8_t id, uint32_t *vec){
	barrier_wait(id);
	if(id < 8) vec[id] = MIN(vec[id],vec[id+8]);
	barrier_wait(id);
	if(id < 4) vec[id] = MIN(vec[id],vec[id+4]);
	barrier_wait(id);
	if(id < 2) vec[id] = MIN(vec[id],vec[id+2]);
	barrier_wait(id);
	if(id < 1) vec[id] = MIN(vec[id],vec[id+1]);
	barrier_wait(id);
}

void reduce_sum(uint8_t id, uint32_t *vec){
	barrier_wait(id);
	if(id < 8) vec[id] = SUM(vec[id],vec[id+8]);
	barrier_wait(id);
	if(id < 4) vec[id] = SUM(vec[id],vec[id+4]);
	barrier_wait(id);
	if(id < 2) vec[id] = SUM(vec[id],vec[id+2]);
	barrier_wait(id);
	if(id < 1) vec[id] = SUM(vec[id],vec[id+1]);
	barrier_wait(id);
}

void reduce(uint8_t id,uint32_t *vec){

	//if( id & 0x1  ) vec[id] = (t_ps[i] & POINTS_PER_T_MSK) | (t_ps[i+1] << POINTS_PER_T);

}

uint32_t max_vec4(uint32_t *vec){
	uint32_t mx0 = MAX(vec[0],vec[1]);
	uint32_t mx1 = MAX(vec[2],vec[3]);
	return MAX(mx0,mx1);
}

uint32_t max_vec8(uint32_t *vec){
	uint32_t mx0 = MAX(vec[0],vec[1]);
	uint32_t mx1 = MAX(vec[2],vec[3]);
	uint32_t mx2 = MAX(vec[4],vec[5]);
	uint32_t mx3 = MAX(vec[6],vec[7]);
	mx0 = MAX(mx0,mx1);
	mx2 = MAX(mx2,mx3);
	return MAX(mx0,mx2);
}

uint32_t max_vec16(uint32_t *vec){
	uint32_t mx0 = MAX(vec[0],vec[1]);
	uint32_t mx1 = MAX(vec[2],vec[3]);
	uint32_t mx2 = MAX(vec[4],vec[5]);
	uint32_t mx3 = MAX(vec[6],vec[7]);
	uint32_t mx4 = MAX(vec[8],vec[9]);
	uint32_t mx5 = MAX(vec[10],vec[11]);
	uint32_t mx6 = MAX(vec[12],vec[13]);
	uint32_t mx7 = MAX(vec[14],vec[15]);
	mx0 = MAX(mx0,mx1);
	mx2 = MAX(mx2,mx3);
	mx4 = MAX(mx4,mx5);
	mx6 = MAX(mx6,mx7);

	mx0 = MAX(mx0,mx2);
	mx4 = MAX(mx4,mx6);

	return MAX(mx0,mx4);
}

void popC(uint32_t v, uint32_t *r){
    uint32_t c = v - ((v >> 1) & 0x55555555);
    c = ((c >> 2) & 0x33333333) + (c & 0x33333333);
    c = ((c >> 4) + c) & 0x0F0F0F0F;
    c = ((c >> 8) + c) & 0x00FF00FF;
    c = ((c >> 16) + c) & 0x0000FFFF;
    r[0]+=c;
}

#endif
