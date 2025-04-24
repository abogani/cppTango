#pragma once

#include <tango/server/readers_writers_lock.h>

namespace Tango
{

template <typename T>
class PointerWithLock
{
  public:
    PointerWithLock(T *ptr, ReadersWritersLock &lock) :
        m_ptr{ptr},
        m_guard{lock}
    {
    }

    PointerWithLock(PointerWithLock &) = delete;
    PointerWithLock(PointerWithLock &&) = delete;
    PointerWithLock &operator=(const PointerWithLock &) = delete;
    PointerWithLock &operator=(PointerWithLock &&) = delete;

    T &operator*() const noexcept
    {
        return *m_ptr;
    }

    T *operator->() const noexcept
    {
        return m_ptr;
    }

  private:
    T *m_ptr;
    ReaderLock m_guard;

    template <typename U>
    friend class PointerWithLock;

    template <typename U>
    friend bool operator==(const PointerWithLock<U> &s, std::nullptr_t) noexcept;

    template <typename U>
    friend bool operator!=(const PointerWithLock<U> &s, std::nullptr_t) noexcept;
};

template <typename T>
bool operator==(const PointerWithLock<T> &s, std::nullptr_t) noexcept
{
    return s.m_ptr == nullptr;
}

template <typename T>
bool operator!=(const PointerWithLock<T> &s, std::nullptr_t) noexcept
{
    return s.m_ptr != nullptr;
}
} // namespace Tango
