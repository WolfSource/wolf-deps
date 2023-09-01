/*
	Project			 : Wolf Engine. Copyright(c) Pooya Eimandar (https://PooyaEimandar.github.io) . All rights reserved.
	Source			 : Please direct any bug to https://github.com/WolfEngine/Wolf.Engine/issues
	Website			 : https://WolfEngine.App
	Name			 : wolf.h
	Description		 : the basic headers for WOLF
	Comment          :
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if (_MSC_VER > 1900) //VS 2017 and the laters versions
#pragma warning(disable : 5105)
#endif

#include <w_memory/w_mem_pool.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>

#define W_SAFE_DELETE(x)            { if (x)  { delete x; x = NULL;                  } }
#define W_SAFE_DELETE_ARRAY(ar)     { if (ar) { delete[] ar; ar = NULL;              } }
#define W_SAFE_RELEASE(x)           { if (x)  { x->release(); delete x; x = NULL;    } }
#define W_ARRAY_SIZE(ar)	        { return  (sizeof(ar) / sizeof(ar[0]); }

#define WOLF_MAJOR_VERSION 2   // Making incompatible API changes
#define WOLF_MINOR_VERSION 1   // Adding functionality in a backwards - compatible manner
#define WOLF_PATCH_VERSION 2   // bug fixes
#define WOLF_DEBUG_VERSION 0   // for debugging

#define W_MAX_BUFFER_SIZE 4096

#define W_EOF    -1
#define W_SUCCESS 0
#define W_FAILURE 1
#define W_TIMEOUT 2
#define W_BAD_ARG 70013//Same as APR_BADARG

#ifdef W_PLATFORM_LINUX
#include <linux/types.h>
#endif

    typedef
#ifdef _WIN64
        __int64
#elif defined _WIN32
        __int32
#elif defined(W_PLATFORM_OSX) || defined(W_PLATFORM_IOS)
        __darwin_off_t
#elif defined W_PLATFORM_ANDROID
        __kernel_off_t
#elif defined W_PLATFORM_LINUX
        __kernel_off_t
#endif
        w_offset;

    typedef struct apr_file_t* w_file;
    typedef struct apr_finfo_t* w_file_info;
    typedef struct w_arg_t
    {
        w_mem_pool pool;
        void* data;
    } w_arg_t;
    typedef struct w_arg_t* w_arg;

    typedef struct w_buffer_t
    {
        uint8_t*    data;
        size_t      len;
    } w_buffer_t;
    typedef w_buffer_t* w_buffer;

    /**
     * initialize wolf
     * @return W_RESULT as a result
    */
    W_SYSTEM_EXPORT
        W_RESULT wolf_init(void);

    /**
     * concatenate two or more char*
     * @param pMemPool The memory pool
     * @note make sure to append NULL
     * @return result chars
    */
    W_SYSTEM_EXPORT
        char* w_strcat(_In_ w_mem_pool pMemPool, ...);

    /**
     * concatenate two or more wchar_t*
     * @param pMemPool The memory pool
     * @note make sure to append NULL
     * @return concated chars
    */
    W_SYSTEM_EXPORT
        wchar_t* w_wstrcat(_In_ w_mem_pool pMemPool, ...);
    
    /**
    * Return a human readable string describing the specified error.
    * @param pStatCode The error code to get a string for
    * @param pBuffer A buffer to hold the error string
    * @param pBufferSize Size of the buffer to hold the string
    * @return a human readable string describing the specified error.
    */
    W_SYSTEM_EXPORT
        char* w_strerror(_In_ W_RESULT pErrorCode, _Inout_z_  char* pBuffer, _In_ size_t pBufferSize);

   /**
    * run main loop of wolf in order to process signals
    * @param pFlags the loop flag
    * <PRE>
    *     0 = RUN_DEFAULT block the current thread and fetch/handle internal events
          1 = RUN_NOWAIT fetch/handle events but don't block at all
          2 = RUN_ONCE	means at most one time. wait for first events and then unblock current thread
    * </PRE>
    * @note this will block current thread
    */
    /*W_SYSTEM_EXPORT
        void wolf_run(_In_ int pFlags);*/

    /**
     * release all resources of wolf
    */
    W_SYSTEM_EXPORT
        void wolf_fini(void);

#ifdef __cplusplus
}
#endif
