#ifndef TANGO_TESTS_BDD_UTILS_AUTO_DEVICE_CLASS_H
#define TANGO_TESTS_BDD_UTILS_AUTO_DEVICE_CLASS_H

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

template <typename F>
struct member_fn_class;

template <typename C, typename R, typename... Args>
struct member_fn_class<R (C::*)(Args... args)>
{
    using type = C;
};

template <typename F>
using member_fn_class_t = typename member_fn_class<F>::type;
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
    void command_factory() override { }

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

// TODO: Add write function
template <auto read_fn>
class AutoAttr : public Tango::Attr
{
  public:
    using Device = detail::member_fn_class_t<decltype(read_fn)>;
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
