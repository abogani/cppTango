#ifndef _DBDATUM_H
#define _DBDATUM_H

#include <tango/idl/tango.h>

#include <bitset>
#include <string>
#include <vector>
#include <memory>

namespace Tango
{
/**********************************************************************
 *                                                                    *
 *  DbDatum -    A database object for sending and receiving data     *
 *               from the Tango database API                          *
 *                                                                    *
 **********************************************************************/

/**
 * A database value
 *
 * A single database value which has a name, type, address and value and methods for inserting and extracting
 * C++ native types. This is the fundamental type for specifying database properties. Every property has a
 * name and has one or more values associated with it. The values can be inserted and extracted using the
 * operators << and >> respectively. A status flag indicates if there is data in the DbDatum object or not. An
 * additional flag allows the user to activate exceptions.
 *
 * @headerfile tango.h
 * @ingroup DBase
 */
class DbDatum
{
  public:
    /// @privatesection
    enum except_flags
    {
        isempty_flag,
        wrongtype_flag,
        numFlags
    };

    /// @publicsection
    /**@name Constructors */
    //@{
    /**
     * Create a DbDatum object.
     *
     * Create an instance of the DbDatum class with name set to the specified parameter
     *
     * @param [in] name    The CORBA ORB pointer. Default value is fine for 99 % of cases
     *
     */
    DbDatum(std::string name);
    /**
     * Create a DbDatum object.
     *
     * Create an instance of the DbDatum class with name set to the specified parameter
     *
     * @param [in] name    The CORBA ORB pointer. Default value is fine for 99 % of cases
     *
     */
    DbDatum(const char *name);
    //@}

    /**@name Operators overloading */
    //@{
    /**
     * Inserters operators
     *
     * The insert and extract operators are specified for the following C++ types :
     * @li bool
     * @li unsigned char
     * @li short
     * @li unsigned short
     * @li DevLong
     * @li DevULong
     * @li DevLong64
     * @li DevULong64
     * @li float
     * @li double
     * @li std::string
     * @li char* (insert only)
     * @li const char *
     * @li std::vector<std::string>
     * @li std::vector<short>
     * @li std::vector<unsigned short>
     * @li std::vector<DevLong>
     * @li std::vector<DevULong>
     * @li std::vector<DevLong64>
     * @li std::vector<DevULong64>
     * @li std::vector<float>
     * @li std::vector<double>
     *
     * Here is an example of creating, inserting and extracting some DbDatum types :
     * @code
     * DbDatum my_short("my_short"), my_long(“my_long”), my_string("my_string");
     * DbDatum my_float_vector("my_float_vector"), my_double_vector("my_double_vector");
     *
     * std::string a_string;
     * short a_short;
     * DevLong a_long;
     * std::vector<float> a_float_vector;
     * std::vector<double> a_double_vector;
     *
     * my_short << 100; // insert a short
     * my_short >> a_short; // extract a short
     * my_long << 1000; // insert a DevLong
     * my_long >> a_long; // extract a long
     * my_string << std::string("estas lista a bailar el tango ?"); // insert a string
     * my_string >> a_string; // extract a string
     * my_float_vector << a_float_vector // insert a vector of floats
     * my_float_vector >> a_float_vector; // extract a vector of floats
     * my_double_vector << a_double_vector; // insert a vector of doubles
     * my_double_vector >> a_double_vector; // extract a vector of doubles
     * @endcode
     *
     * @param [in] val Data to be inserted
     *
     * @exception WrongData if requested
     */
    void operator<<(bool val);
    /**
     * Extractors operators
     *
     * See documentation of the DbDatum::operator<< for details
     *
     * @param [out] val Data to be initalized with database value
     * @return A boolean set to true if the extraction succeeds
     * @exception WrongData if requested
     */
    bool operator>>(bool &val);

    //@}
    /**@name Exception related methods methods */
    //@{
    /**
     * Set exception flag
     *
     * Is a method which allows the user to switch on/off exception throwing for trying to extract data from an
     * empty DbDatum object. The default is to not throw exception. The following flags are supported :
     * @li @b isempty_flag - throw a WrongData exception (reason = API_EmptyDbDatum) if user tries to extract
     *       data from an empty DbDatum object
     * @li @b wrongtype_flag - throw a WrongData exception (reason = API_IncompatibleArgumentType) if user
     *       tries to extract data with a type different than the type used for insertion
     *
     * @param [in] fl The exception flag
     */
    void exceptions(std::bitset<DbDatum::numFlags> fl)
    {
        exceptions_flags = fl;
    }

    /**
     * Get exception flag
     *
     * Returns the whole exception flags.
     * The following is an example of how to use these exceptions related methods
     * @code
     * DbDatum da;
     *
     * std::bitset<DbDatum::numFlags> bs = da.exceptions();
     * std::cout << "bs = " << bs << std::endl;
     *
     * da.set_exceptions(DbDatum::wrongtype_flag);
     * bs = da.exceptions();
     *
     * std::cout << "bs = " << bs << std::endl;
     * @endcode
     *
     * @return The exception flag
     */
    std::bitset<DbDatum::numFlags> exceptions()
    {
        return exceptions_flags;
    }

    /**
     * Reset one exception flag
     *
     * Resets one exception flag
     *
     * @param [in] fl The exception flag
     */
    void reset_exceptions(except_flags fl)
    {
        exceptions_flags.reset((size_t) fl);
    }

    /**
     * Set one exception flag
     *
     * Sets one exception flag. See DbDatum::exceptions() for a usage example
     *
     * @param [in] fl The exception flag
     */
    void set_exceptions(except_flags fl)
    {
        exceptions_flags.set((size_t) fl);
    }

    //@}
    /**@name Miscellaneous methods */
    //@{
    /**
     * Test if instance is empty
     *
     * is_empty() is a boolean method which returns true or false depending on whether the DbDatum object contains
     * data or not. It can be used to test whether a property is defined in the database or not e.g.
     * @code
     * sl_props.push_back(parity_prop);
     * dbase->get_device_property(device_name, sl_props);
     *
     * if (! parity_prop.is_empty())
     * {
     *     parity_prop >> parity;
     * }
     * else
     * {
     *     std::cout << device_name << " has no parity defined in database !" << std::endl;
     * }
     * @endcode
     *
     * @return True if DdDatum instance is empty
     *
     * @exception WrongData if requested
     */
    bool is_empty();
    //@}
    /// @privatesection

    std::string name;
    std::vector<std::string> value_string;
    //
    // constructor methods
    //
    DbDatum();
    ~DbDatum();
    DbDatum(const DbDatum &);
    DbDatum &operator=(const DbDatum &);

    size_t size() const
    {
        return value_string.size();
    }

    //
    // insert methods
    //

    void operator<<(short);
    void operator<<(unsigned char);
    void operator<<(unsigned short);
    void operator<<(DevLong);
    void operator<<(DevULong);
    void operator<<(DevLong64);
    void operator<<(DevULong64);
    void operator<<(float);
    void operator<<(double);
    void operator<<(char *);
    //    void operator << (char *&);
    void operator<<(const char *);
    //    void operator << (const char *&);
    void operator<<(const std::string &);

    void operator<<(const std::vector<std::string> &);
    void operator<<(const std::vector<short> &);
    void operator<<(const std::vector<unsigned short> &);
    void operator<<(const std::vector<DevLong> &);
    void operator<<(const std::vector<DevULong> &);
    void operator<<(const std::vector<DevLong64> &);
    void operator<<(const std::vector<DevULong64> &);
    void operator<<(const std::vector<float> &);
    void operator<<(const std::vector<double> &);

    //
    // extract methods
    //

    bool operator>>(short &) const;
    bool operator>>(unsigned char &) const;
    bool operator>>(unsigned short &) const;
    bool operator>>(DevLong &) const;
    bool operator>>(DevULong &) const;
    bool operator>>(DevLong64 &) const;
    bool operator>>(DevULong64 &) const;
    bool operator>>(float &) const;
    bool operator>>(double &) const;
    bool operator>>(const char *&) const;
    bool operator>>(std::string &) const;

    bool operator>>(std::vector<std::string> &) const;
    bool operator>>(std::vector<short> &) const;
    bool operator>>(std::vector<unsigned short> &) const;
    bool operator>>(std::vector<DevLong> &) const;
    bool operator>>(std::vector<DevULong> &) const;
    bool operator>>(std::vector<DevLong64> &) const;
    bool operator>>(std::vector<DevULong64> &) const;
    bool operator>>(std::vector<float> &) const;
    bool operator>>(std::vector<double> &) const;

  private:
    int value_type;
    int value_size;
    std::bitset<numFlags> exceptions_flags;

    class DbDatumExt
    {
      public:
        DbDatumExt() { }
    };

    std::unique_ptr<DbDatumExt> ext;
};

typedef std::vector<DbDatum> DbData;
} // namespace Tango
#endif
