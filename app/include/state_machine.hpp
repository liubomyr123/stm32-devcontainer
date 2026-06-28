#ifndef STATE_MACHINE_HPP
#define STATE_MACHINE_HPP

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

   private:
    StateMachine() = default;
    State currentState_ = State::IDLE;
};

#endif
