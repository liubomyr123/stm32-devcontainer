#include "state_machine.hpp"

StateMachine& StateMachine::instance()
{
    static StateMachine instance;
    return instance;
}

State StateMachine::getState() const
{
    return currentState_;
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

const UartCmd& StateMachine::getDriveParams() const
{
    return driveParams_;
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