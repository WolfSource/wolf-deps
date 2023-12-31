/*
    Project: Wolf Engine. Copyright © 2014-2023 Pooya Eimandar
    https://github.com/WolfSource/wolf
*/

#pragma once

#ifdef WOLF_SYSTEM_LZMA

#include <cstddef>
#include <vector>
#include <wolf/wolf.hpp>

namespace wolf::system::compression {

struct w_lzma {
  /*
   * compress a stream via lzma1 algorithm
   * @param p_src, the input source
   * @param p_level, the level of compression
   * @returns a vector of compressed stream
   */
  W_API static auto compress_lzma1(_In_ gsl::span<const std::byte> p_src,
                                   _In_ int p_level)
      -> boost::leaf::result<std::vector<std::byte>>;

  /*
   * compress a stream via lzma2 algorithm
   * @param p_src, the input source
   * @param p_level, the level of compression
   * @returns a vector of compressed stream
   */
  W_API static auto compress_lzma2(_In_ gsl::span<const std::byte> p_src,
                                   _In_ int p_level)
      -> boost::leaf::result<std::vector<std::byte>>;

  /*
   * decompress a stream via lzma1 algorithm
   * @param p_src, the input source
   * @returns a vector of decompressed stream
   */
  W_API static auto decompress_lzma1(_In_ gsl::span<const std::byte> p_src)
      -> boost::leaf::result<std::vector<std::byte>>;

  /*
   * decompress a stream via lzma2 algorithm
   * @param p_src, the input source
   * @returns a vector of decompressed stream
   */
  W_API static auto decompress_lzma2(_In_ gsl::span<const std::byte> p_src)
      -> boost::leaf::result<std::vector<std::byte>>;
};
}  // namespace wolf::system::compression

#endif  // WOLF_SYSTEM_LZMA