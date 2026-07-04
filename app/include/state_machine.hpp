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
    int8_t throttle = 0;  // -100..+100
    int8_t steering = 0;  // -100..+100
};

class StateMachine
{
   public:
    static StateMachine& instance();
    State getState() const;
    void updateState(const State newState);

    const DriveParams& getDriveParams() const;
    void setDriveParams(UartCmdType direction, int8_t throttle, int8_t steering);
    static const char* stateToString(State state);

   private:
    StateMachine() = default;
    State currentState_ = State::IDLE;
    DriveParams driveParams_;
};

#endif
