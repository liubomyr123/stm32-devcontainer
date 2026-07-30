#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "include/logger.hpp"
#include "include/uart_config.h"

enum class State
{
    IDLE,
    ARMED,
    DRIVING
};

class StateMachine
{
   public:
    static StateMachine& instance();
    State getState() const;
    void updateState(const State newState);

    const UartCmd& getDriveParams() const;
    void setDriveParams(UartCmd cmd_);
    static const char* stateToString(State state);

   private:
    static constexpr const char* TAG = "SM";
    StateMachine() = default;
    State currentState_ = State::IDLE;
    UartCmd driveParams_{};
};

#endif
