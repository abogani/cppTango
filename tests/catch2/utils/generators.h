#ifndef TANGO_TESTS_CATCH2_UTILS_GENERATORS_H
#define TANGO_TESTS_CATCH2_UTILS_GENERATORS_H

#include "utils/options.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>

#include <tango/tango.h>

namespace TangoTest
{

class IdlVersionGenerator : public Catch::Generators::IGenerator<int>
{
  public:
    IdlVersionGenerator(int min, int max) :
        m_max{max},
        m_curr{min}
    {
        if(g_options.only_idl_version)
        {
            int only = *g_options.only_idl_version;

            if(only >= min && only <= max)
            {
                m_curr = only;
                m_max = only;
            }
            else
            {
                SKIP("no idl version selected from range [" << min << "," << max << "] with idl-only=" << only);
            }
        }
    }

    int const &get() const override
    {
        return m_curr;
    }

    bool next() override
    {
        m_curr += 1;
        return m_curr <= m_max;
    }

  private:
    int m_max;
    int m_curr;
};

inline Catch::Generators::GeneratorWrapper<int> idlversion(int min, int max = Tango::DevVersion)
{
    return Catch::Generators::GeneratorWrapper<int>{new IdlVersionGenerator{min, max}};
}

} // namespace TangoTest

#endif
