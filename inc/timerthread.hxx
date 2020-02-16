/**
 * TimerThread class definition
 *
 * @file TimerThread.hxx
 */

#ifndef TIMERTHREAD_HXX
#define TIMERTHREAD_HXX

/* Includes -------------------------------------------- */
#include <functional>
#include <chrono>
#include <unordered_map>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <cstdint>

/* TimerThread class definition ------------------------ */
class TimerThread
{
    public:
        /* Defining the timer ID type */
        using timer_id_t = std::uint64_t;                     /* Each Timer is assigned a unique ID of type timer_id_t */
        static timer_id_t constexpr no_timer = timer_id_t(0); /* Valid IDs are guaranteed not to be this value */

        /* Defining the handler function type */
        using handler_type = std::function<void()>; // Function object we actually use
        // Function object that we boil down to handler_type with std::bind
        template<typename ... Args>
        using bound_handler_type = std::function<void(Args ...)>;

        /* Defining the microsecond type */
        using time_us_t = std::int64_t; /* Values that are a large-range microsecond count */

        /** @brief Constructor does not start worker until there is a Timer */
        explicit TimerThread();

        /** @brief Destructor is thread safe, even if a timer
         * callback is running. All callbacks are guaranteed
         * to have returned before this destructor returns
         */
        ~TimerThread();

        /** @brief Create timer using microseconds
         * The delay will be called msDelay microseconds from now
         * If msPeriod is nonzero, call the callback again every
         * msPeriod microseconds
         * All timer creation functions eventually call this one
         */
        timer_id_t addTimer(time_us_t    msDelay,
                            time_us_t    msPeriod,
                            handler_type handler);

        /** @brief Create timer using std::chrono delay and period
         * Optionally binds additional arguments to the callback
         */
        template<typename SRep, typename SPer,
                    typename PRep, typename PPer,
                    typename ... Args>
        timer_id_t addTimer(typename std::chrono::duration<SRep, SPer> const &delay,
                            typename std::chrono::duration<PRep, PPer> const &period,
                            bound_handler_type<Args ...> handler,
                            Args && ... args);

        /** @brief Create timer using millisecond units delay and period
         * Optionally binds additional arguments to the callback
         */
        template<typename ... Args>
        timer_id_t addTimer(time_us_t                    msDelay,
                            time_us_t                    msPeriod,
                            bound_handler_type<Args ...> handler,
                            Args && ...                  args);

        /** @brief setInterval API like browser javascript
         * Call handler every `period` milliseconds,
         * starting `period` milliseconds from now
         * Optionally binds additional arguments to the callback
         */
        timer_id_t setInterval(handler_type handler,
                                time_us_t    period);

        /** @brief setTimeout API like browser javascript
         * Call handler once `timeout` ms from now
         */
        timer_id_t setTimeout(handler_type handler,
                                time_us_t    timeout);

        /** @brief setInterval API like browser javascript
         * Call handler every `period` microseconds,
         * starting `period` microseconds from now
         */
        template<typename ... Args>
        timer_id_t setInterval(bound_handler_type<Args ...> handler,
                                time_us_t                    period,
                                Args && ...                  args);

        /** @brief setTimeout API like browser javascript
         * Call handler once `timeout` ms from now
         * binds extra arguments and passes them to the
         * timer callback
         */
        template<typename ... Args>
        timer_id_t setTimeout(bound_handler_type<Args ...> handler,
                                time_us_t                    timeout,
                                Args && ...                  args);

        /** @brief Destroy the specified timer
         *
         * Synchronizes with the worker thread if the
         * callback for this timer is running, which
         * guarantees that the handler for that callback
         * is not running before clearTimer returns
         *
         * You are not required to clear any timers. You can
         * forget their timer_id_t if you do not need to cancel
         * them.
         *
         * The only time you need this is when you want to
         * stop a timer that has a repetition period, or
         * you want to cancel a timeout that has not fired
         * yet
         *
         * See clear() to wipe out all timers in one go
         */
        bool clearTimer(timer_id_t id);

        /* @brief Destroy all timers, but preserve id uniqueness
         * This carefully makes sure every timer is not
         * executing its callback before destructing it
         */
        void clear();

        /* @brief Set the TimerThread's priority
         */
        int setScheduling(const int &pPolicy, const int &pPriority);

        /* @brief Get the TimerThread's priority
         */
        int scheduling(int * const pPolicy, int * const pPriority) noexcept;

        /* Peek at current state */
        std::size_t size() const noexcept;
        bool        empty() const noexcept;

        /** @brief Returns initialized singleton */
        static TimerThread &global();

    private:
        /* Type definitions */
        using Lock         = std::mutex;
        using ScopedLock   = std::unique_lock<Lock>;
        using ConditionVar = std::condition_variable;

        using Clock     = std::chrono::high_resolution_clock;
        using Timestamp = std::chrono::time_point<Clock>;
        using Duration  = std::chrono::microseconds; /* changed milliseconds to microseconds */

        /** @brief Timer structure definition */
        struct Timer {
            explicit Timer(timer_id_t id = 0U);
            Timer(Timer &&r) noexcept;
            Timer &operator=(Timer &&r) noexcept;

            Timer(timer_id_t   id,
                    Timestamp    next,
                    Duration     period,
                    handler_type handler) noexcept;

            // Never called
            Timer(Timer const &r)            = delete;
            Timer &operator=(Timer const &r) = delete;

            timer_id_t   id;
            Timestamp    next;
            Duration     period;
            handler_type handler;

            // You must be holding the 'sync' lock to assign waitCond
            std::unique_ptr<ConditionVar> waitCond;

            bool running;
        };

        // Comparison functor to sort the timer "queue" by Timer::next
        struct NextActiveComparator {
            bool operator()(Timer const &a, Timer const &b) const noexcept
            {
                return a.next < b.next;
            }
        };

        // Queue is a set of references to Timer objects, sorted by next
        using QueueValue = std::reference_wrapper<Timer>;
        using Queue      = std::multiset<QueueValue, NextActiveComparator>;
        using TimerMap   = std::unordered_map<timer_id_t, Timer>;

        void timerThreadWorker();
        bool destroy_impl(ScopedLock        &lock,
                            TimerMap::iterator i,
                            bool               notify);

        // Inexhaustible source of unique IDs
        timer_id_t nextId;

        // The Timer objects are physically stored in this map
        TimerMap active;

        // The ordering queue holds references to items in `active`
        Queue queue;

        // One worker thread for an unlimited number of timers is acceptable
        // Lazily started when first timer is started
        // TODO: Implement auto-stopping the timer thread when it is idle for
        // a configurable period.
        mutable Lock sync;
        ConditionVar wakeUp;
        std::thread worker;
        bool done;
};

/* Template implementation fo class methods */
template<typename SRep, typename SPer,
            typename PRep, typename PPer,
            typename ... Args>
TimerThread::timer_id_t TimerThread::addTimer(typename std::chrono::duration<SRep, SPer> const &delay,
                                                typename std::chrono::duration<PRep, PPer> const &period,
                                                bound_handler_type<Args ...> handler,
                                                Args && ... args)
{
    time_us_t msDelay
        = std::chrono::duration_cast<std::chrono::microseconds>(delay).count();

    time_us_t msPeriod
        = std::chrono::duration_cast<std::chrono::microseconds>(period).count();

    return addTimer(msDelay, msPeriod,
                    std::move(handler),
                    std::forward<Args>(args) ...);
}

template<typename ... Args>
TimerThread::timer_id_t TimerThread::addTimer(time_us_t                    msDelay,
                                                time_us_t                    msPeriod,
                                                bound_handler_type<Args ...> handler,
                                                Args && ...                  args)
{
    return addTimer(msDelay, msPeriod,
                    std::bind(std::move(handler),
                                std::forward<Args>(args) ...));
}

// Javascript-like setInterval
template<typename ... Args>
TimerThread::timer_id_t TimerThread::setInterval(bound_handler_type<Args ...> handler,
                                                    time_us_t                    period,
                                                    Args && ...                  args)
{
    return setInterval(std::bind(std::move(handler),
                                    std::forward<Args>(args) ...),
                        period);
}

// Javascript-like setTimeout
template<typename ... Args>
TimerThread::timer_id_t TimerThread::setTimeout(bound_handler_type<Args ...> handler,
                                                time_us_t                    timeout,
                                                Args && ...                  args)
{
    return setTimeout(std::bind(std::move(handler),
                                std::forward<Args>(args) ...),
                        timeout);
}

#endif /* TIMERTHREAD_HXX */
