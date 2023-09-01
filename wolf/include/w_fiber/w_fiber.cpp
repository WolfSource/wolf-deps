#include "w_fiber.h"

#ifdef WOLF_ENABLE_FIBER

#include <apr_general.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/fiber/all.hpp>
#include <boost/lexical_cast.hpp>
#include "w_log.h"

using namespace boost::fibers;

typedef struct w_fiber_t
{
    long long   id = 0;
    fiber*      fi = nullptr;
} w_fiber_t;

W_RESULT w_fiber_init(
    _In_        w_mem_pool pMemPool,
    _Inout_     w_fiber* pFiber,
    _In_        w_fiber_job pJob,
    _In_opt_    void* pArg)
{
    const char* _trace_info = "w_fiber_init";
    
    *pFiber = NULL;

    if (!pMemPool)
    {
        W_ASSERT_P(false, "memory pool is invalid! trace info %s", _trace_info);
        return APR_BADARG;
    }

    auto _f = (w_fiber)w_malloc(pMemPool, sizeof(w_fiber_t));
    if (!_f)
    {
        W_ASSERT_P(false, "could not allocate memory for w_fiber! trace info %s", _trace_info);
        return W_FAILURE;
    }

    _f->fi = new (std::nothrow) fiber(pJob, pArg);
    if (!_f->fi)
    {
        W_ASSERT_P(false, "could not allocate memory for fiber object! trace info %s", _trace_info);
        return W_FAILURE;
    }

    _f->id = -1;
    auto _id_str = boost::lexical_cast<std::string>(_f->fi->get_id());
    int _ret = sscanf(_id_str.c_str(), "%llu", &_f->id);
    if (_ret == EOF)
    {
        W_ASSERT_P(false, "could not get fiber id! trace info %s", _trace_info);
    }
   
    *pFiber = _f;

    return W_SUCCESS;
}

W_RESULT w_fiber_is_joinable(_In_ w_fiber pFiber)
{
    const char* _trace_info = "w_fiber_is_joinable";

    if (!pFiber || !pFiber->fi)
    {
        W_ASSERT_P(false, 
            "invalid parameters! trace info %s",
            _trace_info);
        return APR_BADARG;
    }
    
    return pFiber->fi->joinable() ? W_SUCCESS : W_FAILURE;
}

W_RESULT w_fiber_join(_In_ w_fiber pFiber)
{
    const char* _trace_info = "w_fiber_join";

    if (!pFiber || !pFiber->fi)
    {
        W_ASSERT_P(false,
            "invalid parameters! trace info %s",
            _trace_info);
        return APR_BADARG;
    }

    pFiber->fi->join();

    return W_SUCCESS;
}

W_RESULT w_fiber_detach(_In_ w_fiber pFiber)
{
    const char* _trace_info = "w_fiber_detach";

    if (!pFiber || !pFiber->fi)
    {
        W_ASSERT_P(false,
            "invalid parameters! trace info %s",
            _trace_info);
        return APR_BADARG;
    }

    pFiber->fi->detach();

    return W_SUCCESS;
}

W_RESULT w_fiber_swap(_Inout_ w_fiber pFiber1, _Inout_ w_fiber pFiber2)
{
    const char* _trace_info = "w_fiber_detach";

    if (!pFiber1 || !pFiber1->fi ||
        !pFiber2 || !pFiber2->fi)
    {
        W_ASSERT_P(false,
            "invalid parameters! trace info %s",
            _trace_info);
        return APR_BADARG;
    }

    pFiber1->fi->swap(*pFiber2->fi);

    return W_SUCCESS;
}

void w_fiber_current_yield()
{
    boost::this_fiber::yield();
}

W_RESULT w_fiber_fini(_Inout_ w_fiber* pFiber)
{
    const char* _trace_info = "w_fiber_detach";

    if (!pFiber || !*pFiber)
    {
        W_ASSERT_P(false,
            "invalid parameters! trace info %s",
            _trace_info);
        return APR_BADARG;
    }

    if ((*pFiber)->fi)
    {
        if ((*pFiber)->fi->joinable())
        {
            (*pFiber)->fi->join();
        }
        W_SAFE_DELETE((*pFiber)->fi);
    }

    *pFiber = NULL;

    return W_SUCCESS;
}


#endif