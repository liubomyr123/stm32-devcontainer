#include "state_machine.hpp"

#include "uart_config.h"

StateMachine::StateMachine()
{
    driveParams_.type = CMD_STOP;
    previousDriveParams_.type = CMD_STOP;
}

StateMachine& StateMachine::instance()
{
    static StateMachine instance;
    return instance;
}

State StateMachine::getState() const
{
    return currentState_;
}

State StateMachine::getPreviousState() const
{
    return previousState_;
}

void StateMachine::updateState(const State newState)
{
    if (currentState_ == newState)
    {
        return;
    }

    LOG_INFO(TAG, "%s → %s", stateToString(currentState_), stateToString(newState));
    currentState_ = newState;
}

void StateMachine::updatePreviousState()
{
    previousState_ = currentState_;
    previousDriveParams_ = driveParams_;
}

const UartCmd& StateMachine::getDriveParams() const
{
    return driveParams_;
}

const UartCmd& StateMachine::getPreviousDriveParams() const
{
    return previousDriveParams_;
}

void StateMachine::setDriveParams(UartCmd cmd_)
{
    driveParams_ = cmd_;
}

const char* StateMachine::stateToString(State state)
{
    switch (state)
    {
        case State::IDLE:
            return "IDLE";
        case State::ARMED:
            return "ARMED";
        case State::DRIVING:
            return "DRIVING";
        default:
            return "UNKNOWN";
    }
}