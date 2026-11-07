#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "aes128ccm_cipher.hpp"
#include "commands.hpp"
#include "nrf24radio.hpp"

enum class CarLoopState
{
    IDLE,
    BROADCASTING,
    BIND_REQ,
    BIND_RESP,
    BIND_CONFIRM,
    BIND_ACK,
    PING_PONG,
};

class RadioCar
{
   public:
    bool init();
    bool update();
    bool handle_flags();
    void handle_no_flags();

    Nrf24Radio* get_nrf()
    {
        return &nrf_;
    }

    RadioCar(SPI_HandleTypeDef* hspi,  //
             uint32_t device_id,       //
             uint32_t device_random,   //
             uint8_t radio_channel,    //
             std::array<uint8_t, 5> radio_filter_key)
        : nrf_{hspi,  //
               NRF_CSN_PORT,
               NRF_CSN_PIN,  //
               NRF_CE_PORT,
               NRF_CE_PIN,  //
               Direction::HalfDuplex,
               device_id,
               device_random},
          radio_channel_(radio_channel),
          radio_filter_key_(radio_filter_key),
          device_id_(device_id),
          device_random_(device_random)
    {
        tx_packet_.fill(0);
        rx_packet_.fill(0);
    }

   private:
    static constexpr const char* TAG = "RADIO_CAR";

    bool update_loop_state();
    bool run_radio();

    bool handle_rx();
    bool handle_tx();

    Nrf24Radio nrf_;

    CarLoopState loop_state_ = CarLoopState::IDLE;
    Role radio_state_ = Role::RxMode;

    uint32_t radio_channel_{0};
    std::array<uint8_t, 5> radio_filter_key_{0};

    uint8_t idle_count_{0};
    uint32_t device_id_{0};
    uint32_t device_random_{0};

    std::array<uint8_t, 16> tx_packet_{0};
    std::array<uint8_t, 16> rx_packet_{0};
};
