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
    State getPreviousState() const;
    void updateState(const State newState);
    void updatePreviousState();

    const UartCmd& getDriveParams() const;
    const UartCmd& getPreviousDriveParams() const;
    void setDriveParams(UartCmd cmd_);
    static const char* stateToString(State state);

   private:
    static constexpr const char* TAG = "SM";
    StateMachine();
    State currentState_ = State::IDLE;
    State previousState_ = State::IDLE;
    UartCmd driveParams_{};
    UartCmd previousDriveParams_{};
};

#endif
