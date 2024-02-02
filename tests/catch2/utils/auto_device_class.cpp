#include "utils/auto_device_class.h"

std::vector<TangoTest::detail::ClassRegistrarBase *> *TangoTest::detail::ClassRegistrarBase::registrars = nullptr;

namespace Tango
{
void DServer::class_factory()
{
    for(auto *registrar : *TangoTest::detail::ClassRegistrarBase::registrars)
    {
        add_class(registrar->init_class());
    }
}
} // namespace Tango
