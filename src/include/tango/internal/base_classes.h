#ifndef TANGO_NON_COPYABLE_H
#define TANGO_NON_COPYABLE_H

namespace Tango::detail
{
class NonCopyable
{
  protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};
} // namespace Tango::detail

#endif // TANGO_NON_COPYABLE_H
