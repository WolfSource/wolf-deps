#pragma once

#include <msquic.h>

#include <array>
#include <memory>
#include <span>
#include <string>
#include <variant>

#include "stream/quic/internal/common.hpp"
#include "system/w_overloaded.hpp"

namespace wolf::stream::quic {

/**
 * @brief An empty tag type indicating no certificate.
 */
class W_API w_certificate_none { /* nothing. tag type. */
};

/**
 * @brief Wrapper of QUIC_CERTIFICATE_HASH.
 *
 * Holds sha1 hash binary digest (20 bytes).
 */
class W_API w_certificate_hash {
  friend class internal::w_raw_access;

 public:
  constexpr static auto hash_size = 20;

  /**
   * @brief construct certificate with given sha1 bytes/digest.
   * @param p_hash sha1 bytes/digest.
   */
  explicit w_certificate_hash(std::span<const uint8_t, hash_size> p_hash) {
    std::memcpy(_cert.ShaHash, p_hash.data(), hash_size);
  }

  w_certificate_hash(const w_certificate_hash& p_other) noexcept
      : w_certificate_hash(p_other._cert.ShaHash) {}

  w_certificate_hash& operator=(const w_certificate_hash& p_other) noexcept {
    std::memcpy(_cert.ShaHash, p_other._cert.ShaHash, hash_size);
    return *this;
  }

  ~w_certificate_hash() { /* nothing */
  }

 private:
  auto raw() noexcept { return &_cert; }
  auto raw() const noexcept { return &_cert; }

  QUIC_CERTIFICATE_HASH _cert;
};

/**
 * @brief Wrapper of QUIC_CERTIFICATE_FILE.
 *
 * Holding file paths of certificate and key,
 * which are loaded when loading credentials into
 * configuration handle.
 */
class w_certificate_file {
  friend class internal::w_raw_access;

 public:
  w_certificate_file(std::string p_cert_file_path, std::string p_key_file_path)
      : _cert_file_path(std::move(p_cert_file_path)),
        _key_file_path(std::move(p_key_file_path)) {
    _cert.CertificateFile = _cert_file_path.c_str();
    _cert.PrivateKeyFile = _key_file_path.c_str();
  }

  w_certificate_file(const w_certificate_file& p_other)
      : w_certificate_file(p_other._cert_file_path, p_other._key_file_path) {}

  w_certificate_file(w_certificate_file&& p_other) noexcept
      : w_certificate_file(std::move(p_other._cert_file_path),
                           std::move(p_other._key_file_path)) {}

  w_certificate_file& operator=(const w_certificate_file& p_other) {
    _cert_file_path = p_other._cert_file_path;
    _key_file_path = p_other._key_file_path;
    _cert.CertificateFile = _cert_file_path.c_str();
    _cert.PrivateKeyFile = _key_file_path.c_str();
    return *this;
  }

  w_certificate_file& operator=(w_certificate_file&& p_other) noexcept {
    _cert_file_path = std::move(p_other._cert_file_path);
    _key_file_path = std::move(p_other._key_file_path);
    _cert.CertificateFile = _cert_file_path.c_str();
    _cert.PrivateKeyFile = _key_file_path.c_str();
    return *this;
  }

  ~w_certificate_file() { /* nothing */
  }

 private:
  auto raw() noexcept { return &_cert; }
  auto raw() const noexcept { return &_cert; }

  std::string _cert_file_path;
  std::string _key_file_path;

  QUIC_CERTIFICATE_FILE _cert;
};

/**
 * @brief Wrapper of QUIC_CREDENTIAL_CONFIG.
 *
 * Used to setup configuration handle.
 */
class w_credential_config {
  friend class internal::w_raw_access;

 public:
  using cert_variant_type =
      std::variant<w_certificate_none, w_certificate_hash, w_certificate_file>;

  /**
   * @brief construct credential config with given certificate type, and flags.
   * @param p_cert    a supported certificate object.
   * (w_certificate_<none/hash/file>)
   * @param p_flags   flags to associate with the credentials.
   */
  explicit w_credential_config(
      cert_variant_type p_cert,
      QUIC_CREDENTIAL_FLAGS p_flags =
          QUIC_CREDENTIAL_FLAGS::QUIC_CREDENTIAL_FLAG_NONE) noexcept
      : _cert_variant(p_cert) {
    std::memset(&_config, 0, sizeof(_config));
    _config.Flags = p_flags;
    setup_raw_config_from_variant(_cert_variant);
  }

  w_credential_config(const w_credential_config& p_other)
      : w_credential_config(
            p_other._cert_variant,
            static_cast<QUIC_CREDENTIAL_FLAGS>(p_other._config.Flags)) {}

  w_credential_config(w_credential_config&& p_other) noexcept
      : w_credential_config(
            std::move(p_other._cert_variant),
            static_cast<QUIC_CREDENTIAL_FLAGS>(p_other._config.Flags)) {}

  w_credential_config& operator=(const w_credential_config& p_other) {
    _cert_variant = p_other._cert_variant;
    setup_raw_config_from_variant(_cert_variant);
    return *this;
  }

  w_credential_config& operator=(w_credential_config&& p_other) noexcept {
    _cert_variant = std::move(p_other._cert_variant);
    setup_raw_config_from_variant(_cert_variant);
    return *this;
  }

  ~w_credential_config() { /* nothing */
  }

 private:
  void setup_raw_config_from_variant(cert_variant_type& p_cert_variant) {
    std::visit(
        w_overloaded{[this](w_certificate_none&) {
                       _config.Type = QUIC_CREDENTIAL_TYPE_NONE;
                     },
                     [this](w_certificate_hash& p_cert) {
                       _config.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_HASH;
                       _config.CertificateHash =
                           internal::w_raw_access::raw(p_cert);
                     },
                     [this](w_certificate_file& p_cert) {
                       _config.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
                       _config.CertificateFile =
                           internal::w_raw_access::raw(p_cert);
                     }},
        p_cert_variant);
  }

  auto raw() noexcept { return &_config; }
  auto raw() const noexcept { return &_config; }

  cert_variant_type _cert_variant;
  QUIC_CREDENTIAL_CONFIG _config;
};

}  // namespace wolf::stream::quic
