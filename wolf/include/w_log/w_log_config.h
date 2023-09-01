/*
	Project			 : Wolf Engine. Copyright(c) Pooya Eimandar (https://PooyaEimandar.github.io) . All rights reserved.
	Source			 : Please direct any bug to https://github.com/WolfEngine/Wolf.Engine/issues
	Website			 : https://WolfEngine.App
    Name			 : w_log_config.h
    Description		 : structures of logger's configuration
    Comment          :
*/

#pragma once

typedef enum w_log_type
{
	W_LOG_DEBUG = 0,
    W_LOG_INFO,
    W_LOG_WARNING,
    W_LOG_ERROR
} w_log_type;

typedef struct w_log_config
{
	const char*	        app_name;
	char*	            log_dir_path;
	w_log_type			flush_level;
	bool		        log_to_std_out;
} w_log_config;


