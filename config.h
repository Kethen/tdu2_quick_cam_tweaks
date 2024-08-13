#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdint.h>

#include <pthread.h>

/*
	maybe todo
	per vehicle config
	pitch/yaw
*/

struct overrides{
	bool enable_steer_look;
	bool enable_head_move;
	bool enable_rev_vibration;
	bool enable_offroad_vibration;
	bool override_fov;
	int8_t fov_min;
	int8_t fov_max;
};

struct coordinates_3d{
	float x;
	float y;
	float z;
};

struct config{
	struct overrides global_overrides;
	struct coordinates_3d global_pos_offset;
};

extern struct config current_config;
extern pthread_mutex_t current_config_mutex;

#ifdef __cplusplus
extern "C" {
#endif

void init_config();
void parse_config();

#ifdef __cplusplus
}
#endif

#endif
