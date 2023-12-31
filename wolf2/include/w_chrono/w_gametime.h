/*
    Project            : Wolf Engine. Copyright(c) Pooya Eimandar (https://PooyaEimandar.github.io) . All rights reserved.
    Source             : Please direct any bug to https://github.com/WolfEngine/Wolf.Engine/issues
    Website            : https://WolfEngine.App
    Name               : w_game_time.h
    Description        : Manage game time
    Comment            :
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "wolf.h"
#include "w_chrono.h"

    //forward declaration
    typedef struct w_gametime_t* w_gametime;
    typedef void w_gametime_tick_callback(w_gametime);

    /**
     * Get elapsed ticks since the previous tick call.
     * @param pMemPool the pool from which to allocate the once flag
     * @param pGameTime pointer to the game time
     * @return pointer to object of w_gametime
    */
    W_SYSTEM_EXPORT
        W_RESULT w_gametime_init(_Inout_ w_mem_pool pMemPool, _Inout_ w_gametime* pGameTime);

    /**
     * reset game time
     */
    W_SYSTEM_EXPORT
        void w_game_time_reset(_Inout_ w_gametime pGameTime);

    /**
     * Get elapsed ticks since the previous tick call.
     * @return elapsed ticks
    */
    W_SYSTEM_EXPORT
        uint64_t w_gametime_get_elapsed_ticks(_In_ w_gametime pGameTime);

    /**
     * Get elapsed seconds since the previous tick call.
     * @return elapsed time
    */
    W_SYSTEM_EXPORT
        double w_gametime_get_elapsed_seconds(_In_ w_gametime pGameTime);

    /**
     * Get total ticks since the start.
     * @return elapsed total ticks
    */
    W_SYSTEM_EXPORT
        uint64_t w_gametime_get_total_ticks(_In_ w_gametime pGameTime);

    /**
     * Get total seconds since the start.
     * @return elapsed total seconds
    */
    W_SYSTEM_EXPORT
        double w_gametime_get_total_seconds(_In_ w_gametime pGameTime);

    /**
     * Get total frame count(s).
     * @return number of frames
    */
    W_SYSTEM_EXPORT
        uint32_t w_gametime_get_frame_count(_In_ w_gametime pGameTime);

    /**
     * Get the current number of framerate.
     * @return framerate
    */
    W_SYSTEM_EXPORT
        uint32_t w_gametime_get_frames_per_second(_In_ w_gametime pGameTime);

    /**
     * Get the fixed time step.
     * @return is fixed time step
    */
    W_SYSTEM_EXPORT
        bool w_gametime_get_fixed_time_step(_In_ w_gametime pGameTime);

    /**
     * Set fixed time step.
    */
    W_SYSTEM_EXPORT
        void w_gametime_enable_fixed_time_step(_In_ w_gametime pGameTime);

    /**
     * Set fixed time step.
    */
    W_SYSTEM_EXPORT
        void w_gametime_disable_fixed_time_step(_In_ w_gametime pGameTime);

    /**
     * Set how often to call tick callback during fixed timestep mode.
    */
    W_SYSTEM_EXPORT
        void w_gametime_set_target_elapsed_ticks(_In_ w_gametime pGameTime, _In_ uint64_t pValue);

    /**
     * Set how often to call tick callback during fixed timestep mode.
    */
    W_SYSTEM_EXPORT
        void w_gametime_set_target_elapsed_seconds(_In_ w_gametime pGameTime, _In_ double pValue);

    /**
     * tick function
     */
    W_SYSTEM_EXPORT
        void w_gametime_tick(_In_ w_gametime pGameTime, _In_opt_ w_gametime_tick_callback pOnTickCallBack);

#ifdef __cplusplus
}
#endif