#include "w_lz4.hpp"

#ifdef WOLF_SYSTEM_LZ4

// NOLINTBEGIN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#include <lz4.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
// NOLINTEND

using w_lz4 = wolf::system::compression::w_lz4;

namespace {
auto s_check_input_len(_In_ const size_t p_src_size) noexcept
    -> boost::leaf::result<int> {
  if (p_src_size < LZ4_MAX_INPUT_SIZE) {
    return 0;
  }

  return W_FAILURE(
      std::errc::invalid_argument,
      wolf::format("source size is greater than LZ4_MAX_INPUT_SIZE: {}",
                   LZ4_MAX_INPUT_SIZE));
}

auto s_shrink_to_fit(_In_ const std::vector<std::byte> &p_src,
                     _In_ const size_t p_shrink_size) noexcept
    -> std::vector<std::byte> {
  std::vector<std::byte> dst;
  dst.resize(p_shrink_size);

  std::copy(p_src.begin(), p_src.begin() + gsl::narrow_cast<int>(p_shrink_size),
            dst.begin());
  return dst;
}
}  // namespace

auto w_lz4::get_compress_bound(_In_ int p_size) noexcept -> int {
  return LZ4_compressBound(p_size);
}

auto w_lz4::compress_default(_In_ gsl::span<const std::byte> p_src) noexcept
    -> boost::leaf::result<std::vector<std::byte>> {
  try {
    const auto src_size = p_src.size();
    if (src_size == 0) {
      return W_FAILURE(std::errc::invalid_argument, "the source is empty");
    }

    BOOST_LEAF_CHECK(s_check_input_len(src_size));

    const auto dst_capacity =
        LZ4_compressBound(gsl::narrow_cast<int>(src_size));
    std::vector<std::byte> tmp;
    tmp.resize(dst_capacity);

    // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
    const auto _bytes =
        LZ4_compress_default(reinterpret_cast<const char *>(p_src.data()),
                             reinterpret_cast<char *>(tmp.data()),
                             gsl::narrow_cast<int>(src_size), dst_capacity);
    // NOLINTEND
    if (_bytes > 0) {
      return s_shrink_to_fit(tmp, _bytes);
    }

    return W_FAILURE(std::errc::operation_canceled,
                     "lz4 compress 'default mode' was failed");
  } catch (const std::exception &e) {
    return W_FAILURE(std::errc::invalid_argument,
                     wolf::format("lz4 compress 'default mode' was failed. An "
                                  "exception just happened: {} ",
                                  e.what()));
  }
}

auto w_lz4::compress_fast(_In_ gsl::span<const std::byte> p_src,
                          _In_ int p_acceleration) noexcept
    -> boost::leaf::result<std::vector<std::byte>> {
  try {
    const auto src_size = p_src.size();
    if (src_size == 0) {
      return W_FAILURE(std::errc::invalid_argument, "the source is empty");
    }

    BOOST_LEAF_CHECK(s_check_input_len(src_size));

    const auto _dst_capacity =
        LZ4_compressBound(gsl::narrow_cast<int>(src_size));
    std::vector<std::byte> tmp;
    tmp.resize(_dst_capacity);

    // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
    const auto bytes = LZ4_compress_fast(
        reinterpret_cast<const char *>(p_src.data()),
        reinterpret_cast<char *>(tmp.data()), gsl::narrow_cast<int>(src_size),
        _dst_capacity, p_acceleration);
    // NOLINTEND

    if (bytes > 0) {
      return s_shrink_to_fit(tmp, bytes);
    }

    return W_FAILURE(std::errc::operation_canceled,
                     "lz4 compress fast mode was failed");
  } catch (const std::exception &e) {
    return W_FAILURE(std::errc::invalid_argument,
                     wolf::format("lz4 compress 'fast mode' was failed. "
                                  "An exception just happened: {}",
                                  e.what()));
  }
}

auto w_lz4::decompress(_In_ gsl::span<const std::byte> p_src,
                       _In_ size_t p_max_retry) noexcept
    -> boost::leaf::result<std::vector<std::byte>> {
  const auto src_size = p_src.size();
  if (src_size == 0) {
    return W_FAILURE(std::errc::invalid_argument, "the source is empty");
  }

  // we will increase our size per each step
  std::vector<std::byte> tmp;
  auto resize = src_size * 2;

#ifdef __clang
#pragma unroll
#endif
  for (auto i = 0; i < p_max_retry; ++i) {
    // resize it for next round
    tmp.resize(resize);

    // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
    const auto _bytes = LZ4_decompress_safe(
        reinterpret_cast<const char *>(p_src.data()),
        reinterpret_cast<char *>(tmp.data()), gsl::narrow_cast<int>(src_size),
        gsl::narrow_cast<int>(tmp.size()));
    // NOLINTEND

    if (_bytes > 0) {
      return s_shrink_to_fit(tmp, _bytes);
    }
    resize *= 2;
  }
  return W_FAILURE(
      std::errc::operation_canceled,
      wolf::format("could not decompress lz4 stream after {}", p_max_retry));
}

#endif  // WOLF_SYSTEM_LZ4
