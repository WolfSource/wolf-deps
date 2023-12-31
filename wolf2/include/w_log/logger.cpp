#include "logger.hpp"
#include <apr_general.h>
#include <w_io/w_io.h>
#include <w_chrono/w_timespan.h>
#include <time.h>
#include <cstdarg>

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#ifdef _MSC_VER
    #include "spdlog/sinks/msvc_sink.h"
#endif

logger::logger(
    _Inout_ w_mem_pool pMemPool,
    _In_ const w_log_config* pConfig)
{
    if (!pMemPool || !pConfig)
    {
        return;
    }

    auto _log_path =
        w_strcat(
            pMemPool,
            pConfig->log_dir_path,
            "/LOG/",
            NULL);
    //if directory of log is not existed
    if (w_io_dir_check_is_dir(
        pMemPool,
        _log_path) != W_SUCCESS)
    {
        //create the directory of log inside the root directory
        //TODO
        w_io_dir_create(pMemPool, _log_path);
    }

    auto _time = w_timespan_init_from_now(pMemPool);
    auto _time_str = w_timespan_to_string(pMemPool, _time, "_");
    auto _log_file_path_str =
        std::string(_log_path) +
        "/" +
        std::string(_time_str) +
        ".wLog";

    std::vector<spdlog::sink_ptr> _sinks;
#if defined(_MSC_VER) && !defined(MinSizeRel)
    _sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

#ifdef W_PLATFORM_WIN
    //convert it to wstring
    std::wstring _log_file_path_str_w(_log_file_path_str.begin(), _log_file_path_str.end());
    _sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(_log_file_path_str_w, true));
#else
    _sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(_log_file_path_str, true));
#endif
    _sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());

    this->_l = std::make_shared<spdlog::logger>(
        pConfig->app_name,
        begin(_sinks),
        end(_sinks));
    if (!this->_l)
    {
        return;
    }

    this->_l->info(
        "Project: \"Wolf Engine(https://WolfEngine.App). "\
        "Copyright(c) Pooya Eimandar(https://PooyaEimandar.github.io). All rights reserved.\". "\
        "Contact: \"Pooya.Eimandar@gmail.com\" "\
        "Version: {}.{}.{}.{}",
        WOLF_MAJOR_VERSION,
        WOLF_MINOR_VERSION,
        WOLF_PATCH_VERSION,
        WOLF_DEBUG_VERSION);

    auto _level = spdlog::level::level_enum::info;
    switch (pConfig->flush_level)
    {
    default:
    case W_LOG_DEBUG:
    case W_LOG_INFO:
        break;
    case W_LOG_WARNING:
        _level = spdlog::level::level_enum::warn;
        break;
    case W_LOG_ERROR:
        _level = spdlog::level::level_enum::err;
        break;
    }
    this->_l->set_level(_level);
    this->_l->flush_on(_level);
}

W_RESULT  logger::write(_In_z_ const char* pFMT)
{
    if (!this->_l) return W_FAILURE;
    this->_l->info(pFMT);
    return W_SUCCESS;
}

W_RESULT  logger::write(
    _In_ const w_log_type pLogType,
    _In_z_ const char* pFMT)
{
    //get first log
    if (!this->_l) return W_FAILURE;
    switch (pLogType)
    {
    default:
    case W_LOG_DEBUG:
#if defined(DEBUG) || defined(_DEBUG)
        this->_l->info(pFMT);
#endif
        break;
    case W_LOG_INFO:
        this->_l->info(pFMT);
        break;
    case W_LOG_WARNING:
        this->_l->warn(pFMT);
        break;
    case W_LOG_ERROR:
        this->_l->error(pFMT);
        break;
    }
    return W_SUCCESS;
}

W_RESULT  logger::write(_In_ const w_log_type pLogType,
    _In_ const int pLogID,
    _In_z_ const char* pFMT)
{
    if (!this->_l) return W_FAILURE;
    switch (pLogType)
    {
    default:
    case W_LOG_DEBUG:
#if defined(DEBUG) || defined(_DEBUG)
        this->_l->info(pFMT);
#endif
        break;
    case W_LOG_INFO:
        this->_l->info(pFMT);
        break;
    case W_LOG_WARNING:
        this->_l->warn(pFMT);
        break;
    case W_LOG_ERROR:
        this->_l->error(pFMT);
        break;
    }
    return W_SUCCESS;
}

template<typename... w_args>
W_RESULT logger::write(_In_ const w_log_type pLogType,
    _In_z_ const char* pFMT,
    _In_ const w_args&... pArgs)
{
    //get first log
    if (!this->_l) return W_FAILURE;
    switch (pLogType)
    {
    default:
    case W_LOG_DEBUG:
#if defined(DEBUG) || defined(_DEBUG)
        this->_l->info(pFMT);
#endif
        break;
    case W_LOG_INFO:
        this->_l->info(pFMT, pArgs...);
        break;
    case W_LOG_WARNING:
        this->_l->warn(pFMT, pArgs...);
        break;
    case W_LOG_ERROR:
        this->_l->error(pFMT, pArgs...);
        break;
    }
    return W_SUCCESS;
}

template<typename... w_args>
W_RESULT logger::write(_In_ const w_log_type pLogType,
    _In_ const int pLogID,
    _In_z_ const char* pFMT,
    _In_ const w_args&... pArgs)
{
    if (!this->_l) return W_FAILURE;
    switch (pLogType)
    {
    default:
    case W_LOG_DEBUG:
#if defined(DEBUG) || defined(_DEBUG)
        this->_l->info(pFMT);
#endif
        break;
    case W_LOG_INFO:
        this->_l->info(pFMT, pArgs...);
        break;
    case W_LOG_WARNING:
        this->_l->warn(pFMT, pArgs...);
        break;
    case W_LOG_ERROR:
        this->_l->error(pFMT, pArgs...);
        break;
    }
    return W_SUCCESS;
}

W_RESULT logger::flush(void)
{
    if (!this->_l) return W_FAILURE;
    this->_l->flush();
    return W_SUCCESS;
}

W_RESULT logger::flush(_In_ int pLogID)
{
    if (!this->_l) return W_FAILURE;
    this->_l->flush();
    return W_SUCCESS;
}

