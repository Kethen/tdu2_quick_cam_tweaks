#include <pthread.h>
#include <unistd.h>

#include "logging.h"
#include "config.h"
#include "hooking.h"

void *config_read_routine(void *args){
	while(true){
		sleep(2);
		parse_config();
	}

	return NULL;
}

void *delayed_hooking_routine(void *args){
	// it looks like 00ca2130 was changed during dinput8 load, then it reverted later
	sleep(2);
	if(init_hooking() != 0){
		LOG("hooking failed, aborting :(\n");
		exit(1);
	}

	return NULL;
}

__attribute__((constructor))
void init(){
	init_logging();

	init_config();
	parse_config();

	pthread_t config_read_thread;
	pthread_create(&config_read_thread, NULL, config_read_routine, NULL);

	pthread_t delayed_hooking_thread;
	pthread_create(&delayed_hooking_thread, NULL, delayed_hooking_routine, NULL);
}
