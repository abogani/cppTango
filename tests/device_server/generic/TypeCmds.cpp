// NOLINTBEGIN(*)

#include "TypeCmds.h"

//+----------------------------------------------------------------------------
//
// method : 		IOVoid::IOVoid()
//
// description : 	constructor for the IOVoid command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOVoid::IOVoid(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOVoid::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOVoid::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        TANGO_LOG << "[IOVoid::execute] " << std::endl;
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
// method : 		IOBool::IOBool()
//
// description : 	constructor for the IOBool command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOBool::IOBool(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOBool::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOBool::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevBoolean theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOBool::execute] received number " << theNumber << std::endl;
        theNumber = !theNumber;
        TANGO_LOG << "[IOBool::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOShort::IOShort()
//
// description : 	constructor for the IOShort command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOShort::IOShort(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOShort::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOShort::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevShort theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOShort::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOShort::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOLong::IOLong()
//
// description : 	constructor for the IOLong command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOLong::IOLong(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOLong::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOLong::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOLong::execute] received number " << theNumber << std::endl;
        DEV_DEBUG_STREAM(device) << "[IOLong::execute] received number " << theNumber;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOLong::execute] return number " << theNumber << std::endl;
        DEV_DEBUG_STREAM(device) << "[IOLong::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOLong64::IOLong64()
//
// description : 	constructor for the IOLong64 command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOLong64::IOLong64(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOLong64::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOLong64::execute(Tango::DeviceImpl *device, const CORBA::Any &in_any)
{
    try
    {
        Tango::DevLong theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOLong64::execute] received number " << theNumber << std::endl;
        DEV_DEBUG_STREAM(device) << "[IOLong64::execute] received number " << theNumber;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOLong64::execute] return number " << theNumber << std::endl;
        DEV_DEBUG_STREAM(device) << "[IOLong64::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOFloat::IOFloat()
//
// description : 	constructor for the IOFloat command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOFloat::IOFloat(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOFloat::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOFloat::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevFloat theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOFloat::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOFloat::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IODouble::IODouble()
//
// description : 	constructor for the IODouble command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IODouble::IODouble(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IODouble::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IODouble::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevDouble theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IODouble::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IODouble::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOUShort::IOUShort()
//
// description : 	constructor for the IOUShort command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOUShort::IOUShort(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOUShort::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOUShort::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevUShort theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOUShort::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOUShort::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOULong::IOULong()
//
// description : 	constructor for the IOULong command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOULong::IOULong(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOULong::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOULong::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevULong theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOULong::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOULong::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOULong64::IOULong64()
//
// description : 	constructor for the IOULong64 command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOULong64::IOULong64(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOULong64::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOULong64::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        Tango::DevULong64 theNumber;
        extract(in_any, theNumber);
        TANGO_LOG << "[IOULong64::execute] received number " << theNumber << std::endl;
        theNumber = theNumber * 2;
        TANGO_LOG << "[IOULong64::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOString::IOString()
//
// description : 	constructor for the IOString command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOString::IOString(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOString::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOString::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    TANGO_LOG_INFO << "[IOString::execute] arrived" << std::endl;
    try
    {
        Tango::DevString theWord;
        extract(in_any, theWord);
        std::string palindrome;
        std::string firstWord = theWord;
        std::string::reverse_iterator currentChar(firstWord.rbegin());
        std::string::reverse_iterator endChar(firstWord.rend());

        TANGO_LOG << "[IOString::execute] firstWord = " << firstWord << std::endl;

        for(; currentChar != endChar; currentChar++)
        {
            TANGO_LOG << "[IOString::execute]  currentChar = " << *currentChar << std::endl;
            palindrome += *currentChar;
        }
        TANGO_LOG << "[IOString::execute] palindrome = " << palindrome << std::endl;
        return insert(palindrome.c_str());
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOCharArray::IOCharArray()
//
// description : 	constructor for the IOCharArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOCharArray::IOCharArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOCharArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOCharArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        TANGO_LOG << "[IOCharArray::execute] entering " << std::endl;
        const Tango::DevVarCharArray *theCharArray;
        extract(in_any, theCharArray);
        Tango::DevVarCharArray *theReturnedArray = new Tango::DevVarCharArray();
        theReturnedArray->length(theCharArray->length());
        for(unsigned int i = 0; i < theCharArray->length(); i++)
        {
            TANGO_LOG << "[IOCharArray::execute] received char " << (*theCharArray)[i] << std::endl;
            (*theReturnedArray)[theCharArray->length() - i - 1] = (*theCharArray)[i];
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOShortArray::IOShortArray()
//
// description : 	constructor for the IOShortArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOShortArray::IOShortArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOShortArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOShortArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarShortArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarShortArray *theReturnedArray = new Tango::DevVarShortArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IOShortArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IOShortArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOLongArray::IOLongArray()
//
// description : 	constructor for the IOLongArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOLongArray::IOLongArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOLongArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOLongArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarLongArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarLongArray *theReturnedArray = new Tango::DevVarLongArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IOLongArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IOLongArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOFloatArray::IOFloatArray()
//
// description : 	constructor for the IOFloatArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOFloatArray::IOFloatArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOFloatArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOFloatArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarFloatArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarFloatArray *theReturnedArray = new Tango::DevVarFloatArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IOFloatArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IOFloatArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IODoubleArray::IODoubleArray()
//
// description : 	constructor for the IODoubleArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IODoubleArray::IODoubleArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Command(name, in, out, in_desc, out_desc)
{
}

bool IODoubleArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IODoubleArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarDoubleArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarDoubleArray *theReturnedArray = new Tango::DevVarDoubleArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IODoubleArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IODoubleArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOUShortArray::IOUShortArray()
//
// description : 	constructor for the IOUShortArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOUShortArray::IOUShortArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOUShortArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOUShortArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarUShortArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarUShortArray *theReturnedArray = new Tango::DevVarUShortArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IOUShortArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IOUShortArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOULongArray::IOULongArray()
//
// description : 	constructor for the IOULongArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOULongArray::IOULongArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOULongArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOULongArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarULongArray *theNumberArray;
        extract(in_any, theNumberArray);
        Tango::DevVarULongArray *theReturnedArray = new Tango::DevVarULongArray();
        theReturnedArray->length(theNumberArray->length());
        for(unsigned int i = 0; i < theNumberArray->length(); i++)
        {
            TANGO_LOG << "[IOULongArray::execute] received number " << (*theNumberArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*theNumberArray)[i] * 2;
            TANGO_LOG << "[IOULongArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOStringArray::IOStringArray()
//
// description : 	constructor for the IOStringArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOStringArray::IOStringArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Command(name, in, out, in_desc, out_desc)
{
}

bool IOStringArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOStringArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarStringArray *theStringArray;
        extract(in_any, theStringArray);
        Tango::DevVarStringArray *theReturnedArray = new Tango::DevVarStringArray();
        theReturnedArray->length(theStringArray->length());
        for(unsigned int i = 0; i < theStringArray->length(); i++)
        {
            TANGO_LOG << "[IOStringArray::execute] received String " << (*theStringArray)[i].in() << std::endl;
            (*theReturnedArray)[theStringArray->length() - i - 1] = (*theStringArray)[i];
            TANGO_LOG << "[IOStringArray::execute] return String " << (*theReturnedArray)[i].in() << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOLongString::IOLongString()
//
// description : 	constructor for the IOLongString command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOLongString::IOLongString(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOLongString::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOLongString::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarLongStringArray *theReceived;
        extract(in_any, theReceived);
        Tango::DevVarLongStringArray *theReturned = new Tango::DevVarLongStringArray;
        unsigned int i;

        ((*theReturned).lvalue).length(((*theReceived).lvalue).length());
        for(i = 0; i < ((*theReceived).lvalue).length(); i++)
        {
            TANGO_LOG << "[IOLongString::execute] received number " << (*theReceived).lvalue[i] << std::endl;
            (*theReturned).lvalue[i] = (*theReceived).lvalue[i] * 2;
            TANGO_LOG << "[IOLongString::execute] return number " << (*theReturned).lvalue[i] << std::endl;
        }
        ((*theReturned).svalue).length(((*theReceived).svalue).length());
        for(i = 0; i < ((*theReceived).svalue).length(); i++)
        {
            TANGO_LOG << "[IOLongString::execute] received string " << (*theReceived).svalue[i].in() << std::endl;
            (*theReturned).svalue[i] = Tango::string_dup((*theReceived).svalue[i]);
            TANGO_LOG << "[IOLongString::execute] return string " << (*theReturned).svalue[i].in() << std::endl;
        }
        return insert(theReturned);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IODoubleString::IODoubleString()
//
// description : 	constructor for the IODoubleString command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IODoubleString::IODoubleString(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IODoubleString::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IODoubleString::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarDoubleStringArray *theReceived;
        Tango::DevVarDoubleStringArray *theReturned = new Tango::DevVarDoubleStringArray();
        unsigned int i;

        extract(in_any, theReceived);
        ((*theReturned).dvalue).length(((*theReceived).dvalue).length());
        for(i = 0; i < ((*theReceived).dvalue).length(); i++)
        {
            TANGO_LOG << "[IODoubleString::execute] received number " << (*theReceived).dvalue[i] << std::endl;
            (*theReturned).dvalue[i] = (*theReceived).dvalue[i] * 2;
            TANGO_LOG << "[IODoubleString::execute] return number " << (*theReturned).dvalue[i] << std::endl;
        }
        ((*theReturned).svalue).length(((*theReceived).svalue).length());
        for(i = 0; i < ((*theReceived).svalue).length(); i++)
        {
            TANGO_LOG << "[IODoubleString::execute] received string " << (*theReceived).svalue[i].in() << std::endl;
            (*theReturned).svalue[i] = (*theReceived).svalue[i];
            TANGO_LOG << "[IODoubleString::execute] return string " << (*theReturned).svalue[i].in() << std::endl;
        }
        return insert(theReturned);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOBooleanArray::IOBooleanArray()
//
// description : 	constructor for the IOBooleanArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOBooleanArray::IOBooleanArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOBooleanArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOBooleanArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevVarBooleanArray *booleanArray;
        extract(in_any, booleanArray);
        Tango::DevVarBooleanArray *theReturnedArray = new Tango::DevVarBooleanArray();
        theReturnedArray->length(booleanArray->length());
        for(unsigned int i = 0; i < booleanArray->length(); i++)
        {
            TANGO_LOG << "[IOBoolArray::execute] received bool " << (*booleanArray)[i] << std::endl;
            (*theReturnedArray)[i] = (*booleanArray)[i];
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OLong::OLong()
//
// description : 	constructor for the OLong command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OLong::OLong(const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool OLong::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OLong::execute(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        long theNumber = 22;
        TANGO_LOG << "[OLong::execute] return number " << theNumber << std::endl;
        DEV_DEBUG_STREAM(device) << "[OLong::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OULong::OULong()
//
// description : 	constructor for the OULong command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OULong::OULong(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool OULong::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OULong::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        unsigned long theNumber = 333;
        TANGO_LOG << "[OULong::execute] return number " << theNumber << std::endl;
        return insert(theNumber);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OLongArray::OLongArray()
//
// description : 	constructor for the OLongArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OLongArray::OLongArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool OLongArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OLongArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        Tango::DevVarLongArray *theReturnedArray = new Tango::DevVarLongArray();
        theReturnedArray->length(4);
        for(int i = 0; i < 4; i++)
        {
            (*theReturnedArray)[i] = 555 + i;
            TANGO_LOG << "[OLongArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OULongArray::OULongArray()
//
// description : 	constructor for the OULongArray command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OULongArray::OULongArray(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool OULongArray::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OULongArray::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        Tango::DevVarULongArray *theReturnedArray = new Tango::DevVarULongArray();
        theReturnedArray->length(3);
        for(int i = 0; i < 3; i++)
        {
            (*theReturnedArray)[i] = 777 + i;
            TANGO_LOG << "[OULongArray::execute] return number " << (*theReturnedArray)[i] << std::endl;
        }
        return insert(theReturnedArray);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OLongString::OLongString()
//
// description : 	constructor for the IOLongString command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OLongString::OLongString(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool OLongString::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OLongString::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        Tango::DevVarLongStringArray *theReturned = new Tango::DevVarLongStringArray();
        int i;

        ((*theReturned).lvalue).length(6);
        for(i = 0; i < 6; i++)
        {
            (*theReturned).lvalue[i] = 999 + i;
            TANGO_LOG << "[OLongString::execute] return number " << (*theReturned).lvalue[i] << std::endl;
        }
        ((*theReturned).svalue).length(1);
        for(i = 0; i < 1; i++)
        {
            (*theReturned).svalue[i] = Tango::string_dup("Hola todos");
            TANGO_LOG << "[OLongString::execute] return string " << (*theReturned).svalue[i].in() << std::endl;
        }
        return insert(theReturned);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		IOUShortArray::IOEncoded()
//
// description : 	constructor for the IOEncoded command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

IOEncoded::IOEncoded(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
}

bool IOEncoded::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *IOEncoded::execute(TANGO_UNUSED(Tango::DeviceImpl *device), const CORBA::Any &in_any)
{
    try
    {
        const Tango::DevEncoded *the_enc;
        extract(in_any, the_enc);
        Tango::DevEncoded *theReturned_enc = new Tango::DevEncoded();
        theReturned_enc->encoded_data.length(the_enc->encoded_data.length());
        TANGO_LOG << "[IOEncoded::execute] received string " << the_enc->encoded_format << std::endl;
        for(unsigned int i = 0; i < the_enc->encoded_data.length(); i++)
        {
            TANGO_LOG << "[IOEncoded::execute] received number " << (int) the_enc->encoded_data[i] << std::endl;
            theReturned_enc->encoded_data[i] = the_enc->encoded_data[i] * 2;
            TANGO_LOG << "[IOEncoded::execute] returned number " << (int) theReturned_enc->encoded_data[i] << std::endl;
        }
        theReturned_enc->encoded_format = Tango::string_dup("Returned string");
        return insert(theReturned_enc);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

//+----------------------------------------------------------------------------
//
// method : 		OEncoded::OEncoded()
//
// description : 	constructor for the OEncoded command of the
//			DevTest.
//
// In : - name : The command name
//	- in : The input parameter type
//	- out : The output parameter type
//	- in_desc : The input parameter description
//	- out_desc : The output parameter description
//
//-----------------------------------------------------------------------------

OEncoded::OEncoded(
    const char *name, Tango::CmdArgType in, Tango::CmdArgType out, const char *in_desc, const char *out_desc) :
    Tango::Command(name, in, out, in_desc, out_desc)
{
    encoded_cmd_ctr = 0;
}

bool OEncoded::is_allowed(Tango::DeviceImpl *device, TANGO_UNUSED(const CORBA::Any &in_any))
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

CORBA::Any *OEncoded::execute(TANGO_UNUSED(Tango::DeviceImpl *device), TANGO_UNUSED(const CORBA::Any &in_any))
{
    try
    {
        Tango::DevEncoded *theReturned = new Tango::DevEncoded();

        encoded_cmd_ctr++;
        if((encoded_cmd_ctr % 2) == 0)
        {
            theReturned->encoded_format = Tango::string_dup("Odd - OEncoded format");
            theReturned->encoded_data.length(2);
            theReturned->encoded_data[0] = 11;
            theReturned->encoded_data[1] = 21;
        }
        else
        {
            theReturned->encoded_format = Tango::string_dup("Even - OEncoded format");
            theReturned->encoded_data.length(4);
            theReturned->encoded_data[0] = 10;
            theReturned->encoded_data[1] = 20;
            theReturned->encoded_data[2] = 30;
            theReturned->encoded_data[3] = 40;
        }
        return insert(theReturned);
    }
    catch(CORBA::Exception &e)
    {
        Tango::Except::print_exception(e);
        throw;
    }
}

// NOLINTEND(*)
