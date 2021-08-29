#pragma once
#include <chrono>
#include <string>

namespace pt {

class ScopeClock {
   public:
    ScopeClock( const std::string& info );
    ~ScopeClock();

   private:
    std::chrono::system_clock::time_point m_begin;
    std::chrono::system_clock::time_point m_end;
    std::string m_info;
};

double GetMsSinceEpoch();

}  // namespace pt
