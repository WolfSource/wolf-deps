/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#ifdef WOLF_SYSTEM_LZ4

#include <cstddef>
#include <vector>
#include <wolf/wolf.hpp>

namespace wolf::system::compression {

struct w_lz4 {
  /*
   * get the size of compress bound
   * @param p_size, the input size
   * @returns the size of bound
   */
  W_API static auto get_compress_bound(_In_ int p_size) noexcept -> int;

  /*
   * compress using the default mode of lz4
   * @param p_src, the input source
   * @returns the vector of compressed stream
   */
  W_API static auto compress_default(
      _In_ gsl::span<const std::byte> p_src) noexcept
      -> boost::leaf::result<std::vector<std::byte>>;

  /*
   * compress using the fast mode of lz4
   * @param p_src, the input source
   * @param p_acceleration, a value between 1 - 65536
   * @returns the vector of compressed stream
   */
  W_API static auto compress_fast(_In_ gsl::span<const std::byte> p_src,
                                  _In_ int p_acceleration) noexcept
      -> boost::leaf::result<std::vector<std::byte>>;

  /*
   * decompress the compressed stream
   * @param p_src, the input source
   * @returns the vector of decompressed stream
   */
  W_API static auto decompress(_In_ gsl::span<const std::byte> p_src,
                               _In_ size_t p_max_retry) noexcept
      -> boost::leaf::result<std::vector<std::byte>>;
};
}  // namespace wolf::system::compression

#endif  // WOLF_SYSTEM_LZ4
