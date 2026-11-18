#include "radio_car.hpp"

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

    result = nrf_.setRxPayloadLength(RX_PAYLOAD_LENGTH);
    if (!result)
    {
        return false;
    }

    LOG_INFO(TAG, "Car radio is initialised");

    return true;
};

bool RadioCar::update_loop_state()
{
    bool is_run_radio = true;

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
                idle_count_ = 0;
                LOG_INFO(TAG, "Switching to broadcasting...");
            }
            is_run_radio = false;
            break;
        }
        case CarLoopState::BROADCASTING:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            LOG_INFO(TAG, "Start broadcasting car ID: %lu", device_id_);

            CarIdCommand cmd{};
            cmd.carId = device_id_;

            tx_packet_[0] = static_cast<uint8_t>(FrameType::Plain);
            std::memcpy(tx_packet_.data() + sizeof(FrameType), &cmd, sizeof(cmd));

            is_run_radio = true;
            break;
        }
        case CarLoopState::BIND_REQ:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            is_run_radio = true;
            break;
        }
        case CarLoopState::BIND_RESP:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            BindRespCommand cmd{};
            cmd.carId = device_id_;
            cmd.carRandom = device_random_;
            cmd.remoteRandomEcho = remote_random_;

            tx_packet_[0] = static_cast<uint8_t>(FrameType::Plain);
            std::memcpy(tx_packet_.data() + sizeof(FrameType), &cmd, sizeof(cmd));

            is_run_radio = true;
            break;
        }
        case CarLoopState::BIND_CONFIRM:
        {
            tx_packet_.fill(0);
            rx_packet_.fill(0);

            is_run_radio = true;
            break;
        }
        case CarLoopState::BIND_ACK:
        case CarLoopState::PING_PONG:
        default:
        {
            break;
        }
    }
    return is_run_radio;
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

    uint8_t handled = 0;
    bool result = true;

    if (radio_state_ == Role::RxMode && ((status & STATUS_RX_DR_BIT) != 0))
    {
        handled = STATUS_RX_DR_BIT;
        result = handle_rx();
    }
    else if (radio_state_ == Role::TxMode && ((status & STATUS_TX_DS_BIT) != 0))
    {
        handled = STATUS_TX_DS_BIT;
        result = handle_tx();
    }

    // Take only the IRQ bits from status and drop the handled one: what remains must be cleared
    uint8_t stale = status & STATUS_IRQ_MASK & ~handled;
    if (stale != 0)
    {
        if ((stale & STATUS_RX_DR_BIT) != 0)
        {
            LOG_WARNING(TAG, "Clearing stale RX_DR");
        }

        if ((stale & STATUS_TX_DS_BIT) != 0)
        {
            LOG_WARNING(TAG, "Clearing stale TX_DS");
        }

        if ((stale & STATUS_MAX_RT_BIT) != 0)
        {
            LOG_WARNING(TAG, "Clearing stale MAX_RT");
        }

        nrf_.writeRegister(REG_STATUS, stale);
    }

    return result;
}

void RadioCar::handle_no_flags()
{
    if (radio_state_ == Role::TxMode)
    {
        nrf_.sendCommand(FLUSH_TX);

        switch (loop_state_)
        {
            case CarLoopState::BROADCASTING:
            {
                LOG_INFO(TAG, "Start listening BIND_REQ for %dms", RadioCar::LISTENING_TIME_MS);

                radio_state_ = Role::RxMode;
                loop_state_ = CarLoopState::BIND_REQ;
                listening_deadline_ms_ = HAL_GetTick() + RadioCar::LISTENING_TIME_MS;
                break;
            }
            case CarLoopState::BIND_RESP:
            {
                LOG_ERROR(TAG, "BIND_RESP: chip did not report TX_DS, back to broadcasting");
                radio_state_ = Role::TxMode;
                loop_state_ = CarLoopState::BROADCASTING;
                break;
            }
            case CarLoopState::IDLE:
            case CarLoopState::BIND_REQ:
            case CarLoopState::BIND_CONFIRM:
            case CarLoopState::BIND_ACK:
            case CarLoopState::PING_PONG:
            default:
            {
                break;
            }
        }
    }
    else if (radio_state_ == Role::RxMode)
    {
        switch (loop_state_)
        {
            case CarLoopState::BIND_REQ:
            {
                LOG_INFO(TAG, "BIND_REQ timeout, back to broadcasting");

                radio_state_ = Role::TxMode;
                loop_state_ = CarLoopState::BROADCASTING;
                break;
            }
            case CarLoopState::BIND_CONFIRM:
            {
                LOG_INFO(TAG, "BIND_CONFIRM timeout, back to broadcasting");

                radio_state_ = Role::TxMode;
                loop_state_ = CarLoopState::BROADCASTING;
                break;
            }
            case CarLoopState::IDLE:
            case CarLoopState::BROADCASTING:
            case CarLoopState::BIND_RESP:
            case CarLoopState::BIND_ACK:
            case CarLoopState::PING_PONG:
            default:
            {
                break;
            }
        }
    }
}

bool RadioCar::handle_rx()
{
    LOG_INFO(TAG, "OK: RX_DS");
    nrf_.writeRegister(REG_STATUS, STATUS_RX_DR_BIT);

    if (radio_state_ == Role::TxMode)
    {
        LOG_INFO(TAG, "We can not be in TX mode");
        return false;
    }

    if (!nrf_.receive(rx_packet_.data(), static_cast<uint8_t>(rx_packet_.size())))
    {
        LOG_ERROR("RADIO", "receive() failed unexpectedly despite RX_DR set");
        return false;
    }

    switch (loop_state_)
    {
        case CarLoopState::BIND_REQ:
        {
            auto frame_type = static_cast<FrameType>(rx_packet_.at(0));
            if (frame_type == FrameType::Plain)
            {
                CommandOpcode opcode{};
                std::memcpy(&opcode,                                // dest
                            rx_packet_.data() + sizeof(FrameType),  // src
                            sizeof(CommandOpcode)                   // size
                );

                switch (opcode)
                {
                    case CommandOpcode::BindReq:
                    {
                        BindReqCommand cmd{};
                        std::memcpy(&cmd,                                   // dest
                                    rx_packet_.data() + sizeof(FrameType),  // src
                                    sizeof(BindReqCommand)                  // size
                        );
                        LOG_INFO(TAG, "RX BindReq: OK");
                        LOG_INFO(TAG, "remoteId=%lu", cmd.remoteId);
                        LOG_INFO(TAG, "remoteRandom=%lu", cmd.remoteRandom);
                        LOG_INFO(TAG, "targetCarId=%lu", cmd.targetCarId);

                        if (cmd.targetCarId == device_id_)
                        {
                            LOG_INFO(TAG, "Remote found correct car.");

                            remote_id_ = cmd.remoteId;
                            remote_random_ = cmd.remoteRandom;

                            radio_state_ = Role::TxMode;
                            loop_state_ = CarLoopState::BIND_RESP;
                        }
                        else
                        {
                            LOG_WARNING(TAG, "Remote is searching for another car id.");

                            radio_state_ = Role::RxMode;
                            loop_state_ = CarLoopState::BIND_REQ;
                        }
                        break;
                    }
                    default:
                    {
                        LOG_WARNING(TAG, "RX BindReq: wrong command");
                        break;
                    }
                }
            }
            break;
        }
        case CarLoopState::BIND_CONFIRM:
        {
            auto frame_type = static_cast<FrameType>(rx_packet_.at(0));
            if (frame_type == FrameType::Plain)
            {
                CommandOpcode opcode{};
                std::memcpy(&opcode,                                // dest
                            rx_packet_.data() + sizeof(FrameType),  // src
                            sizeof(CommandOpcode)                   // size
                );

                switch (opcode)
                {
                    case CommandOpcode::BindConfirm:
                    {
                        BindConfirmCommand cmd{};
                        std::memcpy(&cmd,                                   // dest
                                    rx_packet_.data() + sizeof(FrameType),  // src
                                    sizeof(BindConfirmCommand)              // size
                        );

                        LOG_INFO(TAG, "RX BindConfirm: OK");

                        radio_state_ = Role::TxMode;
                        loop_state_ = CarLoopState::BIND_ACK;
                        break;
                    }
                    default:
                    {
                        LOG_WARNING(TAG, "RX BindConfirm: wrong command");
                        break;
                    }
                }
            }
            break;
        }
        case CarLoopState::IDLE:
        case CarLoopState::BIND_RESP:
        case CarLoopState::BIND_ACK:
        case CarLoopState::PING_PONG:
        default:
        {
            break;
        }
    }

    return true;
}

bool RadioCar::handle_tx()
{
    LOG_INFO(TAG, "OK: TX_DS");
    nrf_.writeRegister(REG_STATUS, STATUS_TX_DS_BIT);

    if (radio_state_ == Role::RxMode)
    {
        LOG_INFO(TAG, "We can not be in RX mode");
        return false;
    }

    switch (loop_state_)
    {
        case CarLoopState::BROADCASTING:
        {
            LOG_INFO(TAG, "Start listening BIND_REQ for %dms", RadioCar::LISTENING_TIME_MS);

            radio_state_ = Role::RxMode;
            loop_state_ = CarLoopState::BIND_REQ;
            listening_deadline_ms_ = HAL_GetTick() + RadioCar::LISTENING_TIME_MS;
            break;
        }
        case CarLoopState::BIND_RESP:
        {
            LOG_INFO(TAG, "Start listening BIND_CONFIRM for %dms", RadioCar::LISTENING_TIME_MS);

            radio_state_ = Role::RxMode;
            loop_state_ = CarLoopState::BIND_CONFIRM;
            listening_deadline_ms_ = HAL_GetTick() + RadioCar::LISTENING_TIME_MS;
            break;
        }
        case CarLoopState::IDLE:
        case CarLoopState::BIND_REQ:
        case CarLoopState::BIND_CONFIRM:
        case CarLoopState::BIND_ACK:
        case CarLoopState::PING_PONG:
        default:
        {
            break;
        }
    }

    return true;
}

uint32_t RadioCar::remaining_ms() const
{
    switch (loop_state_)
    {
        case CarLoopState::BIND_REQ:
        case CarLoopState::BIND_CONFIRM:
        {
            auto left = static_cast<int32_t>(listening_deadline_ms_ - HAL_GetTick());
            return (left > 0) ? static_cast<uint32_t>(left) : 0;
        }
        default:
        {
            return IRQ_WAIT_TIMEOUT_MS;
        }
    }
}
