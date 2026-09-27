#include "include/crypto_selftest.hpp"

#include <cstring>

#include "include/aes128ccm_cipher.hpp"
#include "include/logger.hpp"

void bytesToHex(const uint8_t* data, size_t len, std::array<char, 128>& out)
{
    size_t pos = 0;
    for (size_t i = 0; i < len && pos + 3 < out.size(); ++i)
    {
        pos += static_cast<size_t>(snprintf(out.data() + pos, out.size() - pos, "%02X ", data[i]));
    }
}

bool runCryptoSelfTest()
{
    const uint8_t testKey[Aes128CcmCipher::kKeyLength] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE,
                                                          0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88,
                                                          0x09, 0xCF, 0x4F, 0x3C};

    Aes128CcmCipher cipher(testKey);
    if (!cipher.init())
    {
        return false;
    }

    const uint8_t plaintext[20] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
                                   0x4B, 0x49, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53};

    std::array<char, 128> hexBuf{};

    bytesToHex(plaintext, sizeof(plaintext), hexBuf);
    LOG_INFO("CRYPTO", "Сирі дані:        %s", hexBuf.data());

    // ---- Перший виклик encrypt() ----
    uint8_t ciphertext1[sizeof(plaintext)];
    bool encOk1 = cipher.encrypt(plaintext, sizeof(plaintext), ciphertext1);

    bytesToHex(ciphertext1, sizeof(ciphertext1), hexBuf);
    LOG_INFO("CRYPTO", "Зашифровано (1):  %s", hexBuf.data());

    bytesToHex(cipher.getLastNonce().data(), Aes128CcmCipher::kIvLength, hexBuf);
    LOG_INFO("CRYPTO", "Nonce (1):        %s", hexBuf.data());

    bytesToHex(cipher.getLastTag().data(), Aes128CcmCipher::kTagLength, hexBuf);
    LOG_INFO("CRYPTO", "Tag (1):          %s", hexBuf.data());

    // ---- Другий виклик encrypt() — той самий plaintext, той самий ключ ----
    uint8_t ciphertext2[sizeof(plaintext)];
    bool encOk2 = cipher.encrypt(plaintext, sizeof(plaintext), ciphertext2);

    bytesToHex(ciphertext2, sizeof(ciphertext2), hexBuf);
    LOG_INFO("CRYPTO", "Зашифровано (2):  %s", hexBuf.data());

    bytesToHex(cipher.getLastNonce().data(), Aes128CcmCipher::kIvLength, hexBuf);
    LOG_INFO("CRYPTO", "Nonce (2):        %s", hexBuf.data());

    bytesToHex(cipher.getLastTag().data(), Aes128CcmCipher::kTagLength, hexBuf);
    LOG_INFO("CRYPTO", "Tag (2):          %s", hexBuf.data());

    // ---- Розшифровуємо другий пакет і звіряємо ----
    uint8_t decrypted[sizeof(plaintext)];
    bool decOk = cipher.decrypt(cipher.getLastNonce(), ciphertext2, sizeof(ciphertext2),
                                cipher.getLastTag(), decrypted);

    bytesToHex(decrypted, sizeof(decrypted), hexBuf);
    LOG_INFO("CRYPTO", "Розшифровано:     %s", hexBuf.data());

    bool matches = decOk && (memcmp(plaintext, decrypted, sizeof(plaintext)) == 0);

    LOG_INFO("CRYPTO", "Round-trip: encrypt1=%s encrypt2=%s decrypt=%s match=%s",
             encOk1 ? "OK" : "FAIL",  //
             encOk2 ? "OK" : "FAIL",  //
             decOk ? "OK" : "FAIL",   //
             matches ? "OK" : "FAIL");

    return encOk1 && encOk2 && decOk && matches;
}