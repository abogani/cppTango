#include "utils/auto_device_class.h"

std::vector<TangoTest::detail::ClassRegistrarBase *> *TangoTest::detail::ClassRegistrarBase::registrars = nullptr;

namespace
{
std::vector<std::string> get_enabled_classes()
{
    std::vector<std::string> result;

    const char *maybe_env = std::getenv(TangoTest::detail::k_enabled_classes_env_var);

    if(maybe_env == nullptr)
    {
        return result;
    }

    std::string env{maybe_env};

    size_t pos = 0;
    while(true)
    {
        size_t next = env.find(';', pos);
        if(next == std::string::npos)
        {
            result.push_back(env.substr(pos));
            break;
        }

        result.push_back(env.substr(pos, next - 1));
        pos = next + 1;
    }

    return result;
}

bool contains(const std::vector<std::string> &haystack, const std::string &needle)
{
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

} // namespace

namespace Tango
{
void DServer::class_factory()
{
    std::vector<std::string> enabled = get_enabled_classes();

    for(auto *registrar : *TangoTest::detail::ClassRegistrarBase::registrars)
    {
        if(!enabled.empty() && !contains(enabled, registrar->name))
        {
            continue;
        }

        add_class(registrar->init_class());
    }
}
} // namespace Tango
