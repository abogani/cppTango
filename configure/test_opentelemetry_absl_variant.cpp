#include <opentelemetry/std/variant.h>

#include <type_traits>

int main(int, char **)
{
    static_assert(std::is_same_v<opentelemetry::nostd::variant<int>, std::variant<int>>);
}
