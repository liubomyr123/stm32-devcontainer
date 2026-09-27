#include "include/crypto_selftest.hpp"

#include <cstring>

#include "SecureChannel.hpp"
#include "include/logger.hpp"

bool runCryptoSelfTest()
{
    bool allOk = true;

    const uint8_t testKey[SecureChannel::kKeyLen] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE,
                                                     0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88,
                                                     0x09, 0xCF, 0x4F, 0x3C};

    const uint8_t direction[3] = {0x01, 0x00, 0x00};

    SecureChannel ch;
    if (!ch.setKey(testKey))
    {
        LOG_INFO("CRYPTO", "FAIL: setKey не вдався");
        return false;
    }

    // ---- Сценарій 1: звичайний round-trip ----
    {
        const uint8_t plaintext[20] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
                                       0x4B, 0x49, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53};

        uint8_t nonce7[SecureChannel::kIvLen];
        SecureChannel::buildNonce(47, direction, nonce7);

        uint8_t ciphertext[sizeof(plaintext)];
        uint8_t tag[SecureChannel::kTagLen];

        bool encOk = ch.encrypt(nonce7, plaintext, sizeof(plaintext), ciphertext, tag);

        uint8_t decrypted[sizeof(plaintext)];
        bool decOk = ch.decrypt(nonce7, ciphertext, sizeof(ciphertext), tag, decrypted);

        bool matches = decOk && (memcmp(plaintext, decrypted, sizeof(plaintext)) == 0);

        LOG_INFO("CRYPTO", "Сценарій 1 (round-trip): encrypt=%s decrypt=%s match=%s",
                 encOk ? "OK" : "FAIL", decOk ? "OK" : "FAIL", matches ? "OK" : "FAIL");

        if (!encOk || !decOk || !matches)
            allOk = false;
    }

    // ---- Сценарій 2: підробка ciphertext -> tag НЕ має зійтись ----
    {
        const uint8_t plaintext[20] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
                                       0x4B, 0x49, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53};

        uint8_t nonce7[SecureChannel::kIvLen];
        SecureChannel::buildNonce(48, direction, nonce7);

        uint8_t ciphertext[sizeof(plaintext)];
        uint8_t tag[SecureChannel::kTagLen];
        ch.encrypt(nonce7, plaintext, sizeof(plaintext), ciphertext, tag);

        ciphertext[5] ^= 0xFF;

        uint8_t decrypted[sizeof(plaintext)];
        bool decOk = ch.decrypt(nonce7, ciphertext, sizeof(ciphertext), tag, decrypted);

        bool testPassed = (decOk == false);

        LOG_INFO("CRYPTO", "Сценарій 2 (підроблений ciphertext): decrypt=%s -> тест %s",
                 decOk ? "OK(!)" : "FAIL(очікувано)", testPassed ? "PASSED" : "FAILED");

        if (!testPassed)
            allOk = false;
    }

    // ---- Сценарій 3: підробка tag -> теж НЕ має зійтись ----
    {
        const uint8_t plaintext[20] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
                                       0x4B, 0x49, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53};

        uint8_t nonce7[SecureChannel::kIvLen];
        SecureChannel::buildNonce(49, direction, nonce7);

        uint8_t ciphertext[sizeof(plaintext)];
        uint8_t tag[SecureChannel::kTagLen];
        ch.encrypt(nonce7, plaintext, sizeof(plaintext), ciphertext, tag);

        tag[0] ^= 0xFF;

        uint8_t decrypted[sizeof(plaintext)];
        bool decOk = ch.decrypt(nonce7, ciphertext, sizeof(ciphertext), tag, decrypted);

        bool testPassed = (decOk == false);

        LOG_INFO("CRYPTO", "Сценарій 3 (підроблений tag): decrypt=%s -> тест %s",
                 decOk ? "OK(!)" : "FAIL(очікувано)", testPassed ? "PASSED" : "FAILED");

        if (!testPassed)
            allOk = false;
    }

    LOG_INFO("CRYPTO", "ЗАГАЛЬНИЙ РЕЗУЛЬТАТ: %s", allOk ? "ALL PASSED" : "SOMETHING FAILED");
    return allOk;
}