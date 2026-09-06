#include "cmsis_os2.h"

class MutexGuard
{
   public:
    explicit MutexGuard(osMutexId_t mutex);

    ~MutexGuard();
    MutexGuard(const MutexGuard&) = delete;
    MutexGuard& operator=(const MutexGuard&) = delete;
    MutexGuard(MutexGuard&&) = delete;
    MutexGuard& operator=(MutexGuard&&) = delete;

   private:
    osMutexId_t mutex_;
};