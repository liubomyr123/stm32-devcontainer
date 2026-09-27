#pragma once

#include <array>

#include "include/logger.hpp"
#include "mbedtls/ccm.h"
#include "mbedtls/error.h"

class Aes128CcmCipher
{
   public:
    static constexpr size_t kKeyLength = 16;  // AES-128 secret key
    static constexpr size_t kIvLength = 7;    // nonce
    static constexpr size_t kTagLength = 4;   // auth tag

    bool init();

    Aes128CcmCipher(const uint8_t* aes_key);
    ~Aes128CcmCipher();

    Aes128CcmCipher(const Aes128CcmCipher&) = delete;
    Aes128CcmCipher& operator=(const Aes128CcmCipher&) = delete;
    Aes128CcmCipher(Aes128CcmCipher&&) = delete;
    Aes128CcmCipher& operator=(Aes128CcmCipher&&) = delete;

    bool encrypt(const uint8_t* plaintext, size_t length, uint8_t* out_ciphertext);
    bool decrypt(const std::array<uint8_t, kIvLength>& nonce, const uint8_t* ciphertext,
                 size_t length, const std::array<uint8_t, kTagLength>& tag, uint8_t* out_plaintext);

    const std::array<uint8_t, kIvLength>& getLastNonce() const
    {
        return last_nonce_;
    }
    const std::array<uint8_t, kTagLength>& getLastTag() const
    {
        return last_tag_;
    }

   private:
    static constexpr const char* TAG = "AES128";

    uint32_t tx_counter_ = 0;

    std::array<uint8_t, kIvLength> last_nonce_{};
    std::array<uint8_t, kTagLength> last_tag_{};

    const uint8_t* aes_key_ = nullptr;
    mbedtls_ccm_context ctx_{};
};
