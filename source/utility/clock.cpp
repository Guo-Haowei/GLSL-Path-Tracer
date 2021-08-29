#include "clock.h"

#include <ctime>

// #include "config.h"

namespace pt {

using namespace std::chrono;
using std::ctime;
using std::string;
using std::time_t;

static void GetHourMinSecond( int& h, int& m, int& s )
{
    h         = 0;
    m         = 0;
    int total = s;
    s         = s % 60;
    total     = ( total - s ) / 60;
    m         = total % 60;
    h         = ( total - m ) / 60;
}

ScopeClock::ScopeClock( const string& info )
{
    m_info  = info;
    m_begin = std::chrono::system_clock::now();
}

ScopeClock::~ScopeClock()
{
    m_end = std::chrono::system_clock::now();

    time_t begin             = system_clock::to_time_t( m_begin );
    time_t end               = system_clock::to_time_t( m_end );
    duration<double> seconds = m_end - m_begin;
    printf( "------------------------------------------------------------------\n" );
    printf( "%s\n", m_info.c_str() );
    printf( "  start time    : %s", ctime( &begin ) );
    printf( "  end time      : %s", ctime( &end ) );
    int h, m, s = static_cast<int>( seconds.count() );
    GetHourMinSecond( h, m, s );
    printf( "  elapsed time  : %02d:%02d:%02d\n", h, m, s );
    printf( "------------------------------------------------------------------\n" );
    fflush( stdout );
}

double GetMsSinceEpoch()
{
    return duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
}

}  // namespace pt
