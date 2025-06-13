#ifndef TANGO_TESTS_CATCH2_UTILS_AUTO_DEVICE_CLASS_H
#define TANGO_TESTS_CATCH2_UTILS_AUTO_DEVICE_CLASS_H

#include <tango/tango.h>

#include <functional>
#include <memory>
#include <type_traits>

#include <tango/internal/type_traits.h>

namespace TangoTest
{

namespace detail
{
template <typename T>
using attribute_factory_t = decltype(T::attribute_factory);

template <typename T>
constexpr bool has_attribute_factory = Tango::detail::is_detected_v<attribute_factory_t, T>;

template <typename T>
using command_factory_t = decltype(T::command_factory);

template <typename T>
constexpr bool has_command_factory = Tango::detail::is_detected_v<command_factory_t, T>;

template <typename F>
struct member_fn_traits;

template <>
struct member_fn_traits<std::nullptr_t>
{
    using class_type = void;
};

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

/** @brief Automatically generate a Tango::DeviceClass from a Tango::Device.
 *
 * If the following static member functions are defined then they will be called
 * during device instantiation:
 *
 *   - static void attribute_factory(std::vector<Tango::Attr *> &attrs);
 *   - static void command_factory(std::vector<Tango::Command *> &cmds)
 *
 * Use the TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE macro (in a single
 * implementation file per Device) to instantiate AutoDeviceClass's static
 * members and to register the device class with Tango.
 *
 * The `init_device` method is called automatically just after the constructor.
 * For internal reasons the `delete_device` method, if present, must be called
 * explicitly by the test code.
 *
 * Example:
 *
 *  class MyDevice : public Tango::Device
 *  {
 *    void read_attr(Tango::Attribute &att)
 *    {
 *        att.set_value(&value);
 *    }
 *
 *    void write_attr(Tango::Attribute &att)
 *    {
 *        att.get_write_value(value);
 *    }
 *
 *    static void attribute_factory(std::vector<Tango::Attr *> &attrs)
 *    {
 *        // From IDLv3 onwards the naming of the read and write functions can be
 *        // freely choosen, only for IDL v1/v2 the name of `read_attr` is hardcoded.
 *        // For writeable attributes there is no write function involved for IDL v1/v2,
 *        // see catch2_attr_read_write_simple.cpp for how this is done
 *        attrs.push_back(new TangoTest::AutoAttr<&MyDevice::read_attr, &AutoAttrDev::write_attr>("value",
 * Tango::DEV_DOUBLE));
 *        // ... Add more attributes
 *    }
 *  }
 *
 *
 *  TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(MyDevice)
 *
 * @tparam Device C++ class for Tango device
 * @param s name of device class
 */
template <typename Device>
class AutoDeviceClass : public Tango::DeviceClass
{
  public:
    using Tango::DeviceClass::DeviceClass;

    static AutoDeviceClass *init(const char *s)
    {
        if(_instance == nullptr)
        {
            _instance = new AutoDeviceClass{s};
        }

        return _instance;
    }

    ~AutoDeviceClass() override
    {
        _instance = nullptr;
    }

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

            auto dev = std::make_unique<Device>(this, name);
            dev->init_device();

            device_list.push_back(dev.release());

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
        if constexpr(detail::has_command_factory<Device>)
        {
            Device::command_factory(command_list);
        }
    }

    void attribute_factory(std::vector<Tango::Attr *> &attrs) override
    {
        if constexpr(detail::has_attribute_factory<Device>)
        {
            Device::attribute_factory(attrs);
        }
    }

    static AutoDeviceClass *_instance;
};

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
        if constexpr(traits::arity == 0 && std::is_same_v<void, typename traits::return_type>)
        {
            std::invoke(cmd_fn, static_cast<Device *>(dev));
            return insert();
        }
        else if constexpr(traits::arity == 0)
        {
            auto ret = std::invoke(cmd_fn, static_cast<Device *>(dev));
            return insert(ret);
        }
        else if constexpr(std::is_same_v<void, typename traits::return_type>)
        {
            using arg_type = std::remove_cv_t<std::remove_reference_t<typename traits::template argument_type<0>>>;
            if constexpr(std::is_same_v<arg_type, typename Tango::tango_type_traits<arg_type>::ArrayType>)
            {
                const arg_type *arg;
                extract(in_any, arg);
                std::invoke(cmd_fn, static_cast<Device *>(dev), *arg);
            }
            else
            {
                arg_type arg;
                extract(in_any, arg);
                std::invoke(cmd_fn, static_cast<Device *>(dev), arg);
            }
            return insert();
        }
        else
        {
            using arg_type = std::remove_cv_t<std::remove_reference_t<typename traits::template argument_type<0>>>;
            auto ret = [this, &dev, &in_any]()
            {
                if constexpr(std::is_same_v<arg_type, typename Tango::tango_type_traits<arg_type>::ArrayType>)
                {
                    const arg_type *arg;
                    extract(in_any, arg);
                    return std::invoke(cmd_fn, static_cast<Device *>(dev), *arg);
                }
                else
                {
                    arg_type arg;
                    extract(in_any, arg);
                    return std::invoke(cmd_fn, static_cast<Device *>(dev), arg);
                }
            }();

            return insert(ret);
        }
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
            using ret_type = std::remove_cv_t<std::remove_reference_t<typename traits::return_type>>;
            return Tango::tango_type_traits<ret_type>::type_value();
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
            using arg_type = std::remove_cv_t<std::remove_reference_t<typename traits::template argument_type<0>>>;
            return Tango::tango_type_traits<arg_type>::type_value();
        }
    }
};

template <auto read_fn, auto write_fn = nullptr>
class AutoAttr : public Tango::Attr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::Attr::Attr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoAttr(const char *name, long data_type) :
        Tango::Attr(name, data_type, has_write_fn ? Tango::READ_WRITE : Tango::READ)
    {
    }

    ~AutoAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }
};

template <typename EnumType, auto read_fn, auto write_fn = nullptr>
class AutoEnumAttr : public Tango::Attr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::Attr::Attr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoEnumAttr(const char *name) :
        Tango::Attr(name, Tango::DEV_ENUM, has_write_fn ? Tango::READ_WRITE : Tango::READ)
    {
    }

    ~AutoEnumAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }

    bool same_type(const std::type_info &in_type) override
    {
        return typeid(EnumType) == in_type;
    }

    std::string get_enum_type() override
    {
        return typeid(EnumType).name();
    }
};

template <auto read_fn, auto write_fn = nullptr>
class AutoSpectrumAttr : public Tango::SpectrumAttr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::SpectrumAttr::SpectrumAttr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoSpectrumAttr(const char *name, long data_type, long max_x) :
        Tango::SpectrumAttr(name, data_type, has_write_fn ? Tango::READ_WRITE : Tango::READ, max_x)
    {
    }

    ~AutoSpectrumAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }
};

template <typename EnumType, auto read_fn, auto write_fn = nullptr>
class AutoEnumSpectrumAttr : public Tango::SpectrumAttr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::SpectrumAttr::SpectrumAttr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoEnumSpectrumAttr(const char *name, long max_x) :
        Tango::SpectrumAttr(name, Tango::DEV_ENUM, has_write_fn ? Tango::READ_WRITE : Tango::READ, max_x)
    {
    }

    ~AutoEnumSpectrumAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }

    bool same_type(const std::type_info &in_type) override
    {
        return typeid(EnumType) == in_type;
    }

    std::string get_enum_type() override
    {
        return typeid(EnumType).name();
    }
};

template <auto read_fn, auto write_fn = nullptr>
class AutoImageAttr : public Tango::ImageAttr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::ImageAttr::ImageAttr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoImageAttr(const char *name, long data_type, long max_x, long max_y) :
        Tango::ImageAttr(name, data_type, has_write_fn ? Tango::READ_WRITE : Tango::READ, max_x, max_y)
    {
    }

    ~AutoImageAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }
};

template <typename EnumType, auto read_fn, auto write_fn = nullptr>
class AutoEnumImageAttr : public Tango::ImageAttr
{
  public:
    using ReadDevice = typename detail::member_fn_traits<decltype(read_fn)>::class_type;
    using WriteDevice = typename detail::member_fn_traits<decltype(write_fn)>::class_type;
    constexpr static bool has_write_fn = std::negation_v<std::is_same<decltype(write_fn), std::nullptr_t>>;
    using Tango::ImageAttr::ImageAttr;

    // do we care about other possible parameters to Tango::Attr()?
    AutoEnumImageAttr(const char *name, long max_x, long max_y) :
        Tango::ImageAttr(name, Tango::DEV_ENUM, has_write_fn ? Tango::READ_WRITE : Tango::READ, max_x, max_y)
    {
    }

    ~AutoEnumImageAttr() override { }

    void read(Tango::DeviceImpl *dev, Tango::Attribute &att) override
    {
        std::invoke(read_fn, static_cast<ReadDevice *>(dev), att);
    }

    void write(Tango::DeviceImpl *dev, Tango::WAttribute &att) override
    {
        if constexpr(has_write_fn)
        {
            std::invoke(write_fn, static_cast<WriteDevice *>(dev), att);
        }
    }

    bool same_type(const std::type_info &in_type) override
    {
        return typeid(EnumType) == in_type;
    }

    std::string get_enum_type() override
    {
        return typeid(EnumType).name();
    }
};

namespace detail
{
constexpr const char *k_enabled_classes_env_var = "TANGO_TEST_ENABLED_CLASSES";

struct ClassRegistrarBase
{
    ClassRegistrarBase(const char *n) :
        name{n}
    {
        if(registrars == nullptr)
        {
            registrars = new std::vector<ClassRegistrarBase *>();
        }

        registrars->push_back(this);
    }

    virtual Tango::DeviceClass *init_class() = 0;

    virtual ~ClassRegistrarBase() = default;

    // We use a pointer here so that the first `ClassRegistrarBase` constructs
    // the vector.  This avoids the static initialisation order fiasco.
    static std::vector<ClassRegistrarBase *> *registrars;

    const char *name;
};

template <typename DeviceClass>
struct ClassRegistrar : ClassRegistrarBase
{
    using ClassRegistrarBase::ClassRegistrarBase;

    Tango::DeviceClass *init_class() override
    {
        return DeviceClass::init(name);
    }
};
} // namespace detail

} // namespace TangoTest

/**
 * @brief Instantiate a TangoTest::AutoDeviceClass for DEVICE.
 *
 * For each DEVICE, this macro must be used in a single implementation file to
 * instantiate static data members.
 *
 * The device class will be registered with Tango with the name #DEVICE.
 *
 * @param DEVICE class
 * @param NAME name of class
 */
#define TANGO_TEST_AUTO_DEV_CLASS_INSTANTIATE(DEVICE, NAME)                                        \
    template <>                                                                                    \
    TangoTest::AutoDeviceClass<DEVICE> *TangoTest::AutoDeviceClass<DEVICE>::_instance = nullptr;   \
    namespace                                                                                      \
    {                                                                                              \
    TangoTest::detail::ClassRegistrar<TangoTest::AutoDeviceClass<DEVICE>> NAME##_registrar{#NAME}; \
    }

#endif
