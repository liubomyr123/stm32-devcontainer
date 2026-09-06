#include "mutex_guard.hpp"

MutexGuard::MutexGuard(osMutexId_t mutex) : mutex_(mutex)
{
    if (mutex_ != nullptr)
    {
        osMutexAcquire(mutex_, osWaitForever);
    }
}

MutexGuard::~MutexGuard()
{
    if (mutex_ != nullptr)
    {
        osMutexRelease(mutex_);
    }
}