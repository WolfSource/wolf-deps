#include <cstdlib>
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libpostproc/postprocess.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}  // extern "C"

static_assert(LIBAVCODEC_VERSION_MAJOR == 59);
static_assert(LIBAVDEVICE_VERSION_MAJOR == 59);
static_assert(LIBAVFILTER_VERSION_MAJOR == 8);
static_assert(LIBAVFORMAT_VERSION_MAJOR == 59);
static_assert(LIBAVUTIL_VERSION_MAJOR == 57);
static_assert(LIBPOSTPROC_VERSION_MAJOR == 56);
static_assert(LIBSWRESAMPLE_VERSION_MAJOR == 4);
static_assert(LIBSWSCALE_VERSION_MAJOR == 6);

void assert(bool p_evaluation, const char* p_message)
{
    if (!p_evaluation) {
        std::cerr << p_message << std::endl;
        std::abort();
    }
}

#define ASSERT_VERBOSE(expr) (assert((expr), "assertion '" #expr "' failed."))

int main()
{
    // to ensure compiler will link to the library file,
    // specially if shared library.
    ASSERT_VERBOSE(avcodec_version() == LIBAVCODEC_VERSION_INT);
    ASSERT_VERBOSE(avdevice_version() == LIBAVDEVICE_VERSION_INT);
    ASSERT_VERBOSE(avfilter_version() == LIBAVFILTER_VERSION_INT);
    ASSERT_VERBOSE(avformat_version() == LIBAVFORMAT_VERSION_INT);
    ASSERT_VERBOSE(avutil_version() == LIBAVUTIL_VERSION_INT);
    ASSERT_VERBOSE(postproc_version() == LIBPOSTPROC_VERSION_INT);
    ASSERT_VERBOSE(swresample_version() == LIBSWRESAMPLE_VERSION_INT);
    ASSERT_VERBOSE(swscale_version() == LIBSWSCALE_VERSION_INT);

    auto avchannel_ptr = av_malloc(sizeof(AVChannel));
    ASSERT_VERBOSE(avchannel_ptr);
    av_free(avchannel_ptr);

    return 0;
}
