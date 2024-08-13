#include <fcntl.h>

#include <json.hpp>

#include <pthread.h>

#include "logging.h"

#include "config.h"

#define STR(s) #s

using json = nlohmann::json;

struct config current_config = {0};
pthread_mutex_t current_config_mutex;

extern "C" {
	void init_config(){
		pthread_mutex_init(&current_config_mutex, NULL);
	}

	static void log_config(struct config *c){
		LOG("global overrides:\n");
		#define LOG_GLOBAL_OVERRIDES_BOOL(k){ \
			LOG("%s: %s\n", STR(k), c->global_overrides.k? "true": "false"); \
		}
		#define LOG_GLOBAL_OVERRIDES_FLOAT(k){ \
			LOG("%s: %f\n", STR(k), c->global_overrides.k); \
		}
		#define LOG_GLOBAL_OVERRIDES_INT8(k){ \
			LOG("%s: %d\n", STR(k), c->global_overrides.k); \
		}
		LOG_GLOBAL_OVERRIDES_BOOL(enable_steer_look);
		LOG_GLOBAL_OVERRIDES_BOOL(enable_head_move);
		LOG_GLOBAL_OVERRIDES_BOOL(enable_rev_vibration);
		LOG_GLOBAL_OVERRIDES_BOOL(override_fov);
		LOG_GLOBAL_OVERRIDES_INT8(fov_min);
		LOG_GLOBAL_OVERRIDES_INT8(fov_max);
		#undef LOG_GLOBAL_OVERRIDES_BOOL
		#undef LOG_GLOBAL_OVERRIDES_FLOAT
		#undef LOG_GLOBAL_OVERRIDES_INT8

		#define LOG_GLOBAL_POS_OFFSET_FLOAT(k){ \
			LOG("%s: %f\n", STR(k), c->global_pos_offset.k); \
		}
		LOG_GLOBAL_POS_OFFSET_FLOAT(x);
		LOG_GLOBAL_POS_OFFSET_FLOAT(y);
		LOG_GLOBAL_POS_OFFSET_FLOAT(z);
		#undef LOG_GLOBAL_POS_OFFSET_FLOAT
	}

	void parse_config(){
		const char *config_path = "./tdu2_quick_cam_tweaks_config.json";
		json config;
		bool changed = false;
		char *read_buf = NULL;
		int file_size = 0;
		struct config new_config = {0};
		int read_fd = -1;
		int write_fd = -1;

		read_fd = open(config_path, O_RDONLY | O_BINARY);
		if(read_fd < 0){
			LOG("failed opening %s for reading\n", config_path);
			changed = true;
		}

		if(read_fd >= 0){
			file_size = lseek(read_fd, 0, SEEK_END);
			read_buf = (char *)malloc(file_size);
			lseek(read_fd, 0, SEEK_SET);
		}

		if(read_buf != NULL){
			int res = read_data_from_fd(read_fd, read_buf, file_size);
			if(res != file_size){
				LOG("failed reading %s\n", config_path);
				changed = true;
			}
		}else{
			LOG("failed allocating memory for reading %s\n", config_path);
			changed = true;
		}

		if(!changed){
			try{
				config = json::parse(std::string(read_buf, file_size));
			}catch(...){
				LOG("failed parsing %s\n", config_path);
				changed = true;
			}
			free(read_buf);
		}

		if(read_fd >= 0){
			close(read_fd);
		}

		#define READ_GLOBAL_OVERRIDE(k, d){ \
			try{ \
				new_config.global_overrides.k = config.at("global_overrides").at(STR(k)); \
			}catch(...){ \
				LOG("cannot fetch global override %s from json, using default %s\n", STR(k), STR(d)); \
				new_config.global_overrides.k = d; \
				config["global_overrides"][STR(k)] = d; \
				changed = true; \
			} \
		}
		READ_GLOBAL_OVERRIDE(enable_steer_look, true);
		READ_GLOBAL_OVERRIDE(enable_head_move, true);
		READ_GLOBAL_OVERRIDE(enable_rev_vibration, true);
		READ_GLOBAL_OVERRIDE(override_fov, false);
		READ_GLOBAL_OVERRIDE(fov_min, 60);
		READ_GLOBAL_OVERRIDE(fov_max, 60);
		#undef READ_GLOBAL_OVERRIDE

		#define READ_GLOBAL_POS_OFFSET(k, d){ \
			try { \
				new_config.global_pos_offset.k = config.at("global_pos_offset").at(STR(k)); \
			}catch(...){ \
				LOG("cannot fetch global position offset %s from json, unsing default %s\n", STR(k), STR(d)); \
				new_config.global_pos_offset.k = d; \
				config["global_pos_offset"][STR(k)] = d; \
				changed = true; \
			} \
		}
		READ_GLOBAL_POS_OFFSET(x, 0.0);
		READ_GLOBAL_POS_OFFSET(y, 0.0);
		READ_GLOBAL_POS_OFFSET(z, 0.0);
		#undef READ_GLOBAL_POS_OFFSET

		if(changed){
			LOG("config updated, writing to %s\n", config_path);
			write_fd = open(config_path, O_WRONLY | O_CREAT | O_TRUNC, 00644);
			if(write_fd >= 0){
				std::string config_string = config.dump(4);
				if(write_data_to_fd(write_fd, config_string.c_str(), config_string.size()) != config_string.size()){
					LOG("failed writing %s\n", config_path);
				}
			}else{
				LOG("failed opening %s for writing\n", config_path);
			}
		}

		if(memcmp(&current_config, &new_config, sizeof(struct config)) != 0){
			LOG("applying new config:\n");
			log_config(&new_config);
			pthread_mutex_lock(&current_config_mutex);
			memcpy(&current_config, &new_config, sizeof(struct config));
			pthread_mutex_unlock(&current_config_mutex);
		}
	}
}
