#ifndef TANGO_INTERNAL_PERF_MON_H
#define TANGO_INTERNAL_PERF_MON_H

#include <mutex>
#include <ostream>
#include <chrono>
#include <ratio>

namespace Tango
{
template <typename T>
struct RingBuffer
{
    static constexpr const size_t size = 256;
    T buffer[size];
    size_t index = 0;
    bool first_pass = true;

    void reset()
    {
        index = 0;
        first_pass = true;
    }

    void push(const T &v)
    {
        buffer[index] = v;
        index = (index + 1) % size;
        if(index == 0)
        {
            first_pass = false;
        }
    }

    void json_dump(std::ostream &os)
    {
        os << "[";
        bool first = true;
        auto print_elem = [&first, &os, this](size_t i)
        {
            if(!first)
            {
                os << ",";
            }

            buffer[i].json_dump(os);

            first = false;
        };

        if(!first_pass)
        {
            for(size_t i = index; i < size; ++i)
            {
                print_elem(i);
            }
        }

        for(size_t i = 0; i < index; ++i)
        {
            print_elem(i);
        }
        os << "]";
    }
};

using PerfClock = std::chrono::steady_clock;

static constexpr const std::int64_t k_invalid_duration = std::numeric_limits<std::int64_t>::min();

inline std::int64_t duration_micros(PerfClock::time_point start, PerfClock::time_point end)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

template <typename T>
struct SamplePusher
{
    SamplePusher(bool enabled, T &sample, RingBuffer<T> &buffer, std::mutex &mutex) :
        enabled(enabled),
        sample(sample),
        buffer(buffer),
        mutex(mutex)
    {
    }

    ~SamplePusher()
    {
        if(enabled)
        {
            if(mutex.try_lock())
            {
                buffer.push(sample);
                mutex.unlock();
            }

            sample = T{};
        }
    }

    bool enabled;
    T &sample;
    RingBuffer<T> &buffer;
    std::mutex &mutex;
};

struct TimeBlockMicros
{
    TimeBlockMicros() :
        TimeBlockMicros(false, nullptr)
    {
    }

    TimeBlockMicros(bool enabled, std::int64_t *slot) :
        enabled(enabled),
        slot(slot)
    {
        if(enabled)
        {
            start = PerfClock::now();
        }
    }

    TimeBlockMicros(const TimeBlockMicros &) = delete;
    TimeBlockMicros operator=(const TimeBlockMicros &) = delete;

    TimeBlockMicros &operator=(TimeBlockMicros &&other) noexcept
    {
        enabled = other.enabled;
        slot = other.slot;
        start = other.start;

        other.enabled = false;
        other.slot = nullptr;

        return *this;
    }

    TimeBlockMicros(TimeBlockMicros &&other) noexcept :
        enabled(other.enabled),
        start(other.start),
        slot(other.slot)
    {
        other.enabled = false;
        other.slot = nullptr;
    }

    ~TimeBlockMicros()
    {
        if(enabled)
        {
            PerfClock::time_point end = PerfClock::now();
            *slot += duration_micros(start, end);
        }
    }

    bool enabled;
    PerfClock::time_point start;
    std::int64_t *slot;
};

template <typename T>
struct DoubleBuffer
{
    void json_dump(std::ostream &os)
    {
        RingBuffer<T> *buffer = nullptr;
        {
            std::lock_guard<std::mutex> lg{lock};
            if(enabled)
            {
                std::swap(front, back);
                front->reset();
                buffer = back;
            }
        }

        if(buffer == nullptr)
        {
            os << "null";
        }
        else
        {
            buffer->json_dump(os);
        }
    }

    void enable(bool v)
    {
        std::lock_guard<std::mutex> lg{lock};
        enabled = v;
        if(v)
        {
            front->reset();
        }
    }

    std::mutex lock;
    RingBuffer<T> ring_buffers[2];
    RingBuffer<T> *front = &ring_buffers[0];
    RingBuffer<T> *back = &ring_buffers[1];
    bool enabled = false;
};
} // namespace Tango

#endif
