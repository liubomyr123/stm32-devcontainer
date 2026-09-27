#include "SecureChannel.hpp"

#include <cstring>

SecureChannel::SecureChannel()
{
    mbedtls_ccm_init(&ctx_);
}

SecureChannel::~SecureChannel()
{
    mbedtls_ccm_free(&ctx_);  // затирає ключ з пам'яті, не просто "нічого не робить"
}

bool SecureChannel::setKey(const uint8_t* key)
{
    int rc = mbedtls_ccm_setkey(&ctx_, MBEDTLS_CIPHER_ID_AES, key,
                                kKeyLen * 8 /* mbedTLS хоче розмір ключа в БІТАХ */);
    keySet_ = (rc == 0);
    return keySet_;
}

void SecureChannel::buildNonce(uint32_t counter, const uint8_t directionContext[3],
                               uint8_t outNonce7[kIvLen])
{
    // Перші 4 байти — наш лічильник (те, що реально летить по радіо),
    // великий-ендіан для читабельності при дебазі в hex-дампі.
    outNonce7[0] = static_cast<uint8_t>(counter >> 24);
    outNonce7[1] = static_cast<uint8_t>(counter >> 16);
    outNonce7[2] = static_cast<uint8_t>(counter >> 8);
    outNonce7[3] = static_cast<uint8_t>(counter);

    // Останні 3 байти — фіксований контекст напрямку (наприклад,
    // 0x01 для пульт->машинка, 0x02 для машинка->пульт — узгоджується
    // на Bind, обидві сторони знають наперед, НЕ передається по радіо).
    outNonce7[4] = directionContext[0];
    outNonce7[5] = directionContext[1];
    outNonce7[6] = directionContext[2];
}

bool SecureChannel::encrypt(const uint8_t nonce7[kIvLen], const uint8_t* plaintext, size_t len,
                            uint8_t* outCiphertext, uint8_t outTag[kTagLen])
{
    if (!keySet_)
        return false;

    int rc = mbedtls_ccm_encrypt_and_tag(&ctx_, len, nonce7, kIvLen, nullptr,
                                         0,  // Additional Data — не використовуємо
                                         plaintext, outCiphertext, outTag, kTagLen);

    return rc == 0;
}

bool SecureChannel::decrypt(const uint8_t nonce7[kIvLen], const uint8_t* ciphertext, size_t len,
                            const uint8_t tag[kTagLen], uint8_t* outPlaintext)
{
    if (!keySet_)
        return false;

    // mbedtls_ccm_auth_decrypt САМ звіряє tag і повертає ненульовий
    // код (MBEDTLS_ERR_CCM_AUTH_FAILED), якщо не зійшовся — саме та
    // перевірка, яку ми розписували вручну (Крок 5 у нашій моделі).
    int rc = mbedtls_ccm_auth_decrypt(&ctx_, len, nonce7, kIvLen, nullptr, 0, ciphertext,
                                      outPlaintext, tag, kTagLen);

    return rc == 0;
}