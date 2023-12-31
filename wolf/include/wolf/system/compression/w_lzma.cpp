#include "w_lzma.hpp"

#ifdef WOLF_SYSTEM_LZMA

// NOLINTBEGIN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#include <Lzma2Dec.h>
#include <Lzma2Enc.h>
#include <LzmaDec.h>
#include <LzmaEnc.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
// NOLINTEND

using w_lzma = wolf::system::compression::w_lzma;

constexpr auto LZMA_HEADER_SRC_SIZE = 8;
constexpr auto MAX_HEADER_SIZE = 256 * 1024 * 1024;

namespace {

// NOLINTBEGIN (cppcoreguidelines-no-malloc)
auto s_lzma_alloc(ISzAllocPtr p_ptr, size_t p_size) noexcept -> void * {
  std::ignore = p_ptr;
  return malloc(p_size);
}
void s_lzma_free(ISzAllocPtr p_ptr, void *p_addr) noexcept {
  std::ignore = p_ptr;
  free(p_addr);
}
// NOLINTEND

constexpr ISzAlloc s_alloc_funcs = {s_lzma_alloc, s_lzma_free};

// NOLINTBEGIN (bugprone-easily-swappable-parameters)
void s_lzma_prop(_Inout_ CLzmaEncProps *p_prop, _In_ int p_level,
                 _In_ int p_src_size) noexcept {
  // setup properties
  LzmaEncProps_Init(p_prop);

  constexpr auto min_dic_size = 1 << 12;
  constexpr auto max_dic_size = 3 << 29;
  constexpr auto max_level = 9;

  constexpr auto fb = 40;

  if (p_src_size <= min_dic_size) {
    p_prop->dictSize = min_dic_size;
  } else if (p_src_size >= max_dic_size) {
    p_prop->dictSize = max_dic_size;
  } else {
    p_prop->dictSize =
        gsl::narrow_cast<uint32_t>(p_src_size);  // smaller dictionary = faster!
  }
  p_prop->fb = fb;

  if (p_level >= max_level) {
    p_level = max_level;
  }
  p_prop->level = p_level;
}
// NOLINTEND
}  // namespace

auto w_lzma::compress_lzma1(_In_ gsl::span<const std::byte> p_src,
                            _In_ int p_level)
    -> boost::leaf::result<std::vector<std::byte>> {
  const auto src_size = gsl::narrow_cast<int>(p_src.size());
  if (src_size == 0) {
    return W_FAILURE(std::errc::invalid_argument, "the source is empty");
  }

  // setup properties
  CLzmaEncProps props = {};
  s_lzma_prop(&props, p_level, src_size);

  // prepare space for the encoded properties
  size_t props_size = LZMA_PROPS_SIZE;
  std::array<std::byte, LZMA_PROPS_SIZE> props_encoded = {};

  // allocate some space for the compression output
  // this is way more than necessary in most cases
  constexpr size_t max = 2024;
  auto output_size_64 = gsl::narrow_cast<size_t>(src_size) * 2;
  if (output_size_64 < max) {
    output_size_64 = max;
  }

  std::vector<std::byte> tmp;
  tmp.resize(output_size_64);

  // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
  const auto _lzma_status =
      LzmaEncode(reinterpret_cast<Byte *>(tmp.data()), &output_size_64,
                 reinterpret_cast<const Byte *>(p_src.data()), p_src.size(),
                 &props, reinterpret_cast<Byte *>(props_encoded.data()),
                 &props_size, 0, nullptr, &s_alloc_funcs, &s_alloc_funcs);
  // NOLINTEND

  const auto _compressed_size =
      output_size_64 + LZMA_HEADER_SRC_SIZE + LZMA_PROPS_SIZE;

  if (_lzma_status == SZ_OK) {
    std::vector<std::byte> dst;
    dst.reserve(_compressed_size);

    // tricky: we have to generate the LZMA header
    // 5 bytes properties + 8 byte uncompressed size
    std::copy(props_encoded.begin(), props_encoded.begin() + LZMA_PROPS_SIZE,
              std::back_inserter(dst));

    constexpr auto uncompressed_byte_size = 8;
    constexpr auto oxff = 0xFF;

    for (int i = 0; i < LZMA_HEADER_SRC_SIZE; i++) {
      dst.push_back(
          std::byte((src_size >> (i * uncompressed_byte_size)) & oxff));
    }

    // copy the compressed size
    std::copy(tmp.begin(), tmp.begin() + gsl::narrow_cast<long>(output_size_64),
              std::back_inserter(dst));

    return dst;
  }
  return W_FAILURE(std::errc::operation_canceled, "lzma1 compress failed");
}

auto w_lzma::compress_lzma2(_In_ const gsl::span<const std::byte> p_src,
                            _In_ int p_level)
    -> boost::leaf::result<std::vector<std::byte>> {
  const auto src_size = gsl::narrow_cast<int>(p_src.size());
  if (src_size == 0) {
    return W_FAILURE(std::errc::invalid_argument, "the source is empty");
  }

  auto *enc_handler = Lzma2Enc_Create(&s_alloc_funcs, &s_alloc_funcs);
  if (enc_handler == nullptr) {
    return W_FAILURE(std::errc::operation_canceled,
                     "failed on creating lzma2 encoder");
  }

  // setup properties
  CLzmaEncProps props_1{};
  s_lzma_prop(&props_1, p_level, src_size);

  CLzma2EncProps props_2{};
  Lzma2EncProps_Init(&props_2);
  props_2.lzmaProps = props_1;

  const auto props_status = Lzma2Enc_SetProps(enc_handler, &props_2);
  if (props_status != SZ_OK) {
    return W_FAILURE(std::errc::operation_canceled,
                     "failed on setting lzma2 encoder properties");
  }

  // prepare space for the encoded properties
  const auto properties = Lzma2Enc_WriteProperties(enc_handler);
  // allocate some space for the compression output
  // this is way more than necessary in most cases.
  constexpr size_t max_size = 1024;
  auto output_size_64 = gsl::narrow_cast<size_t>(src_size) * 2;
  if (output_size_64 < max_size) {
    output_size_64 = max_size;
  }

  std::vector<std::byte> tmp;
  tmp.resize(output_size_64);

  // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
  const auto encode_status = Lzma2Enc_Encode2(
      enc_handler, nullptr, reinterpret_cast<Byte *>(tmp.data()),
      &output_size_64, nullptr, reinterpret_cast<const Byte *>(p_src.data()),
      src_size, nullptr);
  // NOLINTEND

  Lzma2Enc_Destroy(enc_handler);

  const auto compressed_size =
      output_size_64 + LZMA_HEADER_SRC_SIZE + sizeof(properties);

  if (encode_status == SZ_OK) {
    std::vector<std::byte> dst;
    dst.reserve(compressed_size);

    /*
        tricky: we have to generate the LZMA header
        1 byte properties + 8 bytes uncompressed size
    */
    dst.push_back(std::byte(properties));
    constexpr auto uncompressed_byte_size = 8;
    constexpr auto oxff = 0xFF;
    for (int i = 0; i < LZMA_HEADER_SRC_SIZE; i++) {
      dst.push_back(
          std::byte((src_size >> (i * uncompressed_byte_size)) & oxff));
    }

    // copy the compressed size
    std::copy(tmp.begin(), tmp.begin() + gsl::narrow_cast<long>(output_size_64),
              std::back_inserter(dst));

    return dst;
  }
  return W_FAILURE(std::errc::operation_canceled, "lzma2 compress failed");
}

auto w_lzma::decompress_lzma1(_In_ gsl::span<const std::byte> p_src)
    -> boost::leaf::result<std::vector<std::byte>> {
  const auto src_size = p_src.size();

  if (src_size < LZMA_HEADER_SRC_SIZE + LZMA_PROPS_SIZE) {
    return W_FAILURE(std::errc::invalid_argument, "invalid lzma1 header size");
  }

  // extract the size from the header
  uint64_t size_from_header = 0;
  for (uint64_t i = 0; i < LZMA_HEADER_SRC_SIZE; i++) {
    const auto head_1 = gsl::at(p_src, LZMA_PROPS_SIZE + i);
    const auto head_2 = i * 8;
    const auto head_3 = gsl::narrow_cast<uint64_t>(head_1 << head_2);
    if (head_3 < std::numeric_limits<uint64_t>::max()) {
      size_from_header |= head_3;
    }
  }

  std::vector<std::byte> dst;
  if (size_from_header <= MAX_HEADER_SIZE) {
    // allocate memory
    dst.resize(size_from_header);

    ELzmaStatus _lzma_status{};
    auto proc_out_size = size_from_header;
    auto proc_in_size = src_size - LZMA_HEADER_SRC_SIZE + LZMA_PROPS_SIZE;

    // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
    // decode via lzma
    const auto status = LzmaDecode(
        reinterpret_cast<Byte *>(dst.data()), &proc_out_size,
        reinterpret_cast<const Byte *>(
            &gsl::at(p_src, LZMA_HEADER_SRC_SIZE + LZMA_PROPS_SIZE)),
        &proc_in_size, reinterpret_cast<const Byte *>(p_src.data()),
        LZMA_PROPS_SIZE, LZMA_FINISH_END, &_lzma_status, &s_alloc_funcs);
    // NOLINTEND

    // return on success
    if (status == SZ_OK && proc_out_size == size_from_header) {
      return dst;
    }
  }
  return W_FAILURE(std::errc::operation_canceled, "lzma1 decompress failed");
}

auto w_lzma::decompress_lzma2(_In_ gsl::span<const std::byte> p_src)
    -> boost::leaf::result<std::vector<std::byte>> {
  const auto _src_size = p_src.size();

  if (_src_size < LZMA_HEADER_SRC_SIZE + sizeof(Byte)) {
    return W_FAILURE(std::errc::invalid_argument, "invalid lzma2 header size");
  }

  // extract the size from the header
  uint64_t _size_from_header = 0;
  for (uint64_t i = 0; i < LZMA_HEADER_SRC_SIZE; i++) {
    const auto _h1 = gsl::at(p_src, gsl::index(sizeof(Byte) + i));
    const auto _h2 = i * 8;
    const auto _h3 = gsl::narrow_cast<uint64_t>(_h1 << _h2);
    if (_h3 < std::numeric_limits<uint64_t>::max()) {
      _size_from_header |= _h3;
    }
  }

  const auto _pre_out_size = _size_from_header * 2;
  if (_size_from_header <= MAX_HEADER_SIZE) {
    std::vector<std::byte> dst;
    dst.resize(_size_from_header);

    CLzma2Dec dec{};
    Lzma2Dec_Construct(&dec);

    auto res = Lzma2Dec_Allocate(&dec, 0, &s_alloc_funcs);
    if (res != SZ_OK) {
      return W_FAILURE(std::errc::invalid_argument,
                       "could not allocate memory for lzma2 decoder");
    }

    Lzma2Dec_Init(&dec);
    size_t out_pos = 0;
    auto in_pos = LZMA_HEADER_SRC_SIZE + sizeof(std::byte);

    ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
    constexpr uint32_t BUF_SIZE = 10240;

    while (out_pos < _pre_out_size) {
      auto dest_len = std::min(gsl::narrow_cast<uint64_t>(BUF_SIZE),
                               _pre_out_size - out_pos);
      auto src_len = std::min(gsl::narrow_cast<size_t>(BUF_SIZE),
                              _src_size - gsl::narrow_cast<size_t>(in_pos));

      // NOLINTBEGIN (cppcoreguidelines-pro-type-reinterpret-cast)
      res = Lzma2Dec_DecodeToBuf(
          &dec, reinterpret_cast<Byte *>(&gsl::at(dst, out_pos)), &dest_len,
          reinterpret_cast<const Byte *>(&gsl::at(p_src, in_pos)), &src_len,
          (out_pos + dest_len == _size_from_header) ? LZMA_FINISH_END
                                                    : LZMA_FINISH_ANY,
          &status);
      // NOLINTEND

      if (res != SZ_OK) {
        return W_FAILURE(std::errc::invalid_argument,
                         "Lzma2Dec_DecodeToBuf failed");
      }

      in_pos += src_len;
      out_pos += dest_len;
      if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
        break;
      }
    }

    Lzma2Dec_Free(&dec, &s_alloc_funcs);

    if (out_pos == _size_from_header) {
      return dst;
    }
  }

  return W_FAILURE(std::errc::operation_canceled, "lzma2 decompress failed");
}

#endif  // WOLF_SYSTEM_LZMA