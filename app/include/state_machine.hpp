#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

#include "include/uart_config.h"

enum class State
{
    IDLE,
    ARMED,
    DRIVING
};

struct DriveParams
{
    UartCmdType direction = CMD_STOP;
    uint8_t speed = 0;
};

class StateMachine
{
   public:
    static StateMachine& instance();
    State getState() const;
    void updateState(const State);

    const DriveParams& getDriveParams() const;
    void setDriveParams(UartCmdType direction, uint8_t speed);
    const char* stateToString(State state);

   private:
    StateMachine() = default;
    State currentState_ = State::IDLE;
    DriveParams driveParams_;
};

#endif
