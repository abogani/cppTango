// NOLINTBEGIN(*)

#ifndef Common_H
  #define Common_H

  #include <iostream>
  #include <tango/tango.h>

  #ifdef WIN32
    #include <process.h>
  #else
    #include <unistd.h>
  #endif

  #include "logging.h"

  #include <mutex>
  #include <condition_variable>
  #include <chrono>
  #include <charconv>
  #include <sstream>

// We have a struct here so that if parse_as is used with an unsupported type it
// will produce an error.
template <typename T>
struct type_name_impl;

template <typename T>
constexpr const char *type_name = type_name_impl<T>::value;

  #define SPECIALIZE_TYPE_NAME(t)                    \
        template <>                                  \
        struct type_name_impl<t>                     \
        {                                            \
            constexpr static const char *value = #t; \
        }

// Add as required
SPECIALIZE_TYPE_NAME(int);
SPECIALIZE_TYPE_NAME(long);
SPECIALIZE_TYPE_NAME(double);
SPECIALIZE_TYPE_NAME(size_t);

  #undef SPECIALIZE_TYPE_NAME

/**
 * Parse a string as the given type.
 *
 * Throws a std::runtime_error if the string cannot be entirely parsed to the type T
 */
template <typename T>
T parse_as(const std::string &str)
{
    T result{};
    auto first = str.data();
    auto last = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(first, last, result);

    if(ec != std::errc{} || ptr != last)
    {
        std::stringstream ss;
        ss << "\"" << str << "\" cannot be entirely parsed into " << type_name<T>;
        throw std::runtime_error{ss.str()};
    }

    return result;
}

  #ifndef TANGO_HAS_FROM_CHARS_DOUBLE
template <>
double parse_as<double>(const std::string &str);
  #endif

/**
 * Wrapper for unsetenv working on windows systems as well.
 * Unset environment variable var
 */
auto unset_env(const std::string &var) -> int;

/**
 * Wrapper for setenv working on windows systems as well.
 * Set environment variable var to value value.
 * Force update if force_update flag is set to true (not used on windows)
 */
auto set_env(const std::string &var, const std::string &value, bool force_update) -> int;

/**
 * Load the ginve file as binary from disc and return it's contents as std::string
 */
std::string load_file(const std::string &file);

/**
 * Counts how many times the overload `push_event(TEvent*)` is called.
 *
 * Access to members is synchronised with a mutex so the class is thread safe,
 * as required by the `Tango::CallBack` base class.
 *
 * A user can inherit from `CountingCallBack` and override
 * `process_event(TEvent*)` to add additional behaviour.  Access to any members
 * added by the derived class must also be synchronised by the mutex.
 * `process_event(TEvent*)` is only called when the mutex is held and `lock()`
 * can be used to provide public accessors for clients to gets copies of
 * additional members.
 */
template <typename TEvent>
class CountingCallBack : public Tango::CallBack
{
  public:
    CountingCallBack() = default;
    CountingCallBack(const CountingCallBack &) = delete;
    CountingCallBack &operator=(const CountingCallBack &) = delete;
    ~CountingCallBack() override = default;

    // Handles events from the tango kernel
    void push_event(TEvent *event) override
    {
        {
            auto guard = lock();

            ++m_invocation_count;
            if(process_event(event))
            {
                ++m_error_count;
            }
        }

        m_cv.notify_one();
    }

    /** Resets the `invocation_count` and `error_count` to zero.
     */
    void reset_counts()
    {
        auto guard = lock();

        m_error_count = 0;
        m_invocation_count = 0;
    }

    /**
     * Block current executing thread until `timeout` seconds have passed or
     * until `should_stop()` returns `true`.
     *
     * Returns `true` if `should_stop` returned `true` or
     * `false` if `timeout` was exceeded.
     */
    template <typename Predicate>
    bool wait_for(Predicate &&should_stop, std::chrono::seconds timeout = std::chrono::seconds(120))
    {
        auto guard = lock();
        return m_cv.wait_for(guard, timeout, std::forward<Predicate>(should_stop));
    }

    /**
     * Returns the number of times `push_event(TEvent*)` has been called since the
     * object was constructed or the most recent call to `reset()`, whichever
     * was later.
     */
    int invocation_count()
    {
        auto guard = lock();
        return m_invocation_count;
    }

    /**
     * Returns the number of times `push_event(TEvent*)` has been called with an
     * error event since the object was constructed or the most recent call
     * to `reset()`, whichever was later.
     *
     * Derived classes can redefine which events count as an error by overriding
     * `process_event(TEvent*)`.  By default an event is considered an error if
     * `event->err` is truthy.
     */
    int error_count()
    {
        auto guard = lock();
        return m_error_count;
    }

    /**
     * Returns the number of times `push_event(TEvent*)` has been called since
     * the object was constructed or the most recent call to `reset()`,
     * whichever is later.
     *
     * This is equivalent to `invocation_count() - error_count()` but the value
     * is computed atomically.
     */
    int success_count()
    {
        auto guard = lock();
        return m_invocation_count - m_error_count;
    }

  protected:
    /**
     * Lock this object's mutex.  To be used by derived classes to implement
     * accessors to any additional members.
     */
    std::unique_lock<std::recursive_mutex> lock()
    {
        return std::unique_lock<std::recursive_mutex>{m_mutex};
    }

  private:
    /**
     * Called by `push_event(TEvent*)` when `m_mutex` is held.
     *
     * Returns true if the event should be counted as an error event.
     */
    virtual bool process_event(TEvent *event)
    {
        return event->err;
    }

    // Number of times `push_event` has been called since the object was
    // constructed or the most recent call to `reset()`, whichever was later.
    int m_invocation_count = 0;

    // Number of times `push_event` has been called with an error event
    // since the object was constructed or the most recent call to `reset()`,
    // whichever was later.
    int m_error_count = 0;

    // Must be held to access any members
    std::recursive_mutex m_mutex;
    // Notified when `push_event` has been called
    std::condition_variable_any m_cv;
};

#endif // Common_H

// NOLINTEND(*)
