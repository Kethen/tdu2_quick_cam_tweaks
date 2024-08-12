#include <pthread.h>
#include <unistd.h>

#include "logging.h"
#include "config.h"

void *config_read_routine(void *args){
	while(true){
		sleep(2);
		parse_config();
	}
}

__attribute__((constructor))
void init(){
	init_logging();
	init_config();
	parse_config();
	pthread_t config_read_thread;
	pthread_create(&config_read_thread, NULL, config_read_routine, NULL);
}
