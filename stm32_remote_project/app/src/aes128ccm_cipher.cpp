#include "aes128ccm_cipher.hpp"

Aes128CcmCipher::Aes128CcmCipher(const uint8_t* aes_key) : aes_key_(aes_key)
{
    mbedtls_ccm_init(&ctx_);
}

Aes128CcmCipher::~Aes128CcmCipher()
{
    mbedtls_ccm_free(&ctx_);
}

bool Aes128CcmCipher::init()
{
    if (aes_key_ == nullptr)
    {
        LOG_WARNING(TAG, "AES key can not be empty");
        return false;
    }

    int rc = mbedtls_ccm_setkey(&ctx_,                  //
                                MBEDTLS_CIPHER_ID_AES,  //
                                aes_key_,               //
                                kKeyLength * 8);
    aes_key_ = nullptr;

    if (rc != 0)
    {
        std::array<char, 100> errBuf{};
        mbedtls_strerror(rc, errBuf.data(), errBuf.size());
        LOG_ERROR(TAG, "mbedtls_ccm_setkey failed: %s (rc=-0x%04X)", errBuf.data(), -rc);
        return false;
    }

    return true;
}

bool Aes128CcmCipher::encrypt(const uint8_t* plaintext, size_t length, uint8_t* out_ciphertext)
{
    last_nonce_[0] = static_cast<uint8_t>(tx_counter_ >> 24);
    last_nonce_[1] = static_cast<uint8_t>(tx_counter_ >> 16);
    last_nonce_[2] = static_cast<uint8_t>(tx_counter_ >> 8);
    last_nonce_[3] = static_cast<uint8_t>(tx_counter_);
    last_nonce_[4] = 0;
    last_nonce_[5] = 0;
    last_nonce_[6] = 0;

    tx_counter_++;

    int rc = mbedtls_ccm_encrypt_and_tag(&ctx_,               //
                                         length,              //
                                         last_nonce_.data(),  //
                                         kIvLength,           //
                                         nullptr, 0,          //
                                         plaintext,           //
                                         out_ciphertext,      //
                                         last_tag_.data(),    //
                                         kTagLength           //
    );

    if (rc != 0)
    {
        std::array<char, 100> errBuf{};
        mbedtls_strerror(rc, errBuf.data(), errBuf.size());
        LOG_ERROR(TAG, "mbedtls_ccm_encrypt_and_tag failed: %s (rc=-0x%04X)", errBuf.data(), -rc);
        return false;
    }

    return true;
}

bool Aes128CcmCipher::decrypt(const std::array<uint8_t, kIvLength>& nonce,
                              const uint8_t* ciphertext, size_t length,
                              const std::array<uint8_t, kTagLength>& tag, uint8_t* out_plaintext)
{
    int rc = mbedtls_ccm_auth_decrypt(&ctx_,          //
                                      length,         //
                                      nonce.data(),   //
                                      kIvLength,      //
                                      nullptr, 0,     //
                                      ciphertext,     //
                                      out_plaintext,  //
                                      tag.data(),     //
                                      kTagLength      //
    );

    if (rc != 0)
    {
        std::array<char, 100> errBuf{};
        mbedtls_strerror(rc, errBuf.data(), errBuf.size());
        LOG_WARNING(TAG, "mbedtls_ccm_auth_decrypt failed: %s (rc=-0x%04X)", errBuf.data(), -rc);
        return false;
    }

    return true;
}