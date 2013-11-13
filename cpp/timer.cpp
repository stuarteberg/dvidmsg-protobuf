#include "Timer.h"
#include <boost/chrono.hpp>

//*********************************************
// Timer::TimerImpl
//*********************************************
class Timer::TimerImpl
{
public:
    TimerImpl()
    : _paused(true)
    , _total_time(0)
    {
    }

    void pause()
    {
        assert(!_paused && "Can't pause a timer that's already paused") ;
        boost::chrono::system_clock::time_point now = boost::chrono::system_clock::now() ;
        _total_time += ( now - _unpause_timestamp ) ;
        _paused = true ;
    }

    void unpause()
    {
        assert(_paused && "Can't unpause a timer that isn't already paused") ;
        _unpause_timestamp = boost::chrono::system_clock::now() ;
        _paused = false ;
    }

    float seconds() const
    {
        if (_paused)
        {
            return _total_time.count() ;
        }
        else
        {
            boost::chrono::duration<float> current_elapsed = _total_time + ( boost::chrono::system_clock::now() - _unpause_timestamp ) ;
            return current_elapsed.count() ;
        }
    }

private:
    bool _paused ;
    boost::chrono::duration<float> _total_time ;
    boost::chrono::system_clock::time_point _unpause_timestamp ;
};

//*********************************************
// Timer
//*********************************************
Timer::Timer()
: _pImpl( new TimerImpl() )
{
}

void Timer::pause()
{
    _pImpl->pause() ;
}

void Timer::unpause()
{
    _pImpl->unpause() ;
}

float Timer::seconds() const
{
    return _pImpl->seconds() ;
}

//*********************************************
// Timer::Token
//*********************************************
Timer::Token::Token( Timer & timer )
: _timer(timer)
{
    _timer.unpause() ;
}

Timer::Token::~Token()
{
    _timer.pause() ;
}

