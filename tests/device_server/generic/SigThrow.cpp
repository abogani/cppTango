// NOLINTBEGIN(*)

#include "SigThrow.h"

//+----------------------------------------------------------------------------
//
// method : 		IOThrow::IOThrow()
//
// description : 	constructor for the IOThrow command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOThrow::IOThrow(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOThrow::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allowed only if the device is on
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IOThrow::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    const Tango::DevVarLongStringArray *theException;
    extract(in_any, theException);
    Tango::ErrSeverity severity = (Tango::ErrSeverity)(theException->lvalue)[0];
    TANGO_LOG << "[IOThrow::execute] throwing severity exception " << severity << std::endl;

    Tango::Except::throw_exception((const char *) (theException->svalue)[0],
                                   (const char *) "This is a test ",
                                   (const char *) "IOThrow::execute()",
                                   (Tango::ErrSeverity) severity);
}

//+----------------------------------------------------------------------------
//
// method : 		IOExcept::IOExcept()
//
// description : 	constructor for the IOThrow command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOExcept::IOExcept(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOExcept::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allowed only if the device is on
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IOExcept::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    using Tango::API_ThrowException;
    Tango::Except::throw_exception(
        (const char *) API_ThrowException, (const char *) "This is a test ", (const char *) "IOExcept::execute()");
}

//+----------------------------------------------------------------------------
//
// method : 		IOReThrow::IOReThrow()
//
// description : 	constructor for the IOReThrow command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOReThrow::IOReThrow(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOReThrow::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allowed only if the device is on
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IOReThrow::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    const Tango::DevVarLongStringArray *theException;
    extract(in_any, theException);
    Tango::ErrSeverity severity = (Tango::ErrSeverity)(theException->lvalue)[0];
    long nb_except = theException->lvalue.length();
    TANGO_LOG << "[IOReThrow::execute] throwing " << nb_except << " exception(s) " << std::endl;
    try
    {
        Tango::Except::throw_exception((const char *) (theException->svalue)[0],
                                       (const char *) "This is a test ",
                                       (const char *) "IOThrow::execute()",
                                       (Tango::ErrSeverity) severity);
    }
    catch(Tango::DevFailed &e)
    {
        if(nb_except > 2)
        {
            int ind = 1;
            for(int i = 0; i < nb_except - 2; i++)
            {
                try
                {
                    Tango::Except::re_throw_exception(e,
                                                      (const char *) (theException->svalue)[ind],
                                                      (const char *) "This is a test ",
                                                      (const char *) "IOReThrow::execute()",
                                                      (Tango::ErrSeverity)((theException->lvalue)[ind]));
                }
                catch(Tango::DevFailed &ex)
                {
                    e = ex;
                    ind++;
                }
            }
        }
        Tango::Except::re_throw_exception(e,
                                          (const char *) (theException->svalue)[nb_except - 1],
                                          (const char *) "This is a test ",
                                          (const char *) "IOReThrow::execute()",
                                          (Tango::ErrSeverity)((theException->lvalue)[nb_except - 1]));
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IORegClassSig::IORegClassSig()
//
// description : 	constructor for the IORegClassSig command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IORegClassSig::IORegClassSig(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IORegClassSig::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allways allowed
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IORegClassSig::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theSignal;
        extract(in_any, theSignal);
        TANGO_LOG << "[IORegClassSig::execute] received signal number " << theSignal << std::endl;
        device->get_device_class()->register_signal(theSignal);
        return insert();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IORegSig::IORegSig()
//
// description : 	constructor for the IOregSig command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IORegSig::IORegSig(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IORegSig::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allways allowed
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IORegSig::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theSignal;
        extract(in_any, theSignal);
        TANGO_LOG << "[IORegSig::execute] received signal number " << theSignal << std::endl;
        device->register_signal(theSignal);
        return insert();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IORegSigOwn::IORegSigOwn()
//
// description : 	constructor for the IOregSigOwn command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IORegSigOwn::IORegSigOwn(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IORegSigOwn::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allways allowed
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IORegSigOwn::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
#ifdef __linux
    try
    {
        Tango::DevLong theSignal;
        extract(in_any, theSignal);
        TANGO_LOG << "[IORegSigOwn::execute] received signal number " << theSignal << std::endl;
        device->register_signal(theSignal, true);
        return insert();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
#else
    CORBA::Any *out = nullptr;
    return out;
#endif
}

//+----------------------------------------------------------------------------
//
// method : 		IOUnregClassSig::IOUnregClassSig()
//
// description : 	constructor for the IOUnregClassSig command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOUnregClassSig::IOUnregClassSig(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOUnregClassSig::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command allways allowed
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IOUnregClassSig::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theSignal;
        extract(in_any, theSignal);
        TANGO_LOG << "[IOUnregClassSig::execute] received signal number " << theSignal << std::endl;
        device->get_device_class()->unregister_signal(theSignal);
        return insert();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOUnregSig::IOUnregSig()
//
// description : 	constructor for the IOUnregSig command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOUnregSig::IOUnregSig(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOUnregSig::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    //
    // command always allowed
    //

    if(device->get_state() == Tango::ON)
    {
        return (true);
    }
    else
    {
        return (false);
    }
}

CORBA::Any *IOUnregSig::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theSignal;
        extract(in_any, theSignal);
        TANGO_LOG << "[IOUnregSig::execute] received signal number " << theSignal << std::endl;
        device->unregister_signal(theSignal);
        return insert();
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

// NOLINTEND(*)
