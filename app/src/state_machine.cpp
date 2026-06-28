#include "state_machine.hpp"

#include "include/logger.hpp"

StateMachine& StateMachine::instance()
{
    static StateMachine instance;
    return instance;
}

State StateMachine::getState() const
{
    return currentState_;
}
