#pragma once

#include <array>
#include <cstring>

#include "include/commands.hpp"
#include "include/logger.hpp"
#include "mbedtls/ccm.h"
#include "mbedtls/error.h"

class Aes128CcmCipher
{
   public:
    static constexpr size_t aes_key_length = 16;  // AES-128 secret key
    static constexpr size_t nonce_length = 7;     // nonce
    static constexpr size_t tag_length = 4;       // auth tag

    bool init();

    Aes128CcmCipher(const uint8_t* aes_key);
    ~Aes128CcmCipher();

    Aes128CcmCipher(const Aes128CcmCipher&) = delete;
    Aes128CcmCipher& operator=(const Aes128CcmCipher&) = delete;
    Aes128CcmCipher(Aes128CcmCipher&&) = delete;
    Aes128CcmCipher& operator=(Aes128CcmCipher&&) = delete;

    bool encrypt(const uint8_t* plaintext, size_t length, uint8_t* out_ciphertext);

    template <typename CommandT>
    using Packet = std::array<uint8_t, nonce_length + sizeof(CommandT) + tag_length>;

    template <typename CommandT>
    bool encryptAndPack(const CommandT& cmd, Packet<CommandT>& out_packet)
    {
        std::array<uint8_t, sizeof(CommandT)> plain_data{};
        std::memcpy(plain_data.data(), &cmd, sizeof(CommandT));

        std::array<uint8_t, sizeof(CommandT)> encrypted_data{};
        if (!encrypt(plain_data.data(), plain_data.size(), encrypted_data.data()))
        {
            return false;
        }

        std::memcpy(out_packet.data(),      //
                    getLastNonce().data(),  //
                    nonce_length);
        std::memcpy(out_packet.data() + nonce_length,  //
                    encrypted_data.data(),             //
                    encrypted_data.size());
        std::memcpy(out_packet.data() + nonce_length + encrypted_data.size(),
                    getLastTag().data(),  //
                    tag_length);

        return true;
    }

    bool decrypt(const std::array<uint8_t, nonce_length>& nonce, const uint8_t* ciphertext,
                 size_t length, const std::array<uint8_t, tag_length>& tag, uint8_t* out_plaintext);

    template <typename CommandT>
    bool unpackAndDecrypt(const Packet<CommandT>& packet, CommandT& out_cmd)
    {
        std::array<uint8_t, nonce_length> nonce{};
        std::memcpy(nonce.data(), packet.data(), nonce_length);

        std::array<uint8_t, sizeof(CommandT)> encrypted_data{};
        std::memcpy(encrypted_data.data(), packet.data() + nonce_length, sizeof(CommandT));

        std::array<uint8_t, tag_length> tag{};
        std::memcpy(tag.data(), packet.data() + nonce_length + sizeof(CommandT), tag_length);

        std::array<uint8_t, sizeof(CommandT)> plain_data{};
        if (!decrypt(nonce,                  //
                     encrypted_data.data(),  //
                     encrypted_data.size(),  //
                     tag,                    //
                     plain_data.data()))
        {
            return false;
        }

        std::memcpy(&out_cmd, plain_data.data(), sizeof(CommandT));
        return true;
    }

    const std::array<uint8_t, nonce_length>& getLastNonce() const
    {
        return last_nonce_;
    }
    const std::array<uint8_t, tag_length>& getLastTag() const
    {
        return last_tag_;
    }

   private:
    static constexpr const char* TAG = "AES128";

    uint32_t tx_counter_ = 0;

    std::array<uint8_t, nonce_length> last_nonce_{};
    std::array<uint8_t, tag_length> last_tag_{};

    const uint8_t* aes_key_ = nullptr;
    mbedtls_ccm_context ctx_{};
};
