#include<meetpoint.h>

void barrier_wait(uint8_t id){
	meetpoint_t mtp = meetpoint_get(0);
	if(id < 1){
		meetpoint_notify(mtp);
	}else{
		meetpoint_subscribe(mtp);
	}
}
