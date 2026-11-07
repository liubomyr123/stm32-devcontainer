#include "radio_car.hpp"

#include "logger.hpp"

bool RadioCar::init()
{
    if (!nrf_.init())
    {
        return false;
    }

    bool result = nrf_.setAirDataRate(DataRate::Mbps1);
    if (!result)
    {
        return false;
    }

    result = nrf_.setChannel(radio_channel_);
    if (!result)
    {
        return false;
    }

    result = nrf_.setTxRfFilterKey(radio_filter_key_);
    if (!result)
    {
        return false;
    }

    result = nrf_.setRxRfFilterKey(radio_filter_key_);
    if (!result)
    {
        return false;
    }

    result = nrf_.setRxPayloadLength(16);
    if (!result)
    {
        return false;
    }

    LOG_INFO(TAG, "Car radio is initialised");

    return true;
};

bool RadioCar::update_loop_state()
{
    switch (loop_state_)
    {
        case CarLoopState::IDLE:
        {
            if (idle_count_ < 5)  // Just for testing
            {
                LOG_INFO(TAG, "Idle count: %d of 5", idle_count_);
                idle_count_++;
                osDelay(1000);
            }
            else
            {
                radio_state_ = Role::TxMode;
                loop_state_ = CarLoopState::BROADCASTING;
                LOG_INFO(TAG, "Switching to broadcasting...");
            }
            return false;
        }
        case CarLoopState::BROADCASTING:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            LOG_INFO(TAG, "Start broadcasting car ID: %u", device_id_);

            CarIdCommand cmd{};
            cmd.carId = device_id_;

            tx_packet_[0] = static_cast<uint8_t>(FrameType::Plain);
            std::memcpy(tx_packet_.data() + sizeof(FrameType), &cmd, sizeof(cmd));

            break;
        }
        case CarLoopState::BIND_REQ:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            if (!found_target_car)
            {
                radio_state_ = Role::RxMode;
                loop_state_ = CarLoopState::SEARCHING;
                continue;
            }

            BindReqCommand bind_req_cmd{};
            bind_req_cmd.remoteId = nrf_.getDeviceId();
            bind_req_cmd.remoteRandom = nrf_.getDeviceRandom();
            bind_req_cmd.targetCarId = CAR_ID_TO_CONNECT;

            tx_packet_[0] = static_cast<uint8_t>(FrameType::Plain);
            std::memcpy(tx_packet_.data() + 1, &bind_req_cmd, sizeof(bind_req_cmd));
            break;
        }
        case CarLoopState::BIND_RESP:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);
            break;
        }
        case CarLoopState::BIND_CONFIRM:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            BindConfirmCommand bind_confirm_cmd{};

            tx_packet_[0] = static_cast<uint8_t>(FrameType::Encrypted);
            std::memcpy(tx_packet_.data() + 1, &bind_confirm_cmd, sizeof(bind_confirm_cmd));
            break;
        }
        case CarLoopState::BIND_ACK:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);
            break;
        }
        case CarLoopState::PING_PONG:
        {
            break;
        }
        default:
        {
            break;
        }
    }
    return true;
};

bool RadioCar::run_radio()
{
    bool result = nrf_.goStandbyI();
    if (!result)
    {
        LOG_ERROR("RADIO", "goStandbyI() failed unexpectedly");
        return false;
    }

    switch (radio_state_)
    {
        case Role::TxMode:
        {
            auto frame_type = static_cast<FrameType>(tx_packet_.at(0));

            switch (frame_type)
            {
                case FrameType::Plain:
                {
                    if (!nrf_.transmit(tx_packet_.data(), static_cast<uint8_t>(tx_packet_.size())))
                    {
                        return false;
                    }
                    break;
                }
                case FrameType::Encrypted:
                {
                    std::array<uint8_t, 16> encrypted_tx_packet{};
                    encrypted_tx_packet[0] = static_cast<uint8_t>(FrameType::Encrypted);

                    if (!tx_cipher.encrypt(tx_packet_.data() + sizeof(FrameType),
                                           tx_packet_.size() - sizeof(FrameType),
                                           encrypted_tx_packet.data() + sizeof(FrameType)))
                    {
                        return false;
                    }
                    if (!nrf_.transmit(encrypted_tx_packet.data(),
                                       static_cast<uint8_t>(encrypted_tx_packet.size())))
                    {
                        return false;
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }

            if (!nrf_.startTx())
            {
                return false;
            }
            break;
        }
        case Role::RxMode:
        {
            if (!nrf_.startRx())
            {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool RadioCar::update()
{
    if (!update_loop_state())
    {
        return false;  // loop again
    }

    if (!run_radio())
    {
        return false;  // loop again
    }

    return true;  // continue loop
}

bool RadioCar::handle_flags()
{
    uint8_t status = nrf_.readRegister(REG_STATUS);

    if ((status & STATUS_RX_DR_BIT) != 0)
    {
        handle_rx();
    }

    if ((status & STATUS_TX_DS_BIT) != 0)
    {
        handle_tx();
    }

    return true;
}

void RadioCar::handle_no_flags()
{
    if (radio_state_ == Role::TxMode)
    {
        nrf_.sendCommand(FLUSH_TX);
    }
    if (radio_state_ == Role::RxMode)
    {
        radio_state_ = Role::TxMode;
    }
}

bool RadioCar::handle_rx()
{
    if (!nrf_.receive(rx_packet_.data(), static_cast<uint8_t>(rx_packet_.size())))
    {
        LOG_ERROR("RADIO", "receive() failed unexpectedly despite RX_DR set");
        continue;
    }

    switch (radio_state)
    {
        case Role::TxMode:
        {
            break;
        }
        case Role::RxMode:
        {
            switch (loop_state)
            {
                case LoopState::IDLE:
                {
                    break;
                }
                case LoopState::SEARCHING:
                {
                    auto frame_type = static_cast<FrameType>(rx_packet_.at(0));
                    switch (frame_type)
                    {
                        case FrameType::Plain:
                        {
                            CommandOpcode opcode{};
                            std::memcpy(&opcode, rx_packet_.data() + sizeof(FrameType),
                                        sizeof(opcode));

                            switch (opcode)
                            {
                                case CommandOpcode::CarId:
                                {
                                    CarIdCommand bind_res_cmd{};
                                    std::memcpy(&bind_res_cmd,
                                                rx_packet_.data() + sizeof(FrameType),
                                                sizeof(CarIdCommand));
                                    LOG_INFO("RADIO", "car_id=%d", bind_res_cmd.carId);
                                    if (bind_res_cmd.carId == CAR_ID_TO_CONNECT)
                                    {
                                        found_target_car = true;
                                        radio_state = Role::TxMode;
                                        loop_state = LoopState::BIND_REQ;
                                    }

                                    // if (car_id_list_length < 4)
                                    // {
                                    //     bool found = false;
                                    //     for (size_t i = 0; i < CAR_ID_LIST_SIZE; i++)
                                    //     {
                                    //         if (car_id_list.at(i) ==
                                    //         bind_res_cmd.carId)
                                    //         {
                                    //             found = true;
                                    //             break;
                                    //         }
                                    //     }
                                    //     if (!found)
                                    //     {
                                    //         car_id_list.at(car_id_list_length) =
                                    //             bind_res_cmd.carId;
                                    //         car_id_list_length++;
                                    //     }
                                    // }

                                    break;
                                }
                                default:
                                {
                                    //
                                    break;
                                }
                            }
                            break;
                        }
                        case FrameType::Encrypted:
                        {
                            // Error, we expect plain type of packet
                            break;
                        }
                        default:
                        {
                            // Error, we expect plain type of packet
                            break;
                        }
                    }
                    break;
                }
                case LoopState::BIND_REQ:
                {
                    break;
                }
                case LoopState::BIND_RESP:
                {
                    auto frame_type = static_cast<FrameType>(rx_packet_.at(0));
                    switch (frame_type)
                    {
                        case FrameType::Plain:
                        {
                            CommandOpcode opcode{};
                            std::memcpy(&opcode, rx_packet_.data() + sizeof(FrameType),
                                        sizeof(opcode));

                            switch (opcode)
                            {
                                case CommandOpcode::BindResp:
                                {
                                    BindRespCommand bind_res_cmd{};
                                    std::memcpy(&bind_res_cmd,
                                                rx_packet_.data() + sizeof(FrameType),
                                                sizeof(BindRespCommand));
                                    LOG_INFO("RADIO",
                                             "car_id=%d | carRandom=%d | "
                                             "remoteRandomEcho=%d",
                                             bind_res_cmd.carId, bind_res_cmd.carRandom,
                                             bind_res_cmd.remoteRandomEcho);

                                    if (bind_res_cmd.remoteRandomEcho != device_id)
                                    {
                                        // Error
                                        break;
                                    }

                                    // crating key_remote_to_car and init cipher
                                    //  [remote_id 4][car_id 4][R1 4][R2 4], де блок[0]
                                    //  ^= маркер напрямку:

                                    std::array<uint8_t, 16> aes_key{};
                                    // 4
                                    std::memcpy(aes_key.data(), &device_id, sizeof(device_id));
                                    // 4
                                    std::memcpy(aes_key.data() + sizeof(device_id),
                                                rx_packet_.data() + sizeof(FrameType) +
                                                    sizeof(CommandOpcode) +
                                                    sizeof(BindRespCommand::carId),
                                                sizeof(BindRespCommand::carId));
                                    // 4
                                    std::memcpy(aes_key.data() + sizeof(device_id) +
                                                    sizeof(BindRespCommand::carId),
                                                rx_packet_.data() + sizeof(FrameType) +
                                                    sizeof(CommandOpcode) +
                                                    sizeof(BindRespCommand::carId) +
                                                    sizeof(BindRespCommand::carRandom),
                                                sizeof(BindRespCommand::carRandom));
                                    // 4
                                    std::memcpy(aes_key.data() + sizeof(device_id) +
                                                    sizeof(BindRespCommand::carId) +
                                                    sizeof(BindRespCommand::carRandom),
                                                &device_random, sizeof(device_random));

                                    {
                                        aes_key.at(0) ^= 1;
                                        rx_cipher.setAesKey(aes_key.data());

                                        if (!rx_cipher.init())
                                        {
                                            vTaskDelete(nullptr);
                                        }
                                    }
                                    {
                                        aes_key.at(0) ^= 0;
                                        tx_cipher.setAesKey(aes_key.data());

                                        if (!tx_cipher.init())
                                        {
                                            vTaskDelete(nullptr);
                                        }
                                    }
                                    radio_state = Role::TxMode;
                                    loop_state = LoopState::BIND_CONFIRM;
                                    break;
                                }
                                default:
                                {
                                    //
                                    break;
                                }
                            }
                            break;
                        }
                        case FrameType::Encrypted:
                        {
                            // Error, we expect plain type of packet
                            break;
                        }
                        default:
                        {
                            // Error, we expect plain type of packet
                            break;
                        }
                    }
                    break;
                }
                case LoopState::BIND_CONFIRM:
                {
                    break;
                }
                case LoopState::BIND_ACK:
                {
                    auto frame_type = static_cast<FrameType>(rx_packet_.at(0));
                    switch (frame_type)
                    {
                        case FrameType::Plain:
                        {
                            // Error, we expect encrypted type of packet
                            break;
                        }
                        case FrameType::Encrypted:
                        {
                            CommandOpcode opcode{};
                            std::memcpy(&opcode, rx_packet_.data() + sizeof(FrameType),
                                        sizeof(opcode));

                            switch (opcode)
                            {
                                case CommandOpcode::BindAck:
                                {
                                    std::array<uint8_t, Aes128CcmCipher::nonce_length> nonce{};
                                    std::memcpy(nonce.data(), rx_packet_.data() + sizeof(FrameType),
                                                Aes128CcmCipher::nonce_length);

                                    std::array<uint8_t, sizeof(BindAckCommand)> encrypted_data{};
                                    std::memcpy(encrypted_data.data(),
                                                rx_packet_.data() + sizeof(FrameType) +
                                                    Aes128CcmCipher::nonce_length,
                                                sizeof(BindAckCommand));

                                    std::array<uint8_t, Aes128CcmCipher::tag_length> tag{};
                                    std::memcpy(tag.data(),
                                                rx_packet_.data() + Aes128CcmCipher::nonce_length +
                                                    sizeof(FrameType) + sizeof(BindAckCommand),
                                                Aes128CcmCipher::tag_length);

                                    std::array<uint8_t, sizeof(BindAckCommand)> plain_data{};
                                    if (!rx_cipher.decrypt(nonce,                  //
                                                           encrypted_data.data(),  //
                                                           encrypted_data.size(),  //
                                                           tag,                    //
                                                           plain_data.data()))
                                    {
                                        continue;
                                    }

                                    BindAckCommand bind_ack_cmd{};
                                    std::memcpy(&bind_ack_cmd,
                                                rx_packet_.data() + sizeof(FrameType),
                                                sizeof(BindAckCommand));
                                    LOG_INFO("RADIO", "BindAckCommand");
                                    loop_state = LoopState::PING_PONG;
                                    break;
                                }
                                default:
                                {
                                    //
                                    break;
                                }
                            }
                            break;
                        }
                        default:
                        {
                            // Error, we expect plain type of packet
                            break;
                        }
                    }
                    break;
                }
                case LoopState::PING_PONG:
                {
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        default:
        {
            break;
        }
    }

    // "Write 1 to clear bit" (Table 28)
    nrf_.writeRegister(REG_STATUS, STATUS_RX_DR_BIT);
}

bool RadioCar::handle_tx()
{
    LOG_INFO("RADIO", "OK: TX_DS, sent=%d", tx_counter);
    nrf_.writeRegister(REG_STATUS, STATUS_TX_DS_BIT);

    switch (loop_state)
    {
        case LoopState::IDLE:
        {
            break;
        }
        case LoopState::BIND_REQ:
        {
            loop_state = LoopState::BIND_RESP;
            break;
        }
        case LoopState::BIND_CONFIRM:
        {
            loop_state = LoopState::BIND_ACK;
            break;
        }
        case LoopState::BIND_ACK:
        {
            break;
        }
        case LoopState::PING_PONG:
        {
            break;
        }
        default:
        {
            break;
        }
    }
    radio_state = Role::RxMode;
}