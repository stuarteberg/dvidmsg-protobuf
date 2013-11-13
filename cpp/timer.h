#ifndef TIMER_H_20131111
#define TIMER_H_20131111

#include <boost/shared_ptr.hpp>

class Timer
{
public:
    Timer() ;

    // RAII pause/unpause
    class Token ;

    // Query total elapsed time spent in "unpaused" state.
    float seconds() const ;

    // "Manual" pause/unpause
    void pause() ;
    void unpause() ;

private:
    // Hidden implementation
    class TimerImpl ;
    boost::shared_ptr<TimerImpl> _pImpl ;
};

// Helper class: RAII pause/unpause
class Timer::Token
{
public:
    Token( Timer & timer ) ;
    ~Token() ;
private:
    Timer & _timer ;
};

#endif // TIMER_H_20131111
