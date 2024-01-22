#ifndef TANGO_TESTS_CATCH2_UTILS_AUTO_DEVICE_CLASS_H
#define TANGO_TESTS_CATCH2_UTILS_AUTO_DEVICE_CLASS_H

#include <tango/tango.h>

#include <functional>
#include <type_traits>

namespace TangoTest
{

namespace detail
{
template <typename T, typename = std::void_t<>>
struct has_attribute_factory : std::false_type
{
};

template <typename T>
struct has_attribute_factory<T, std::void_t<decltype(T::attribute_factory)>> : std::true_type
{
};

template <typename T>
constexpr bool has_attribute_factory_v = has_attribute_factory<T>::value;

template <typename T, typename = std::void_t<>>
struct has_command_factory : std::false_type
{
};

template <typename T>
struct has_command_factory<T, std::void_t<decltype(T::command_factory)>> : std::true_type
{
};

template <typename T>
constexpr bool has_command_factory_v = has_command_factory<T>::value;

template <typename F>
struct member_fn_traits;

template <typename C, typename R, typename... Args>
struct member_fn_traits<R (C::*)(Args... args)>
{
    using class_type = C;
    using return_type = R;
    constexpr static int arity = std::tuple_size<std::tuple<Args...>>();
    template <int N>
    using argument_type = std::tuple_element_t<N, std::tuple<Args...>>;
};
} // namespace detail

// TODO: Support commands

//+ Class template to automatically generate a Tango::DeviceClass from a
/// Tango::Device.
///
/// The template expects the following static member functions to be defined:
///
///   - static void attribute_factory(std::vector<Tango::Attr *> attrs);
///
/// Use the TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE macro (in a single
/// implementation file per Device) to instantiate the static members
/// AutoDeviceClass and to register the device class with Tango.
///
/// Example:
///
///  class MyDevice : public Tango::Device
///  {
///      static void attribute_factory(std::vector<Tango::Attr *> attrs)
///      {
///          // ... Add attributes here
///      }
///  };
///
///  TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(MyDevice)
template <typename Device>
class AutoDeviceClass : public Tango::DeviceClass
{
  public:
    using Tango::DeviceClass::DeviceClass;

    static AutoDeviceClass *init(const char *s)
    {
        if(!_instance)
        {
            _instance = new AutoDeviceClass{s};
        }

        return _instance;
    }

    ~AutoDeviceClass() override { }

    void device_factory(const Tango::DevVarStringArray *devlist_ptr) override
    {
        auto *tg = Tango::Util::instance();

        for(CORBA::ULong i = 0; i < devlist_ptr->length(); ++i)
        {
            const char *name = (*devlist_ptr)[i];

            // "NoName" means no device with this class was specified
            // on the CLI (in nodb mode).  We do not create this device as it is
            // not needed for the test.
            if(strcmp(name, "NoName") == 0)
            {
                continue;
            }

            device_list.push_back(new Device{this, name});

            if(tg->use_db() && !tg->use_file_db())
            {
                export_device(device_list.back());
            }
            else
            {
                export_device(device_list.back(), name);
            }
        }
    }

  protected:
    void command_factory() override
    {
        if constexpr(detail::has_command_factory_v<Device>)
        {
            Device::command_factory(command_list);
        }
    }

    void attribute_factory(std::vector<Tango::Attr *> &attrs) override
    {
        if constexpr(detail::has_attribute_factory_v<Device>)
        {
            Device::attribute_factory(attrs);
        }
    }

    static AutoDeviceClass *_instance;
};

// TODO: AutoCommand ?
template <auto cmd_fn>
class AutoCommand : public Tango::Command
{
  public:
    using traits = detail::member_fn_traits<decltype(cmd_fn)>;
    using Device = typename traits::class_type;
    using Tango::Command::Command;

    AutoCommand(const std::string &name) :
        Tango::Command(name, type_in(), type_out())
    {
    }

    ~AutoCommand() override { }

    CORBA::Any *execute(Tango::DeviceImpl *dev, const CORBA::Any &in_any) override
    {
        CORBA::Any *out_any = new CORBA::Any();
        if constexpr(traits::arity == 0 && std::is_same_v<void, typename traits::return_type>)
        {
            std::invoke(cmd_fn, static_cast<Device *>(dev));
        }
        else if constexpr(traits::arity == 0)
        {
            auto ret = std::invoke(cmd_fn, static_cast<Device *>(dev));
            *out_any <<= ret;
        }
        else if constexpr(std::is_same_v<void, typename traits::return_type>)
        {
            typename traits::argument_type<0> arg;
            in_any >>= arg;
            std::invoke(cmd_fn, static_cast<Device *>(dev), arg);
        }
        else
        {
            typename traits::argument_type<0> arg;
            in_any >>= arg;
            auto ret = std::invoke(cmd_fn, static_cast<Device *>(dev), arg);
            *out_any <<= ret;
        }
        return out_any;
    }

  private:
    constexpr static Tango::CmdArgType type_out()
    {
        if constexpr(std::is_same_v<void, typename traits::return_type>)
        {
            return Tango::DEV_VOID;
        }
        else
        {
            return Tango::tango_type_traits<typename traits::return_type>::type_value();
        }
    }

    constexpr static Tango::CmdArgType type_in()
    {
        if constexpr(traits::arity == 0)
        {
            return Tango::DEV_VOID;
        }
        else
        {
            return Tango::tango_type_traits<typename traits::template argument_type<0>>::type_value();
        }
    }
};

// TODO: Add write function
template <auto read_fn>
class AutoAttr : public Tango::Attr
{
  public:
    using Device = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using Tango::Attr::Attr;

    ~AutoAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<Device *>(dev), att);
    }
};

namespace detail
{
struct ClassRegistrarBase
{
    ClassRegistrarBase()
    {
        if(!registrars)
        {
            registrars = new std::vector<ClassRegistrarBase *>();
        }

        registrars->push_back(this);
    }

    virtual Tango::DeviceClass *init_class() = 0;

    // We use a pointer here so that the first `ClassRegistrarBase` constructs
    // the vector.  This avoids the static initialisation order fiasco.
    static std::vector<ClassRegistrarBase *> *registrars;
};

template <typename DeviceClass>
struct ClassRegistrar : ClassRegistrarBase
{
    ClassRegistrar(const char *n) :
        name{n}
    {
    }

    Tango::DeviceClass *init_class() override
    {
        return DeviceClass::init(name);
    }

    const char *name;
};
} // namespace detail

} // namespace TangoTest

//+ Instantiate a TangoTest::AutoDeviceClass for DEVICE.
///
/// For each DEVICE, this macro must be used in a single implementation file to
/// instantiate static data members.
///
/// The device class will be registered with Tango with the name #DEVICE.
#define TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE, NAME)                                        \
    template <>                                                                                    \
    TangoTest::AutoDeviceClass<DEVICE> *TangoTest::AutoDeviceClass<DEVICE>::_instance = nullptr;   \
    namespace                                                                                      \
    {                                                                                              \
    TangoTest::detail::ClassRegistrar<TangoTest::AutoDeviceClass<DEVICE>> NAME##_registrar{#NAME}; \
    }

#endif
