#pragma once

#include <cstddef>
#include <cstdint>

#include "mbedtls/ccm.h"

/**
 * SecureChannel — тонка обгортка над mbedtls_ccm_* для одного напрямку
 * зв'язку (один ключ = один SecureChannel; для двонапрямного каналу
 * буде два окремих екземпляри, K1 і K2 — саме так, як ми домовились).
 *
 * Свідомі параметри для нашого проєкту (не змінюй без причини):
 *   - iv_len  = 7 байт  — мінімально дозволений mbedTLS розмір nonce,
 *                          економить байти в 32-байтному ліміті nRF24.
 *   - tag_len = 4 байти — компроміс надійність/розмір, обраний раніше.
 *
 * Клас НЕ знає нічого про радіо, про Bind, про replay-захист (лічильник
 * "останній прийнятий nonce") — це все відповідальність вищого рівня
 * (протокольного класу), SecureChannel — лише сам крипто-примітив.
 */
class SecureChannel
{
   public:
    static constexpr size_t kKeyLen = 16;  // AES-128
    static constexpr size_t kIvLen = 7;    // nonce, як домовились
    static constexpr size_t kTagLen = 4;   // tag, як домовились

    SecureChannel();
    ~SecureChannel();

    // Забороняємо копіювання — контекст mbedTLS тримає стан ключа,
    // копіювати його "як є" небезпечно і безглуздо.
    SecureChannel(const SecureChannel&) = delete;
    SecureChannel& operator=(const SecureChannel&) = delete;

    /**
     * Ініціалізація ключем. Викликається один раз (чи повторно на Bind,
     * коли міняється сесійний ключ).
     * @param key     рівно kKeyLen (16) байт
     * @return true при успіху
     */
    bool setKey(const uint8_t* key);

    /**
     * Побудувати повний 7-байтний nonce з нашого маленького 4-байтного
     * лічильника + фіксованого 3-байтного контексту напрямку (те, що
     * ми обговорювали як спосіб не передавати всі 7 байт по радіо —
     * летить лише 4-байтний counter, решта 3 байти обидві сторони
     * знають наперед).
     */
    static void buildNonce(uint32_t counter, const uint8_t directionContext[3],
                           uint8_t outNonce7[kIvLen]);

    /**
     * Зашифрувати + порахувати tag.
     * @param nonce7        повний 7-байтний nonce (після buildNonce)
     * @param plaintext     сирі дані
     * @param len           довжина сирих даних
     * @param outCiphertext буфер на len байт
     * @param outTag        буфер на kTagLen байт
     * @return true при успіху
     */
    bool encrypt(const uint8_t nonce7[kIvLen], const uint8_t* plaintext, size_t len,
                 uint8_t* outCiphertext, uint8_t outTag[kTagLen]);

    /**
     * Розшифрувати + перевірити tag.
     * @return true ЛИШЕ якщо tag зійшовся І дані успішно розшифровані.
     *         false — пакет відкидається повністю, outPlaintext
     *         в цьому випадку недостовірний і використовуватись не має.
     */
    bool decrypt(const uint8_t nonce7[kIvLen], const uint8_t* ciphertext, size_t len,
                 const uint8_t tag[kTagLen], uint8_t* outPlaintext);

   private:
    mbedtls_ccm_context ctx_;
    bool keySet_ = false;
};