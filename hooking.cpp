#include <stdint.h>
#include <string.h>

#include <fcntl.h>

#include <MinHook.h>

#include "config.h"
#include "logging.h"

struct camera{
	int8_t fov_min;
	int8_t fov_max;
	char steer_look_byte[0x4];
	char head_move_array[0x30];
	char rev_vibration_array[0x4];
	struct coordinates_3d pos;
	struct coordinates_3d ori_pos;
};

void *f00ca2130_ref = (void *)0x00ca2130;
uint32_t (__attribute__ ((stdcall)) *f00ca2130_orig)(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4);
uint32_t __attribute__ ((stdcall)) f00ca2130_patched(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4){
	LOG_VERBOSE("%s: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", __func__, param_1, param_2, param_3, param_4);

	uint32_t ret = f00ca2130_orig(param_1, param_2, param_3, param_4);

	uint32_t cptr = *(uint32_t *)(param_1 + 0x420);
	int32_t view_id = *(int32_t *)(cptr + 0x2e8);
	// 20 chase near
	// 21 chase far
	// 22 bumper
	// 23 cockpit
	// 24 hood
	LOG_VERBOSE("%s: cptr is 0x%08x, view_id is %d\n", __func__, cptr, view_id);

	#if 0
	if(view_id >= 20 && view_id <=24){
		char path_buf[64];
		sprintf(path_buf, "./camera_mem_dump_%d_%08x.bin", view_id, cptr);
		int dump_fd = open(path_buf, O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, 00644);
		if(dump_fd >= 0){
			LOG("%s: writing camera memory to %s\n", __func__, path_buf);
			write_data_to_fd(dump_fd, (const char *)cptr, 0x1000);
			close(dump_fd);
		}else{
			LOG("%s: failed dumping camera memory\n", __func__);
		}
	}
	#endif

	static uint32_t last_camera_location = 0;
	static struct camera orig_camera = {0};

	if(last_camera_location != 0){
		// restore last camera
		*(int8_t *)(last_camera_location + 0x5e4) = orig_camera.fov_min;
		*(int8_t *)(last_camera_location + 0x5e4 + 1) = orig_camera.fov_max;
		memcpy((void *)(last_camera_location + 0x300), (void *)orig_camera.steer_look_byte, 4);
		memcpy((void *)(last_camera_location + 0x5a0), (void *)orig_camera.head_move_array, 0x30);
		memcpy((void *)(last_camera_location + 0x5ec), (void *)orig_camera.rev_vibration_array, 4);
		*(float *)(last_camera_location + 0x540) = orig_camera.pos.x;
		*(float *)(last_camera_location + 0x544) = orig_camera.pos.y;
		*(float *)(last_camera_location + 0x548) = orig_camera.pos.z;
		*(float *)(last_camera_location + 0x550) = orig_camera.ori_pos.x;
		*(float *)(last_camera_location + 0x554) = orig_camera.ori_pos.y;
		*(float *)(last_camera_location + 0x558) = orig_camera.ori_pos.z;
	}

	// save current camera
	last_camera_location = cptr;
	orig_camera.fov_min = *(int8_t *)(last_camera_location + 0x5e4);
	orig_camera.fov_max = *(int8_t *)(last_camera_location + 0x5e4 + 1);
	memcpy((void *)orig_camera.steer_look_byte, (void *)(last_camera_location + 0x300), 4);
	memcpy((void *)orig_camera.head_move_array, (void *)(last_camera_location + 0x5a0), 0x30);
	memcpy((void *)orig_camera.rev_vibration_array, (void *)(last_camera_location + 0x5ec), 4);
	orig_camera.pos.x = *(float *)(last_camera_location + 0x540);
	orig_camera.pos.y = *(float *)(last_camera_location + 0x544);
	orig_camera.pos.z = *(float *)(last_camera_location + 0x548);
	orig_camera.ori_pos.x = *(float *)(last_camera_location + 0x550);
	orig_camera.ori_pos.y = *(float *)(last_camera_location + 0x554);
	orig_camera.ori_pos.z = *(float *)(last_camera_location + 0x558);

	LOG_VERBOSE("%s: fov_min %d, fov_max %d\n", __func__, orig_camera.fov_min, orig_camera.fov_max);

	// apply camera changes from config
	if(view_id >= 20 || view_id <= 24){
		pthread_mutex_lock(&current_config_mutex);

		if(view_id == 23 || view_id == 24){
			if(!current_config.global_overrides.enable_steer_look){
				uint8_t bytes[] = {9, 0, 4, 0};
				memcpy((void *)(last_camera_location + 0x300), bytes, 4);
			}

			if(!current_config.global_overrides.enable_head_move){
				memset((void *)(last_camera_location + 0x5a0), 0, 0x30);
			}
		}

		if(!current_config.global_overrides.enable_rev_vibration){
			//uint8_t bytes[] = {0x00, 0x96, 0x46, 0x96};
			//uint8_t bytes[] = {0x96, 0x96, 0x96, 0x96};
			uint8_t bytes[] = {0xFF, 0xFF, 0xFF, 0xFF};
			//uint8_t bytes[] = {0x00, 0x00, 0x00, 0x00};
			memcpy((void *)(last_camera_location + 0x5ec), bytes, 4);
		}


		if(current_config.global_overrides.override_fov){
			*(int8_t *)(last_camera_location + 0x5e4) = current_config.global_overrides.fov_min;
			*(int8_t *)(last_camera_location + 0x5e4 + 1) = current_config.global_overrides.fov_max;
		}

		*(float *)(last_camera_location + 0x540) = *(float *)(last_camera_location + 0x540) + current_config.global_pos_offset.x;
		*(float *)(last_camera_location + 0x544) = *(float *)(last_camera_location + 0x544) + current_config.global_pos_offset.y;
		*(float *)(last_camera_location + 0x548) = *(float *)(last_camera_location + 0x548) + current_config.global_pos_offset.z;
		*(float *)(last_camera_location + 0x550) = *(float *)(last_camera_location + 0x550) + current_config.global_pos_offset.x;
		*(float *)(last_camera_location + 0x554) = *(float *)(last_camera_location + 0x554) + current_config.global_pos_offset.y;
		*(float *)(last_camera_location + 0x558) = *(float *)(last_camera_location + 0x558) + current_config.global_pos_offset.z;

		pthread_mutex_unlock(&current_config_mutex);
	}

	return ret;
}


int init_hooking(){
	int ret = 0;

	ret = MH_Initialize();
	if(ret != MH_OK && ret != MH_ERROR_ALREADY_INITIALIZED){
		LOG("Failed initializing MinHook, %d\n", ret);
		return -1;
	}

	ret = MH_CreateHook(f00ca2130_ref, (LPVOID)&f00ca2130_patched, (LPVOID *)&f00ca2130_orig);
	if(ret != MH_OK){
		LOG("Failed creating hook for 0x00ca2130, %d\n", ret);
		for(int i = 0;i < 12; i++){
			LOG("0x%02x ", ((uint8_t *)f00ca2130_ref)[i]);
		}
		LOG("\n");
		return -1;
	}
	ret = MH_EnableHook(f00ca2130_ref);
	if(ret != MH_OK){
		LOG("Failed enableing hook for 0x00ca2130, %d\n", ret);
		return -1;
	}

	return 0;
}
