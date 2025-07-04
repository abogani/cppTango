// Copyright (C) :      2004,2005,2006,2007,2008,2009,2010,2011,2012,2013,2014,2015
//                        European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Tango.  If not, see <http://www.gnu.org/licenses/>.

#include <tango/client/filedatabase.h>
#include <tango/common/utils/type_info.h>
#include <tango/common/tango_const.h>
#include <tango/common/tango_type_traits.h>
#include <tango/server/logging.h>
#include <tango/server/except.h>
#include <tango/client/apiexcept.h>

#include <iostream>
#include <numeric>
#include <algorithm>
// DbInfo                              done
// DbImportDevice
// DbExportDevice
// DbUnExportDevice
// DbAddDevice
// DbDeleteDevice
// DbAddServer
// DbDeleteServer
// DbExportServer
// DbUnExportServer
// DbGetServerInfo

// DbGetDeviceProperty                  done
// DbPutDeviceProperty                  done
// DbDeleteDeviceProperty               done
// DbGetDeviceAttributeProperty         done
// DbPutDeviceAttributeProperty         done
// DbDeleteDeviceAttributeProperty      to check
// DbGetClassProperty                   done
// DbPutClassProperty                   done
// DbDeleteClassProperty
// DbGetClassAttributeProperty          done
// DbPutClassAttributeProperty          done
// DbDeleteClassAttributeProperty
// DbGetDeviceList                      done

// DbGetDeviceMemberList
// DbGetDeviceExportedList
// DbGetDeviceDomainList
// DbGetDeviceFamilyList
// DbGetProperty                        done
// DbPutProperty                        done
// DbDeleteProperty                     done
// DbGetAliasDevice
// DbGetDeviceAlias
// DbGetAttributeAlias
// DbGetDeviceAliasList
// DbGetAttributeAliasList

// ****************************************************
// Go to the next significant character
// ****************************************************

// ****************************************************
// return the lexical classe of the next word               */
// ****************************************************

using namespace std;

namespace
{

char chartolower(const char c)
{
    if(c >= 'A' && c <= 'Z')
    {
        return c - 'A' + 'a';
    }
    else
    {
        return c;
    }
}

bool equalsIgnoreCase(const string &s1, const string &s2)
{
    bool ret = false;
    if(s1.size() == s2.size())
    {
        int l = s1.size();
        while(l != 0)
        {
            ret = (chartolower(s1.at(l - 1)) == chartolower(s2.at(l - 1)));
            l--;
            if(!ret)
            {
                break;
            }
        }
    }
    return ret;
}

Tango::t_device *search_device(Tango::t_server &s, string &name)
{
    for(unsigned int j = 0; j < s.devices.size(); j++)
    {
        if(equalsIgnoreCase(s.devices[j]->name, name))
        {
            return s.devices[j];
        }
    }

    return nullptr;
}

Tango::t_tango_class *search_class(Tango::t_server &s, string &name)
{
    for(unsigned int i = 0; i < s.classes.size(); i++)
    {
        if(equalsIgnoreCase(s.classes[i]->name, name))
        {
            return s.classes[i];
        }
    }
    return nullptr;
}

Tango::t_free_object *search_free_object(Tango::t_server &s, string &name)
{
    for(unsigned int i = 0; i < s.free_objects.size(); i++)
    {
        if(equalsIgnoreCase(s.free_objects[i]->name, name))
        {
            return s.free_objects[i];
        }
    }
    return nullptr;
}

Tango::t_attribute_property *search_dev_attr_prop(Tango::t_device *d, const string &name)
{
    for(unsigned int i = 0; i < d->attribute_properties.size(); i++)
    {
        if(equalsIgnoreCase(d->attribute_properties[i]->attribute_name, name))
        {
            return d->attribute_properties[i];
        }
    }
    return nullptr;
}

Tango::t_attribute_property *search_class_attr_prop(Tango::t_tango_class *d, const string &name)
{
    for(unsigned int i = 0; i < d->attribute_properties.size(); i++)
    {
        if(equalsIgnoreCase(d->attribute_properties[i]->attribute_name, name))
        {
            return d->attribute_properties[i];
        }
    }
    return nullptr;
}

char *to_corba_string(CORBA::ULong val)
{
    auto str = std::to_string(val);
    return Tango::string_dup(str.c_str());
}

} // anonymous namespace

namespace Tango
{
const char *FileDatabase::lexical_word_null = "NULL";
const char *FileDatabase::lexical_word_number = "NUMBER";
const char *FileDatabase::lexical_word_string = "STRING";
const char *FileDatabase::lexical_word_coma = "COMA";
const char *FileDatabase::lexical_word_colon = "COLON";
const char *FileDatabase::lexical_word_slash = "SLASH";
const char *FileDatabase::lexical_word_backslash = "BackSLASH";
const char *FileDatabase::lexical_word_arrow = "->";
int FileDatabase::ReadBufferSize = 4069;
int FileDatabase::MaxWordLength = 256;

template <class T>
bool hasName<T>::operator()(T *obj)
{
    return (equalsIgnoreCase(obj->name, name));
}

template <class T>
bool hasAttributeName<T>::operator()(T *obj)
{
    return (equalsIgnoreCase(obj->attribute_name, attribute_name));
}

FileDatabaseExt::FileDatabaseExt() { }

FileDatabase::FileDatabase(const std::string &file_name) :
    ext(new FileDatabaseExt)
{
    TANGO_LOG_DEBUG << "FILEDATABASE: FileDatabase constructor" << endl;
    filename = file_name;

    parse_res_file(filename);
}

FileDatabase::~FileDatabase()
{
    TANGO_LOG_DEBUG << "FILEDATABASE: FileDatabase destructor" << endl;
    //    write_file();

    std::vector<t_device *>::iterator i;
    for(i = m_server.devices.begin(); i != m_server.devices.end(); ++i)
    {
        std::vector<t_property *>::iterator p;
        for(p = (*i)->properties.begin(); p != (*i)->properties.end(); ++p)
        {
            delete(*p);
        }
        std::vector<t_attribute_property *>::iterator pa;
        for(pa = (*i)->attribute_properties.begin(); pa != (*i)->attribute_properties.end(); ++pa)
        {
            std::vector<t_property *>::iterator p;
            for(p = (*pa)->properties.begin(); p != (*pa)->properties.end(); ++p)
            {
                delete(*p);
            }
            delete(*pa);
        }
        delete(*i);
    }
    std::vector<t_tango_class *>::iterator j;
    for(j = m_server.classes.begin(); j != m_server.classes.end(); ++j)
    {
        std::vector<t_property *>::iterator p;
        for(p = (*j)->properties.begin(); p != (*j)->properties.end(); ++p)
        {
            delete(*p);
        }
        std::vector<t_attribute_property *>::iterator pa;
        for(pa = (*j)->attribute_properties.begin(); pa != (*j)->attribute_properties.end(); ++pa)
        {
            std::vector<t_property *>::iterator p;
            for(p = (*pa)->properties.begin(); p != (*pa)->properties.end(); ++p)
            {
                delete(*p);
            }
            delete(*pa);
        }
        delete(*j);
    }

    for(auto *obj : m_server.free_objects)
    {
        for(auto *prop : obj->properties)
        {
            delete prop;
        }
        delete obj;
    }
}

// ****************************************************
// read the next character in the file
// ****************************************************
void FileDatabase ::read_char(ifstream &f)
{
    CurrentChar = NextChar;
    if(!f.eof())
    {
        f.get(NextChar);
    }
    else
    {
        NextChar = 0;
    }
    if(CurrentChar == '\n')
    {
        CrtLine++;
    }
}

int FileDatabase ::class_lex(const string &tmp_word)
{
    /* exepction */
    if(tmp_word.empty())
    {
        return 0;
    }
    if(tmp_word.length() == 0)
    {
        return _TG_STRING;
    }

    /* Special character */

    if(tmp_word == "/")
    {
        return _TG_SLASH;
    }
    if(tmp_word == "\\")
    {
        return _TG_ASLASH;
    }
    if(tmp_word == ",")
    {
        return _TG_COMA;
    }
    if(tmp_word == ":")
    {
        return _TG_COLON;
    }
    if(tmp_word == "->")
    {
        return _TG_ARROW;
    }

    return _TG_STRING;
}

// ****************************************************
// Go to the next line                                */
// ****************************************************
void FileDatabase ::jump_line(ifstream &f)
{
    while(CurrentChar != '\n' && CurrentChar != 0)
    {
        read_char(f);
    }
    read_char(f);
}

void FileDatabase ::jump_space(ifstream &f)
{
    while((CurrentChar <= 32) && (CurrentChar > 0))
    {
        read_char(f);
    }
}

// ****************************************************
// Read the next word in the file                           */
// ****************************************************
string FileDatabase ::read_word(ifstream &f)
{
    string ret_word;

    /* Jump space and comments */
    jump_space(f);
    while(CurrentChar == '#')
    {
        jump_line(f);
        jump_space(f);
    }

    /* Jump C like comments */
    if(CurrentChar == '/')
    {
        read_char(f);
        if(CurrentChar == '*')
        {
            bool end = false;
            read_char(f);
            while(end)
            {
                while(CurrentChar != '*')
                {
                    read_char(f);
                }
                read_char(f);
                end = (CurrentChar == '/');
            }
            read_char(f);
            jump_space(f);
        }
        else
        {
            ret_word = "/";
            return ret_word;
        }
    }

    StartLine = CrtLine;

    /* Treat special character */
    if(CurrentChar == ':' || CurrentChar == '/' || CurrentChar == ',' || CurrentChar == '\\' ||
       (CurrentChar == '-' && NextChar == '>'))
    {
        if(CurrentChar != '-')
        {
            ret_word += CurrentChar;
        }
        else
        {
            ret_word += CurrentChar;
            read_char(f);
            ret_word += CurrentChar;
        }
        read_char(f);
        return ret_word;
    }

    /* Treat string */
    if(CurrentChar == '"')
    {
        read_char(f);
        while(CurrentChar != '"' && CurrentChar != 0 && CurrentChar != '\n')
        {
            ret_word += CurrentChar;
            read_char(f);
        }
        if(CurrentChar == 0 || CurrentChar == '\n')
        {
            TANGO_LOG_DEBUG << "Error at line " << StartLine << endl;
            TangoSys_MemStream desc;
            desc << "File database: Error in file at line " << StartLine;
            desc << " in file " << filename << "." << ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
        }
        read_char(f);
        return ret_word;
    }

    /* Treat other word */
    while(CurrentChar > 32 && CurrentChar != ':' && CurrentChar != '/' && CurrentChar != '\\' && CurrentChar != ',')
    {
        if(CurrentChar == '-' && NextChar == '>')
        {
            break;
        }
        ret_word += CurrentChar;
        read_char(f);
    }

    if(ret_word.length() == 0)
    {
        return string(lexical_word_null);
    }

    return ret_word;
}

// ****************************************************
// Read the next word in the file
// And allow / inside
// ****************************************************
string FileDatabase::read_full_word(ifstream &f)
{
    string ret_word;

    StartLine = CrtLine;
    jump_space(f);

    /* Treat special character */
    if(CurrentChar == ',' || CurrentChar == '\\')
    {
        ret_word += CurrentChar;
        read_char(f);
        return ret_word;
    }

    /* Treat string */
    if(CurrentChar == '"')
    {
        read_char(f);
        while(CurrentChar != '"' && CurrentChar != 0)
        {
            if(CurrentChar == '\\')
            {
                read_char(f);
            }
            ret_word += CurrentChar;
            read_char(f);
        }
        if(CurrentChar == 0)
        {
            TANGO_LOG_DEBUG << "Warning: String too long at line " << StartLine << endl;
            TangoSys_MemStream desc;
            desc << "File database: String too long at line " << StartLine;
            desc << " in file " << filename << "." << ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
        }
        read_char(f);
        if(ret_word.length() == 0)
        {
            ret_word = string(lexical_word_null);
        }
        return ret_word;
    }

    /* Treat other word */
    while(CurrentChar > 32 && CurrentChar != '\\' && CurrentChar != ',')
    {
        ret_word += CurrentChar;
        read_char(f);
    }

    if(ret_word.length() == 0)
    {
        ret_word = string(lexical_word_null);
        return ret_word;
    }

    return ret_word;
}

void FileDatabase::CHECK_LEX(int lt, int le)
{
    if(lt != le)
    {
        TANGO_LOG_DEBUG << "Error at line " << StartLine << endl;
        TangoSys_MemStream desc;
        desc << "File database: Error in file at line " << StartLine;
        desc << " in file " << filename << "." << ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
    }
}

vector<string> FileDatabase::parse_resource_value(ifstream &f)
{
    int lex;
    vector<string> ret;

    /* Resource value */
    lex = _TG_COMA;

    while((lex == _TG_COMA || lex == _TG_ASLASH) && word != "")
    {
        word = read_full_word(f);
        lex = class_lex(word);

        /* allow ... ,\ syntax */
        if(lex == _TG_ASLASH)
        {
            word = read_full_word(f);
            lex = class_lex(word);
        }

        CHECK_LEX(lex, _TG_STRING);

        ret.push_back(word);

        word = read_word(f);
        lex = class_lex(word);
    }

    return ret;
}

// ****************************************************
// Parse a resource file
// Return error as String (zero length when sucess)
// ****************************************************

std::string FileDatabase::parse_res_file(const std::string &file_name)
{
    ifstream f;
    bool eof = false;
    int lex;

    t_tango_class *un_class;
    t_device *un_device;

    string domain;
    string family;
    string member;
    string name;
    string prop_name;

    CrtLine = 1;
    NextChar = ' ';
    CurrentChar = ' ';

    TANGO_LOG_DEBUG << "FILEDATABASE: entering parse_res_file" << endl;

    /* OPEN THE FILE                  */

    f.open(file_name.c_str(), ifstream::in);
    if(!f.good())
    {
        TangoSys_MemStream desc;
        desc << "FILEDATABASE could not open file " << file_name << "." << ends;
        TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
    }

    /* CHECK BEGINING OF CONFIG FILE  */

    word = read_word(f);
    if(word == "")
    {
        f.close();
        return file_name + " is empty...";
    }
    lex = class_lex(word);
    m_server.name = word;

    /* PARSE                          */

    while(!eof)
    {
        switch(lex)
        {
            /* Start a resource mame */
        case _TG_STRING:

            /* Domain */
            domain = word;
            word = read_word(f);
            lex = class_lex(word);

            // TANGO_LOG << "DOMAIN " << domain << endl;;
            CHECK_LEX(lex, _TG_SLASH);

            /* Family */
            word = read_word(f);
            lex = class_lex(word);
            CHECK_LEX(lex, _TG_STRING);
            family = word;
            // TANGO_LOG << "FAMILI " << family << endl;
            word = read_word(f);
            lex = class_lex(word);

            switch(lex)
            {
            case _TG_SLASH:

                /* Member */
                word = read_word(f);
                lex = class_lex(word);
                CHECK_LEX(lex, _TG_STRING);
                member = word;
                word = read_word(f);
                lex = class_lex(word);

                switch(lex)
                {
                case _TG_SLASH:
                    /* We have a 4 fields name */
                    word = read_word(f);
                    lex = class_lex(word);
                    CHECK_LEX(lex, _TG_STRING);
                    name = word;

                    word = read_word(f);
                    lex = class_lex(word);

                    switch(lex)
                    {
                    case _TG_COLON:
                    {
                        /* Device definition */
                        m_server.instance_name = family;
                        vector<string> values = parse_resource_value(f);
                        lex = class_lex(word);
                        // TANGO_LOG << "Class name : " << name << endl;
                        un_class = new t_tango_class;
                        un_class->name = name;
                        m_server.classes.push_back(un_class);
                        if(equalsIgnoreCase(member, "device"))
                        {
                            /* Device definition */
                            for(unsigned int n = 0; n < values.size(); n++)
                            {
                                // TANGO_LOG << "    Device: <" << values[n] << ">" << endl;
                                un_device = new t_device;
                                un_device->name = values[n];
                                m_server.devices.push_back(un_device);
                                un_class->devices.push_back(un_device);
                            }
                        }
                    }
                    break;

                    case _TG_ARROW:
                    {
                        /* We have an attribute property definition */
                        word = read_word(f);
                        lex = class_lex(word);
                        CHECK_LEX(lex, _TG_STRING);
                        prop_name = word;
                        // TANGO_LOG << "Attribute property: " << prop_name << endl;

                        /* jump : */
                        word = read_word(f);
                        lex = class_lex(word);
                        CHECK_LEX(lex, _TG_COLON);

                        /* Resource value */
                        vector<string> values = parse_resource_value(f);
                        lex = class_lex(word);

                        /* Device attribute definition */

                        // TANGO_LOG << "    " << domain << "/" << family << "/" << member << endl;
                        string device_name = domain + "/" + family + "/" + member;
                        t_device *d = search_device(m_server, device_name);

                        t_attribute_property *un_dev_attr_prop = search_dev_attr_prop(d, name);
                        if(un_dev_attr_prop == nullptr)
                        {
                            un_dev_attr_prop = new t_attribute_property;
                            un_dev_attr_prop->attribute_name = name;
                            d->attribute_properties.push_back(un_dev_attr_prop);
                        }
                        t_property *prop = new t_property;
                        prop->name = prop_name;
                        for(unsigned int n = 0; n < values.size(); n++)
                        {
                            // TANGO_LOG << "     <" << values[n] << ">" << endl;
                            prop->value.push_back(values[n]);
                        }
                        un_dev_attr_prop->properties.push_back(prop);
                    }
                    break;

                    default:
                    {
                        TangoSys_MemStream desc;
                        desc << "COLON or -> expected at line " << StartLine;
                        return desc.str();
                    }
                    }
                    break;

                case _TG_ARROW:
                {
                    /* We have a device property or attribute class definition */

                    word = read_word(f);
                    lex = class_lex(word);
                    CHECK_LEX(lex, _TG_STRING);
                    prop_name = word;

                    /* jump : */
                    word = read_word(f);
                    lex = class_lex(word);
                    CHECK_LEX(lex, _TG_COLON);

                    /* Resource value */
                    vector<string> values = parse_resource_value(f);
                    lex = class_lex(word);

                    if(equalsIgnoreCase(domain, "class"))
                    {
                        /* Class attribute property definition */
                        // TANGO_LOG << "Class attribute property definition" << endl;
                        // TANGO_LOG << "      family,member,prop_name,values :" << family <<","<<member<< ","
                        // <<prop_name<<","<< endl;
                        t_tango_class *c = search_class(m_server, family);

                        if(c != nullptr)
                        {
                            t_attribute_property *un_class_attr_prop = search_class_attr_prop(c, member);
                            if(un_class_attr_prop == nullptr)
                            {
                                un_class_attr_prop = new t_attribute_property;
                                un_class_attr_prop->attribute_name = member;
                                c->attribute_properties.push_back(un_class_attr_prop);
                            }
                            t_property *prop = new t_property;
                            prop->name = prop_name;
                            for(unsigned int n = 0; n < values.size(); n++)
                            {
                                // TANGO_LOG << "     <" << values[n] << ">" << endl;
                                prop->value.push_back(values[n]);
                            }
                            un_class_attr_prop->properties.push_back(prop);

                            // put_tango_class_attr_prop(family,member,prop_name,values);
                        }
                    }
                    else
                    {
                        /* Device property definition */
                        // TANGO_LOG << "Device property definition " << prop_name << endl;
                        // TANGO_LOG << "    " << domain << "/" << family << "/" << member << endl;
                        string device_name = domain + "/" + family + "/" + member;
                        t_device *d = search_device(m_server, device_name);

                        if(d != nullptr)
                        {
                            t_property *un_dev_prop = new t_property;
                            un_dev_prop->name = prop_name;
                            for(unsigned int n = 0; n < values.size(); n++)
                            {
                                // TANGO_LOG << "     <" << values[n] << ">" << endl;
                                un_dev_prop->value.push_back(values[n]);
                            }
                            d->properties.push_back(un_dev_prop);
                        }
                    }
                }
                break;

                default:
                {
                    TangoSys_MemStream desc;
                    desc << "SLASH or -> expected at line " << StartLine;
                    return desc.str();
                }
                }
                break;

            case _TG_ARROW:
            {
                /* We have a class property */
                /* Member */
                word = read_word(f);
                lex = class_lex(word);
                CHECK_LEX(lex, _TG_STRING);
                member = word;
                word = read_word(f);
                lex = class_lex(word);

                /* Resource value */
                vector<string> values = parse_resource_value(f);
                lex = class_lex(word);

                /* Class resource */
                if(equalsIgnoreCase(domain, "class"))
                {
                    // TANGO_LOG << "Tango resource class " << endl;
                    {
                        un_class = search_class(m_server, family);
                        if(un_class != nullptr)
                        {
                            t_property *un_prop = new t_property;
                            un_prop->name = member;
                            // TANGO_LOG << "Proprieta : " << member << endl;
                            for(unsigned int n = 0; n < values.size(); n++)
                            {
                                // TANGO_LOG << "    " << member << "[" << n << "] = " << values[n] << endl;
                                un_prop->value.push_back(values[n]);
                            }
                            un_class->properties.push_back(un_prop);
                        }
                    }
                }
                else if(equalsIgnoreCase(domain, "free"))
                {
                    t_free_object *obj = search_free_object(m_server, family);

                    // We add free objects on demand.
                    if(obj == nullptr)
                    {
                        obj = new t_free_object;
                        obj->name = family;
                        m_server.free_objects.push_back(obj);
                    }

                    t_property *prop = new t_property;
                    prop->name = member;

                    for(unsigned int n = 0; n < values.size(); n++)
                    {
                        prop->value.push_back(values[n]);
                    }
                    obj->properties.push_back(prop);
                }
                else
                {
                    return "Invlalid class property syntax on " + domain + "/" + family + "/" + member;
                }
            }
            break;

            default:
            {
                TangoSys_MemStream desc;
                desc << "SLASH or -> expected at line " << StartLine;
                return desc.str();
            }
            }
            break;

        default:
        {
            TangoSys_MemStream desc;
            desc << "Invalid resource name get  instead of STRING al line " << StartLine;
            return desc.str();
        }
        }

        eof = (word == lexical_word_null);
    }

    f.close();
    return "";
}

void FileDatabase::display()
{
    TANGO_LOG << " ************************** " << endl;
    TANGO_LOG << "server = " << m_server.name << endl;
    for(unsigned int i = 0; i < m_server.classes.size(); i++)
    {
        unsigned int j;
        TANGO_LOG << "    class = " << m_server.classes[i]->name << endl;
        for(j = 0; j < m_server.classes[i]->devices.size(); j++)
        {
            unsigned int k;
            TANGO_LOG << "        device = " << m_server.classes[i]->devices[j]->name << endl;
            for(k = 0; k < m_server.classes[i]->devices[j]->properties.size(); k++)
            {
                TANGO_LOG << "            proper = " << m_server.classes[i]->devices[j]->properties[k]->name
                          << "  value: " << endl;
                for(unsigned int l = 0; l < m_server.classes[i]->devices[j]->properties[k]->value.size(); l++)
                {
                    TANGO_LOG << "                 value[" << l
                              << "] = " << m_server.classes[i]->devices[j]->properties[k]->value[l] << endl;
                }
            }
            for(k = 0; k < m_server.classes[i]->devices[j]->attribute_properties.size(); k++)
            {
                TANGO_LOG << "            attribute  = "
                          << m_server.classes[i]->devices[j]->attribute_properties[k]->attribute_name << endl;
                for(unsigned int l = 0; l < m_server.classes[i]->devices[j]->attribute_properties[k]->properties.size();
                    l++)
                {
                    TANGO_LOG << "                 property[" << l
                              << "] = " << m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->name
                              << endl;
                    for(unsigned int m = 0;
                        m < m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->value.size();
                        m++)
                    {
                        TANGO_LOG << "                    value[" << m << "] = "
                                  << m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->value[m]
                                  << endl;
                    }
                }
            }
        }
        for(j = 0; j < m_server.classes[i]->properties.size(); j++)
        {
            TANGO_LOG << "        proper = " << m_server.classes[i]->properties[j]->name << "  value: " << endl;
            for(unsigned int l = 0; l < m_server.classes[i]->properties[j]->value.size(); l++)
            {
                TANGO_LOG << "                 value[" << l << "] = " << m_server.classes[i]->properties[j]->value[l]
                          << endl;
            }
        }
    }
}

string FileDatabase ::get_display()
{
    ostringstream ost;
    ost << " ************************** " << endl;
    ost << "server = " << m_server.name << endl;
    for(unsigned int i = 0; i < m_server.classes.size(); i++)
    {
        unsigned int j;
        ost << "    class = " << m_server.classes[i]->name << endl;
        for(j = 0; j < m_server.classes[i]->devices.size(); j++)
        {
            unsigned int k;
            ost << "        device = " << m_server.classes[i]->devices[j]->name << endl;
            for(k = 0; k < m_server.classes[i]->devices[j]->properties.size(); k++)
            {
                ost << "            proper = " << m_server.classes[i]->devices[j]->properties[k]->name
                    << "  value: " << endl;
                for(unsigned int l = 0; l < m_server.classes[i]->devices[j]->properties[k]->value.size(); l++)
                {
                    ost << "                 value[" << l
                        << "] = " << m_server.classes[i]->devices[j]->properties[k]->value[l] << endl;
                }
            }
            for(k = 0; k < m_server.classes[i]->devices[j]->attribute_properties.size(); k++)
            {
                ost << "            attribute  = "
                    << m_server.classes[i]->devices[j]->attribute_properties[k]->attribute_name << endl;
                for(unsigned int l = 0; l < m_server.classes[i]->devices[j]->attribute_properties[k]->properties.size();
                    l++)
                {
                    ost << "                 property[" << l
                        << "] = " << m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->name
                        << endl;
                    for(unsigned int m = 0;
                        m < m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->value.size();
                        m++)
                    {
                        ost << "                    value[" << m << "] = "
                            << m_server.classes[i]->devices[j]->attribute_properties[k]->properties[l]->value[m]
                            << endl;
                    }
                }
            }
        }
        for(j = 0; j < m_server.classes[i]->properties.size(); j++)
        {
            ost << "        proper = " << m_server.classes[i]->properties[j]->name << "  value: " << endl;
            for(unsigned int l = 0; l < m_server.classes[i]->properties[j]->value.size(); l++)
            {
                ost << "                 value[" << l << "] = " << m_server.classes[i]->properties[j]->value[l] << endl;
            }
        }
    }
    return ost.str();
}

static std::string escape_double_quote(const std::string &value)
{
    std::string escaped_value;
    escaped_value.reserve(value.size());
    for(const auto c : value)
    {
        if(c == '"')
        {
            escaped_value.push_back('\\');
        }
        escaped_value.push_back(c);
    }
    return escaped_value;
}

static void write_string_value(const std::string &value, std::ostream &out)
{
    bool has_space = value.find(' ') != string::npos;
    bool has_newline = value.find('\n') != string::npos;
    bool has_double_quotes = value.find('"') != string::npos;
    if(has_space || has_newline || has_double_quotes)
    {
        out << "\"";
    }
    out << (has_double_quotes ? escape_double_quote(value) : value);
    if(has_space || has_newline || has_double_quotes)
    {
        out << "\"";
    }
}

void FileDatabase ::write_file()
{
    ofstream f;
    string f_name(filename);
    /*
    string::size_type pos;
    if ((pos = f_name.rfind('/')) == string::npos)
        f_name = "_" + f_name;
    else
        f_name.insert(pos + 1,"_",1);
    */

    f.open(f_name.c_str());
    vector<t_tango_class *>::const_iterator it;
    for(it = m_server.classes.begin(); it != m_server.classes.end(); ++it)
    {
        f << m_server.name << "/" << m_server.instance_name << "/DEVICE/" << (*it)->name << ": ";
        int margin = m_server.name.size() + 1 + m_server.instance_name.size() + 8 + (*it)->name.size() + 2;
        string margin_s(margin, ' ');
        auto iterator_d = (*it)->devices.begin();
        f << "\"" << (*iterator_d)->name << "\"";
        ++iterator_d;
        for(auto itd = iterator_d; itd != (*it)->devices.end(); ++itd)
        {
            f << ",\\" << endl;
            f << margin_s << "\"" << (*itd)->name << "\"";
        }
        f << endl;
    }
    f << endl;

    for(it = m_server.classes.begin(); it != m_server.classes.end(); ++it)
    {
        f << "#############################################" << endl;
        f << "# CLASS " << (*it)->name << endl;
        f << endl;
        for(auto itp = (*it)->properties.begin(); itp != (*it)->properties.end(); ++itp)
        {
            f << "CLASS/" << (*it)->name << "->" << (*itp)->name << ": ";
            int margin = 6 + (*it)->name.size() + 2 + (*itp)->name.size() + 2;
            string margin_s(margin, ' ');
            auto iterator_s = (*itp)->value.begin();
            if((*iterator_s).length() == 0)
            {
                (*iterator_s)[0] = ' ';
            }
            if(iterator_s != (*itp)->value.end())
            {
                // f << "\"" << (*iterator_s) << "\"";
                if((*iterator_s).length() == 0)
                {
                    f << "\"\"";
                }
                else
                {
                    write_string_value(*iterator_s, f);
                }
                ++iterator_s;
                for(auto its = iterator_s; its != (*itp)->value.end(); ++its)
                {
                    f << ",\\" << endl;
                    f << margin_s;
                    write_string_value(*its, f);
                }
            }
            f << endl;
        }
        f << endl;
        f << "# CLASS " << (*it)->name << " attribute properties" << endl;
        f << endl;
        for(auto itap = (*it)->attribute_properties.begin(); itap != (*it)->attribute_properties.end(); ++itap)
        {
            for(auto itp = (*itap)->properties.begin(); itp != (*itap)->properties.end(); ++itp)
            {
                f << "CLASS/" << (*it)->name << "/" << (*itap)->attribute_name << "->" << (*itp)->name << ": ";
                int margin = 6 + (*it)->name.size() + 1 + (*itap)->attribute_name.size() + 2 + (*itp)->name.size() + 2;
                auto iterator_s = (*itp)->value.begin();
                if(iterator_s != (*itp)->value.end())
                {
                    write_string_value(*iterator_s, f);
                    ++iterator_s;
                    for(auto its = iterator_s; its != (*itp)->value.end(); ++its)
                    {
                        f << ",\\" << endl;
                        string margin_s(margin, ' ');
                        f << margin_s;
                        write_string_value(*its, f);
                    }
                }
                f << endl;
            }
        }
        f << endl;
    }
    f << endl;
    for(auto ite = m_server.devices.begin(); ite != m_server.devices.end(); ++ite)
    {
        f << "# DEVICE " << (*ite)->name << " properties " << endl << endl;
        for(auto itp = (*ite)->properties.begin(); itp != (*ite)->properties.end(); ++itp)
        {
            f << (*ite)->name << "->" << (*itp)->name << ": ";
            auto iterator_s = (*itp)->value.begin();
            if(iterator_s != (*itp)->value.end())
            {
                int margin = (*ite)->name.size() + 1 + (*itp)->name.size() + 2;
                write_string_value(*iterator_s, f);
                ++iterator_s;
                for(auto its = iterator_s; its != (*itp)->value.end(); ++its)
                {
                    f << ",\\" << endl;
                    string margin_s(margin, ' ');
                    f << margin_s;
                    write_string_value(*its, f);
                }
            }
            f << endl;
        }
        f << endl;
        f << "# DEVICE " << (*ite)->name << " attribute properties" << endl << endl;
        for(auto itap = (*ite)->attribute_properties.begin(); itap != (*ite)->attribute_properties.end(); ++itap)
        {
            for(auto itp = (*itap)->properties.begin(); itp != (*itap)->properties.end(); ++itp)
            {
                f << (*ite)->name << "/" << (*itap)->attribute_name << "->" << (*itp)->name << ": ";
                int margin = (*ite)->name.size() + 1 + (*itap)->attribute_name.size() + 2 + (*itp)->name.size() + 2;
                auto iterator_s = (*itp)->value.begin();
                if(iterator_s != (*itp)->value.end())
                {
                    write_string_value(*iterator_s, f);
                    ++iterator_s;
                    for(auto its = iterator_s; its != (*itp)->value.end(); ++its)
                    {
                        f << ",\\" << endl;
                        string margin_s(margin, ' ');
                        f << margin_s;
                        write_string_value(*its, f);
                    }
                }
                f << endl;
            }
        }
    }

    f << "#############################################\n";
    f << "# FREE OBJECT attributes\n\n";
    for(auto *obj : m_server.free_objects)
    {
        for(auto *prop : obj->properties)
        {
            f << "FREE/" << obj->name << "->" << prop->name << ": ";
            int margin = 5 + obj->name.size() + 2 + prop->name.size() + 2;
            string margin_s(margin, ' ');
            auto its = prop->value.begin();
            if(its != prop->value.end())
            {
                write_string_value(*its, f);
                ++its;
                for(; its != prop->value.end(); ++its)
                {
                    f << ",\\" << endl;
                    f << margin_s;
                    write_string_value(*its, f);
                }
            }
            f << "\n";
        }
    }

    f.close();
}

CORBA::Any_var FileDatabase ::DbGetDeviceProperty(CORBA::Any &send)
{
    auto *data_out = new DevVarStringArray;
    const Tango::DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetDeviceProperty" << endl;

    send >>= data_in;

    CORBA::Any_var any;
    any = new CORBA::Any();
    int index = 0;

    data_out->length(2);
    (*data_out)[0] = Tango::string_dup((*data_in)[0]);
    index++;
    auto num_prop = data_in->length() - 1;
    (*data_out)[index] = to_corba_string(num_prop);
    index++;

    if(data_in->length() >= 2)
    {
        unsigned long nb_defined_dev = m_server.devices.size();
        unsigned long i;
        int seq_length = 2;

        for(i = 0; i < nb_defined_dev; i++)
        {
            if(equalsIgnoreCase((*data_in)[0].in(), m_server.devices[i]->name))
            {
                for(unsigned int j = 1; j < data_in->length(); j++)
                {
                    unsigned long m;

                    unsigned long nb_defined_prop = m_server.devices[i]->properties.size();
                    for(m = 0; m < nb_defined_prop; m++)
                    {
                        // if ( strcmp((*data_in)[j], m_server.devices[i]->properties[m]->name.c_str()) == 0 )
                        if(equalsIgnoreCase((*data_in)[j].in(), m_server.devices[i]->properties[m]->name))
                        {
                            int num_val = 0;

                            num_prop++;
                            num_val = m_server.devices[i]->properties[m]->value.size();
                            seq_length = seq_length + 2 + m_server.devices[i]->properties[m]->value.size();
                            data_out->length(seq_length);
                            (*data_out)[index] = Tango::string_dup(m_server.devices[i]->properties[m]->name.c_str());
                            index++;
                            (*data_out)[index] = to_corba_string(num_val);
                            index++;
                            for(int k = 0; k < num_val; k++)
                            {
                                (*data_out)[index] =
                                    Tango::string_dup(m_server.devices[i]->properties[m]->value[k].c_str());
                                index++;
                            }
                            break;
                        }
                    }

                    if(m == nb_defined_prop)
                    {
                        seq_length = seq_length + 3;
                        data_out->length(seq_length);
                        (*data_out)[index] = Tango::string_dup((*data_in)[j].in());
                        index++;
                        (*data_out)[index] = Tango::string_dup("0");
                        index++;
                        (*data_out)[index] = Tango::string_dup(" ");
                        index++;
                    }
                }
                break;
            }
        }

        if(i == nb_defined_dev)
        {
            for(i = 0; i < num_prop; i++)
            {
                seq_length = seq_length + 3;
                data_out->length(seq_length);
                (*data_out)[index] = Tango::string_dup((*data_in)[i + 1].in());
                index++;
                (*data_out)[index] = Tango::string_dup("0");
                index++;
                (*data_out)[index] = Tango::string_dup(" ");
                index++;
            }
        }
    }

    any.inout() <<= data_out;

    // for (unsigned int i = 0; i < data_out->length(); i++)
    //     TANGO_LOG << "data_out[" << i << "] = " << (*data_out)[i] << endl;

    return any;
}

CORBA::Any_var FileDatabase ::DbPutDeviceProperty(CORBA::Any &send)
{
    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbPutDeviceProperty" << endl;

    CORBA::Any_var any = new CORBA::Any;

    const Tango::DevVarStringArray *data_in = nullptr;
    unsigned int n_properties = 0;
    int n_values = 0;

    send >>= data_in;

    if((*data_in).length() > 1)
    {
        int index = 0;
        std::vector<t_device *>::iterator it;
        it = find_if(m_server.devices.begin(), m_server.devices.end(), hasName<t_device>(string((*data_in)[index])));
        index++;
        if(it == m_server.devices.end())
        {
            TANGO_LOG_DEBUG << "Nome device " << (*data_in)[0] << " non trovato. " << endl;
            return any;
        }
        t_device &device_trovato = *(*(it));

        sscanf((*data_in)[1], "%6u", &n_properties);
        index++;
        for(unsigned int i = 0; i < n_properties; i++)
        {
            std::vector<t_property *>::iterator prop_it;
            prop_it = find_if(device_trovato.properties.begin(),
                              device_trovato.properties.end(),
                              hasName<t_property>(string((*data_in)[index])));
            index++;
            if(prop_it != device_trovato.properties.end())
            {
                /* we found a  property */
                t_property &prop = (*(*prop_it));
                sscanf((*data_in)[index], "%6d", &n_values);
                index++;
                prop.value.resize(n_values);
                for(int j = 0; j < n_values; j++)
                {
                    prop.value[j] = (*data_in)[index];
                    index++;
                }
            }
            else
            {
                /* it is a new property */
                t_property *temp_property = new t_property;
                temp_property->name = (*data_in)[index - 1];
                sscanf((*data_in)[index], "%6d", &n_values);
                index++;
                for(int j = 0; j < n_values; j++)
                {
                    temp_property->value.emplace_back((*data_in)[index]);
                    index++;
                }
                device_trovato.properties.push_back(temp_property);
            }
        }
    }

    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbDeleteDeviceProperty(CORBA::Any &send)
{
    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbDeleteDeviceProperty" << endl;

    const Tango::DevVarStringArray *data_in = nullptr;

    send >>= data_in;

    // for(unsigned int i = 0; i < (*data_in).length(); i++)
    //     TANGO_LOG << "(*data_in)[" << i << "] = " << (*data_in)[i] << endl;

    std::vector<t_device *>::iterator it;
    it = find_if(m_server.devices.begin(), m_server.devices.end(), hasName<t_device>(string((*data_in)[0])));

    if(it != m_server.devices.end())
    {
        for(unsigned int i = 1; i < (*data_in).length(); i++)
        {
            t_device &device_trovato = *(*(it));
            std::vector<t_property *>::iterator itp;
            itp = find_if(device_trovato.properties.begin(),
                          device_trovato.properties.end(),
                          hasName<t_property>(string((*data_in)[i])));

            if(itp != device_trovato.properties.end())
            {
                delete *itp;
                device_trovato.properties.erase(itp, itp + 1);
            }
        }
    }

    CORBA::Any_var any = new CORBA::Any;

    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbGetDeviceAttributeProperty(CORBA::Any &send)
{
    auto *data_out = new DevVarStringArray;
    const Tango::DevVarStringArray *data_in = nullptr;

    CORBA::Any_var any;
    any = new CORBA::Any();

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetDeviceAttributeProperty" << endl;

    send >>= data_in;

    // for(unsigned int i = 0; i < data_in->length(); i++)
    //     TANGO_LOG << "send[" << i << "] = " << (*data_in)[i] << endl;

    int index = 0;
    data_out->length(2);
    (*data_out)[0] = Tango::string_dup((*data_in)[0]);
    index++;
    auto num_attr = data_in->length() - 1;
    (*data_out)[index] = to_corba_string(num_attr);
    index++;

    std::vector<t_device *>::iterator dev_it;
    dev_it = find_if(m_server.devices.begin(), m_server.devices.end(), hasName<t_device>(string((*data_in)[0])));
    if(dev_it != m_server.devices.end())
    {
        // TANGO_LOG << "Device " << (*dev_it)->name << " trovato." << endl;
        for(unsigned int k = 0; k < num_attr; k++)
        {
            data_out->length(index + 2);
            (*data_out)[index] = Tango::string_dup((*data_in)[k + 1]);
            index++; // attribute name
            (*data_out)[index] = Tango::string_dup("0");
            index++; // number of properties
            for(unsigned int j = 0; j < (*dev_it)->attribute_properties.size(); j++)
            {
                if(equalsIgnoreCase((*dev_it)->attribute_properties[j]->attribute_name, (*data_in)[k + 1].in()))
                {
                    // TANGO_LOG << "Proprieta' " << (*dev_it)->attribute_properties[j]->attribute_name << " trovata."
                    // << endl;
                    auto num_prop = (*dev_it)->attribute_properties[j]->properties.size();

                    (*data_out)[index - 1] = to_corba_string(num_prop);

                    for(unsigned int l = 0; l < num_prop; l++)
                    {
                        data_out->length(index + 1 + 1 +
                                         (*dev_it)->attribute_properties[j]->properties[l]->value.size());
                        (*data_out)[index] =
                            Tango::string_dup((*dev_it)->attribute_properties[j]->properties[l]->name.c_str());
                        index++;
                        auto num_attr_prop = (*dev_it)->attribute_properties[j]->properties[l]->value.size();
                        (*data_out)[index] = to_corba_string(num_attr_prop);
                        index++;

                        for(unsigned int ii = 0; ii < (*dev_it)->attribute_properties[j]->properties[l]->value.size();
                            ii++)
                        {
                            // TANGO_LOG << ii << " = " <<
                            // (*dev_it)->attribute_properties[j]->properties[l]->value[ii].c_str() << endl;
                            (*data_out)[index] =
                                Tango::string_dup((*dev_it)->attribute_properties[j]->properties[l]->value[ii].c_str());
                            index++;
                        }
                    }
                }
            }
        }
    }
    else
    {
        data_out->length(index + (2 * num_attr));
        for(unsigned int i = 0; i < num_attr; i++)
        {
            (*data_out)[index] = Tango::string_dup((*data_in)[i + 1]);
            index++;
            (*data_out)[index] = Tango::string_dup("0");
            index++;
        }
    }

    // for(unsigned int i = 0; i < data_out->length(); i++)
    //     TANGO_LOG << "data_out[" << i << "] = " << (*data_out)[i] << endl;

    any.inout() <<= data_out;

    return any;
}

CORBA::Any_var FileDatabase ::DbPutDeviceAttributeProperty(CORBA::Any &send)
{
    const Tango::DevVarStringArray *data_in = nullptr;
    unsigned int num_prop = 0;
    unsigned int num_attr = 0;
    unsigned int num_vals = 0;

    CORBA::Any_var any = new CORBA::Any;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbPutDeviceAttributeProperty" << endl;

    send >>= data_in;

    unsigned int index = 0;

    std::vector<t_device *>::iterator dev_it;
    dev_it = find_if(m_server.devices.begin(), m_server.devices.end(), hasName<t_device>(string((*data_in)[index])));
    index++;
    if(dev_it != m_server.devices.end())
    {
        sscanf((*data_in)[index], "%6u", &num_attr);
        index++;
        for(unsigned int j = 0; j < num_attr; j++)
        {
            t_attribute_property *temp_attribute_property;
            std::vector<t_attribute_property *>::iterator attr_prop_it;
            attr_prop_it = find_if((*dev_it)->attribute_properties.begin(),
                                   (*dev_it)->attribute_properties.end(),
                                   hasAttributeName<t_attribute_property>(string((*data_in)[index])));
            if(attr_prop_it != (*dev_it)->attribute_properties.end())
            {
                temp_attribute_property = (*attr_prop_it);
            }
            else
            {
                // the property is not yet in the file: we add it
                temp_attribute_property = new t_attribute_property;
                temp_attribute_property->attribute_name = string((*data_in)[index]);
                (*dev_it)->attribute_properties.push_back(temp_attribute_property);
            }
            // if (equalsIgnoreCase((*dev_it)->attribute_properties[j]->attribute_name, (*data_in)[index].in()))

            index++;
            sscanf((*data_in)[index], "%6u", &num_prop);
            index++;
            for(unsigned int i = 0; i < num_prop; i++)
            {
                bool exist = false;
                for(unsigned int k = 0; k < temp_attribute_property->properties.size(); k++)
                {
                    if(equalsIgnoreCase(temp_attribute_property->properties[k]->name, (*data_in)[index].in()))
                    {
                        index++;
                        temp_attribute_property->properties[k]->value.erase(
                            temp_attribute_property->properties[k]->value.begin(),
                            temp_attribute_property->properties[k]->value.begin() +
                                temp_attribute_property->properties[k]->value.size());
                        sscanf((*data_in)[index], "%6u", &num_vals);
                        index++;
                        for(unsigned int n = 0; n < num_vals; n++)
                        {
                            temp_attribute_property->properties[k]->value.emplace_back((*data_in)[index]);
                            index++;
                        }

                        // (*dev_it)->attribute_properties[j]->properties[k]->value.push_back( string((*data_in)[index])
                        // );index++;
                        if(index >= data_in->length())
                        {
                            write_file();
                            return any;
                        }
                        exist = true;
                    }
                }
                if(!exist)
                {
                    t_property *new_prop = new t_property;
                    new_prop->name = (*data_in)[index];
                    index++;

                    sscanf((*data_in)[index], "%6u", &num_vals);
                    index++;
                    for(unsigned int n = 0; n < num_vals; n++)
                    {
                        new_prop->value.emplace_back((*data_in)[index]);
                        index++;
                    }

                    temp_attribute_property->properties.push_back(new_prop);
                    if(index >= data_in->length())
                    {
                        write_file();
                        return any;
                    }
                }
            }
        }
    }
    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbDeleteDeviceAttributeProperty(CORBA::Any &send)
{
    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbDeleteDeviceAttributeProperty" << endl;

    const Tango::DevVarStringArray *data_in = nullptr;

    send >>= data_in;

    // for(unsigned int i = 0; i < (*data_in).length(); i++)
    //     TANGO_LOG << "(*data_in)[" << i << "] = " << (*data_in)[i] << endl;

    std::vector<t_device *>::iterator it;
    it = find_if(m_server.devices.begin(), m_server.devices.end(), hasName<t_device>(string((*data_in)[0])));

    if(it != m_server.devices.end())
    {
        t_device &device_trovato = *(*(it));
        for(unsigned int j = 0; j < device_trovato.attribute_properties.size(); j++)
        {
            if(equalsIgnoreCase(device_trovato.attribute_properties[j]->attribute_name, (*data_in)[1].in()))
            {
                for(unsigned int m = 2; m < (*data_in).length(); m++)
                {
                    std::vector<t_property *>::iterator itp;
                    itp = find_if(device_trovato.attribute_properties[j]->properties.begin(),
                                  device_trovato.attribute_properties[j]->properties.end(),
                                  hasName<t_property>(string((*data_in)[m])));

                    if(itp != device_trovato.attribute_properties[j]->properties.end())
                    {
                        delete *itp;
                        device_trovato.attribute_properties[j]->properties.erase(itp, itp + 1);
                    }
                }
            }
        }
    }

    CORBA::Any_var any = new CORBA::Any;
    write_file();
    return any;
}

/*
 * FileDatabase :: DbGetClassProperty(CORBA::Any& send)
 * @param send CORBA::Any Argin representing a DevVarStringArray
 * Argin description:
 * Str[0] = Tango class
 * Str[1] = Property #1 name
 * Str[2] = Property #2 name
 * ...
 * @return DevVarStringArray as CORBA::Any
 * Argout description:
 * Str[0] = Tango class
 * Str[1] = Number of properties
 * Str[2] = Property #1 name
 * Str[3] = Property #1 value size (number of value elements, 1 for scalar case, 0 if class property not found)
 * Str[4] = Property #1 value
 * Str[5] = Property #1 value (array case)
 * ...
 * Str[n] = Property #1 value (array case)
 * Str[n+1] = Property #2 name
 * Str[n+2] = Property #2 value size (number of value elements, 1 for scalar case, 0 if class property not found)
 * Str[n+3] = Property #2 value
 * Str[n+4] = Property #2 value (array case)
 * ...
 */
CORBA::Any_var FileDatabase ::DbGetClassProperty(CORBA::Any &send)
{
    auto *data_out = new DevVarStringArray;
    const Tango::DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetClassProperty" << endl;

    send >>= data_in;

    CORBA::Any_var any = new CORBA::Any();
    int index = 0;
    int seq_length = 2;

    data_out->length(2);
    (*data_out)[0] = Tango::string_dup((*data_in)[0]);
    index++;
    const auto num_prop = data_in->length() - 1;
    (*data_out)[index] = to_corba_string(num_prop);
    index++;

    unsigned long nb_classes_defined = m_server.classes.size();
    unsigned long i;

    for(i = 0; i < nb_classes_defined; i++)
    {
        if(equalsIgnoreCase((*data_in)[0].in(), m_server.classes[i]->name))
        {
            // m_server.classes[i] is the class we are looking for
            for(unsigned int j = 1; j < (*data_in).length();
                j++) // at index 0 is the name of the class, property names are following
            {
                unsigned long nb_prop_defined = m_server.classes[i]->properties.size();
                unsigned long m;
                for(m = 0; m < nb_prop_defined; m++)
                {
                    if(equalsIgnoreCase((*data_in)[j].in(), m_server.classes[i]->properties[m]->name))
                    {
                        auto num_val = m_server.classes[i]->properties[m]->value.size();
                        seq_length = seq_length + 2 + num_val;
                        (*data_out).length(seq_length);
                        (*data_out)[index] = Tango::string_dup((*data_in)[j]);
                        index++;
                        (*data_out)[index] = to_corba_string(num_val);
                        index++;
                        for(unsigned int n = 0; n < num_val; n++)
                        {
                            (*data_out)[index] =
                                Tango::string_dup(m_server.classes[i]->properties[m]->value[n].c_str());
                            index++;
                        }
                        break;
                    }
                }

                if(m == nb_prop_defined)
                {
                    // The requested property does not exist in the specified class
                    seq_length = seq_length + 2;
                    data_out->length(seq_length);
                    // The requested property name is returned, followed by a 0,
                    // meaning the length of this property value is 0 ( <=> class property not found )
                    (*data_out)[index] = Tango::string_dup((*data_in)[j].in());
                    index++;
                    (*data_out)[index] = Tango::string_dup("0");
                    index++;
                }
            }
            break;
        }
    }

    if(i == nb_classes_defined)
    {
        for(i = 0; i < num_prop; i++)
        {
            seq_length = seq_length + 2;
            data_out->length(seq_length);
            (*data_out)[index] = Tango::string_dup((*data_in)[i + 1].in());
            index++;
            (*data_out)[index] = Tango::string_dup("0");
            index++;
            //            (*data_out)[index] = Tango::string_dup(" ");index++;
        }
    }

    any.inout() <<= data_out;

    TANGO_LOG_DEBUG << "FILEDATABASE: ending DbGetClassProperty" << endl;

    return any;
}

CORBA::Any_var FileDatabase ::DbPutClassProperty(CORBA::Any &send)
{
    CORBA::Any_var any = new CORBA::Any;
    const Tango::DevVarStringArray *data_in = nullptr;
    unsigned int n_properties = 0;
    int n_values = 0;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbPutClassProperty" << endl;

    send >>= data_in;

    if((*data_in).length() > 1)
    {
        unsigned int index = 0;
        std::vector<t_tango_class *>::iterator it;
        it = find_if(
            m_server.classes.begin(), m_server.classes.end(), hasName<t_tango_class>(string((*data_in)[index])));
        index++;
        if(it == m_server.classes.end())
        {
            TANGO_LOG_DEBUG << "Nome classe " << (*data_in)[0] << " non trovato. " << endl;
            return any;
        }
        t_tango_class &classe_trovata = *(*(it));

        sscanf((*data_in)[index], "%6u", &n_properties);
        index++;
        for(unsigned int i = 0; i < n_properties; i++)
        {
            std::vector<t_property *>::iterator prop_it;
            prop_it = find_if(classe_trovata.properties.begin(),
                              classe_trovata.properties.end(),
                              hasName<t_property>(string((*data_in)[index])));
            if(prop_it != classe_trovata.properties.end())
            {
                /* we found a  property */
                index++;
                t_property &prop = (*(*prop_it));
                sscanf((*data_in)[index], "%6d", &n_values);
                index++;
                prop.value.resize(n_values);
                for(int j = 0; j < n_values; j++)
                {
                    prop.value[j] = (*data_in)[index];
                    index++;
                    // db_data[i] >> prop.value;
                }
            }
            else
            {
                /* it is a new property */
                t_property *temp_property = new t_property;
                temp_property->name = (*data_in)[index];
                index++;
                sscanf((*data_in)[index], "%6d", &n_values);
                index++;
                for(int j = 0; j < n_values; j++)
                {
                    temp_property->value.emplace_back((*data_in)[index]);
                    index++;
                }
                classe_trovata.properties.push_back(temp_property);
                if(index >= data_in->length())
                {
                    write_file();
                    return any;
                }
            }
        }
    }

    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbDeleteClassProperty(CORBA::Any &send)
{
    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbDeleteClassProperty" << endl;

    const Tango::DevVarStringArray *data_in = nullptr;

    send >>= data_in;

    //    for(unsigned int i = 0; i < (*data_in).length(); i++)
    //        TANGO_LOG << "(*data_in)[" << i << "] = " << (*data_in)[i] << endl;

    std::vector<t_tango_class *>::iterator it;
    it = find_if(m_server.classes.begin(), m_server.classes.end(), hasName<t_tango_class>(string((*data_in)[0])));

    if(it != m_server.classes.end())
    {
        for(unsigned int i = 1; i < (*data_in).length(); i++)
        {
            t_tango_class &classe_trovata = *(*(it));
            std::vector<t_property *>::iterator itp;
            itp = find_if(classe_trovata.properties.begin(),
                          classe_trovata.properties.end(),
                          hasName<t_property>(string((*data_in)[i])));

            if(itp != classe_trovata.properties.end())
            {
                delete *itp;
                classe_trovata.properties.erase(itp, itp + 1);
            }
        }
    }

    CORBA::Any_var any = new CORBA::Any;
    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbGetClassAttributeProperty(CORBA::Any &send)
{
    CORBA::Any_var any = new CORBA::Any();
    auto *data_out = new DevVarStringArray;
    const Tango::DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetClassAttributeProperty" << endl;

    send >>= data_in;

    int index = 0;
    data_out->length(2);
    (*data_out)[0] = Tango::string_dup((*data_in)[0]);
    index++;
    auto num_attr = data_in->length() - 1;
    (*data_out)[1] = to_corba_string(num_attr);
    index++;

    std::vector<t_tango_class *>::iterator it;
    it = find_if(m_server.classes.begin(), m_server.classes.end(), hasName<t_tango_class>(string((*data_in)[0])));
    if(it == m_server.classes.end())
    {
        TANGO_LOG_DEBUG << "Nome classe " << (*data_in)[0] << " non trovato. " << endl;
        data_out->length(index + (num_attr * 2));
        for(unsigned int j = 0; j < num_attr; j++)
        {
            (*data_out)[index] = Tango::string_dup((*data_in)[j + 1]);
            index++;
            (*data_out)[index] = Tango::string_dup("0");
            index++;
        }
        any.inout() <<= data_out;

        return any;
    }
    t_tango_class &classe_trovata = *(*(it));

    for(unsigned int k = 0; k < num_attr; k++)
    {
        data_out->length(index + 2);
        (*data_out)[index] = Tango::string_dup((*data_in)[k + 1]);
        index++;
        (*data_out)[index] = Tango::string_dup("0");
        index++;

        for(unsigned int j = 0; j < classe_trovata.attribute_properties.size(); j++)
        {
            if(equalsIgnoreCase(classe_trovata.attribute_properties[j]->attribute_name, (*data_in)[k + 1].in()))
            {
                auto num_prop = classe_trovata.attribute_properties[j]->properties.size();
                // data_out->length(index + 2*num_prop);
                (*data_out)[index - 1] = to_corba_string(num_prop);
                for(unsigned int l = 0; l < classe_trovata.attribute_properties[j]->properties.size(); l++)
                {
                    data_out->length(index + 1 + 1 +
                                     classe_trovata.attribute_properties[j]->properties[l]->value.size());
                    (*data_out)[index] =
                        Tango::string_dup(classe_trovata.attribute_properties[j]->properties[l]->name.c_str());
                    index++;

                    auto num_attr_prop = classe_trovata.attribute_properties[j]->properties[l]->value.size();
                    (*data_out)[index] = to_corba_string(num_attr_prop);
                    index++;
                    //(*data_out)[index] =
                    // Tango::string_dup(classe_trovata.attribute_properties[j]->properties[l]->name.c_str());index++;
                    // string temp_value("");
                    if(classe_trovata.attribute_properties[j]->properties[l]->value.size() > 0)
                    {
                        // temp_value += classe_trovata.attribute_properties[j]->properties[l]->value[0];
                        for(unsigned int m = 0; m < classe_trovata.attribute_properties[j]->properties[l]->value.size();
                            m++)
                        {
                            (*data_out)[index] = Tango::string_dup(
                                classe_trovata.attribute_properties[j]->properties[l]->value[m].c_str());
                            index++;
                            // temp_value +=  "\n" + classe_trovata.attribute_properties[j]->properties[l]->value[m];
                        }
                    }
                }
            }
        }
    }

    // for(unsigned int i = 0; i < data_out->length(); i++)
    //     TANGO_LOG << "data_out[" << i << "] = " << (*data_out)[i] << endl;

    any.inout() <<= data_out;

    return any;
}

CORBA::Any_var FileDatabase ::DbPutClassAttributeProperty(CORBA::Any &send)
{
    CORBA::Any_var any = new CORBA::Any();
    const Tango::DevVarStringArray *data_in = nullptr;
    unsigned int num_attr = 0;
    unsigned int num_prop = 0;
    unsigned int num_vals = 0;
    unsigned int index = 0;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbPutClassAttributeProperty" << endl;

    send >>= data_in;

    std::vector<t_tango_class *>::iterator it;
    it = find_if(m_server.classes.begin(), m_server.classes.end(), hasName<t_tango_class>(string((*data_in)[index])));
    index++;
    if(it == m_server.classes.end())
    {
        TANGO_LOG_DEBUG << "FILEDATABASE:  DbPutClassAttributeProperty class " << string((*data_in)[0]) << " not found."
                        << endl;
    }
    else
    {
        sscanf((*data_in)[index], "%6u", &num_attr);
        index++;
        t_tango_class &classe_trovata = *(*(it));
        t_attribute_property *temp_attribute_property;
        std::vector<t_attribute_property *>::iterator attr_prop_it;

        for(unsigned int j = 0; j < num_attr; j++)
        {
            // search an attribute property for this attribute
            attr_prop_it = find_if(classe_trovata.attribute_properties.begin(),
                                   classe_trovata.attribute_properties.end(),
                                   hasAttributeName<t_attribute_property>(string((*data_in)[index])));
            if(attr_prop_it != classe_trovata.attribute_properties.end())
            {
                temp_attribute_property = (*attr_prop_it);
            }
            else
            {
                // the property is not yet in the file: we add it
                temp_attribute_property = new t_attribute_property;
                temp_attribute_property->attribute_name = string((*data_in)[index]);
                classe_trovata.attribute_properties.push_back(temp_attribute_property);
            }
            index++;
            sscanf((*data_in)[index], "%6u", &num_prop);
            index++;
            for(unsigned int i = 0; i < num_prop; i++)
            {
                bool exist = false;
                for(unsigned int k = 0; k < temp_attribute_property->properties.size(); k++)
                {
                    if(equalsIgnoreCase(temp_attribute_property->properties[k]->name, (*data_in)[index].in()))
                    {
                        index++;
                        temp_attribute_property->properties[k]->value.erase(
                            temp_attribute_property->properties[k]->value.begin(),
                            temp_attribute_property->properties[k]->value.begin() +
                                temp_attribute_property->properties[k]->value.size());
                        sscanf((*data_in)[index], "%6u", &num_vals);
                        index++;
                        for(unsigned int n = 0; n < num_vals; n++)
                        {
                            temp_attribute_property->properties[k]->value.emplace_back((*data_in)[index]);
                            index++;
                        }

                        //(*dev_it)->attribute_properties[j]->properties[k]->value.push_back( string((*data_in)[index])
                        //);index++;
                        if(index >= data_in->length())
                        {
                            write_file();
                            return any;
                        }
                        exist = true;
                    }
                }
                if(!exist)
                {
                    t_property *new_prop = new t_property;
                    new_prop->name = (*data_in)[index];
                    index++;

                    sscanf((*data_in)[index], "%6u", &num_vals);
                    index++;
                    for(unsigned int n = 0; n < num_vals; n++)
                    {
                        new_prop->value.emplace_back((*data_in)[index]);
                        index++;
                    }

                    temp_attribute_property->properties.push_back(new_prop);
                    if(index >= data_in->length())
                    {
                        write_file();
                        return any;
                    }
                }
            }
        }
    }

    write_file();
    return any;
}

CORBA::Any_var FileDatabase ::DbDeleteClassAttributeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceList(CORBA::Any &send)
{
    CORBA::Any_var any = new CORBA::Any();
    const Tango::DevVarStringArray *data_in = nullptr;
    auto *data_out = new DevVarStringArray;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetDeviceList" << endl;

    send >>= data_in;

    if(data_in->length() == 2)
    {
        if(equalsIgnoreCase((*data_in)[0].in(), m_server.name + "/" + m_server.instance_name))
        {
            unsigned int i;
            for(i = 0; i < m_server.classes.size(); i++)
            {
                // if ( strcmp((*data_in)[1], m_server.classes[i]->name.c_str()) == 0 )
                if(equalsIgnoreCase((*data_in)[1].in(), m_server.classes[i]->name))
                {
                    data_out->length(m_server.classes[i]->devices.size());
                    for(unsigned int j = 0; j < m_server.classes[i]->devices.size(); j++)
                    {
                        (*data_out)[j] = Tango::string_dup(m_server.classes[i]->devices[j]->name.c_str());
                    }
                    break;
                }
            }

            if(i == m_server.classes.size())
            {
                delete data_out;

                TangoSys_MemStream desc;
                desc << "File database: Can't find class " << (*data_in)[1];
                desc << " in file " << filename << "." << ends;
                TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
            }
        }
        else
        {
            delete data_out;

            TangoSys_MemStream desc;
            desc << "File database: Can't find device server " << (*data_in)[0];
            desc << " in file " << filename << "." << ends;
            TANGO_THROW_DETAILED_EXCEPTION(ApiConnExcept, API_DatabaseFileError, desc.str());
        }
    }

    any.inout() <<= data_out;
    return any;
}

CORBA::Any_var FileDatabase ::DbInfo(CORBA::Any &)
{
    CORBA::Any_var any = new CORBA::Any();

    auto generate_string = [](std::string prefix, CORBA::ULong size) -> char *
    {
        prefix += std::to_string(size);
        return Tango::string_dup(prefix.c_str());
    };

    auto prop_func = [](CORBA::ULong init, auto s) -> CORBA::ULong { return init + s->properties.size(); };

    auto prop_attr_func = [](CORBA::ULong init, auto s) -> CORBA::ULong
    { return init + s->attribute_properties.size(); };

    auto accumulate = [](auto const &vec, auto func) -> CORBA::ULong
    { return std::accumulate(std::begin(vec), std::end(vec), 0u, func); };

    auto *data_out = new DevVarStringArray;
    data_out->length(13);

    auto header = std::string("TANGO FileDatabase  ") + filename;
    (*data_out)[0] = Tango::string_dup(header.c_str());
    (*data_out)[1] = Tango::string_dup("");
    (*data_out)[2] = Tango::string_dup("Running since ----");
    (*data_out)[3] = Tango::string_dup("");
    (*data_out)[4] = generate_string("Devices defined = ", m_server.devices.size());
    (*data_out)[5] = generate_string("Devices exported = ", m_server.devices.size());
    (*data_out)[6] = Tango::string_dup("Device servers defined = 1");
    (*data_out)[7] = Tango::string_dup("Device servers exported = 1");
    (*data_out)[8] = Tango::string_dup("");
    (*data_out)[9] = generate_string("Class properties defined = ", accumulate(m_server.classes, prop_func));
    (*data_out)[10] = generate_string("Device properties defined = ", accumulate(m_server.devices, prop_func));
    (*data_out)[11] =
        generate_string("Class attribute properties defined = ", accumulate(m_server.classes, prop_attr_func));
    (*data_out)[12] =
        generate_string("Device attribute properties defined = ", accumulate(m_server.devices, prop_attr_func));

    any.inout() <<= data_out;

    return any;
}

CORBA::Any_var FileDatabase ::DbImportDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbExportDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbUnExportDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbAddDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbDeleteDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbAddServer(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbDeleteServer(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbExportServer(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbUnExportServer(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetServerInfo(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceMemberList(CORBA::Any &)
{
    CORBA::Any_var any = new CORBA::Any();

    auto *argout = new Tango::DevVarStringArray();
    argout->length(1);
    (*argout)[0] = Tango::string_dup("NoMember");
    any.inout() <<= argout;

    return any;
}

CORBA::Any_var FileDatabase ::DbGetDeviceWideList(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceExportedList(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceFamilyList(CORBA::Any &)
{
    CORBA::Any_var any = new CORBA::Any();

    auto *argout = new Tango::DevVarStringArray();
    argout->length(1);
    (*argout)[0] = Tango::string_dup("NoDevice");
    any.inout() <<= argout;

    return any;
}

CORBA::Any_var FileDatabase ::DbGetDeviceDomainList(CORBA::Any &)
{
    CORBA::Any_var any = new CORBA::Any();

    auto *argout = new Tango::DevVarStringArray();
    argout->length(1);
    (*argout)[0] = Tango::string_dup("NoDevice");
    any.inout() <<= argout;

    return any;
}

/**
 *	Command DbGetProperty related method
 *	Description: Get free object property
 *
 *	@param argin Str[0] = Object name
 *               Str[1] = Property name
 *               Str[n] = Property name
 *	@returns Str[0] = Object name
 *           Str[1] = Property number
 *           Str[2] = Property name
 *           Str[3] = Property value number (array case)
 *           Str[4] = Property value 1
 *           Str[n] = Property value n (array case)
 *           Str[n + 1] = Property name
 *           Str[n + 2] = Property value number (array case)
 *           Str[n + 3] = Property value 1
 *           Str[n + m] = Property value m
 */
CORBA::Any_var FileDatabase::DbGetProperty(CORBA::Any &send)
{
    CORBA::Any_var any = new CORBA::Any();

    const DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbGetProperty" << endl;

    if(!(send >>= data_in))
    {
        std::stringstream ss;
        ss << "Incorrect type passed to FileDatabase::DbGetProperty. Expecting "
           << tango_type_traits<DevVarStringArray>::type_value() << ", found " << detail::corba_any_to_type_name(send);
        TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, ss.str().c_str());
    }

    if(data_in->length() < 1)
    {
        std::stringstream ss;
        ss << "Invalid number of arguments passed to FileDatabase::DbGetProperty. Expecting at least 1, found "
           << data_in->length();
        TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, ss.str().c_str());
    }

    auto *data_out = new DevVarStringArray;

    const auto &free_objects = m_server.free_objects;
    const char *obj_name = (*data_in)[0].in();

    unsigned long num_prop = data_in->length() - 1;

    // Allocate space for the properties up-front to avoid too may allocations.
    //
    // Here we are allocating the minimum amount of space required for each
    // property (3) plus object and property number (2). That is, slots for:
    //
    //  - The name
    //  - The number of elements of the value
    //  - At least one element (this is expected even if the number of elements is
    //  zero)
    //
    // Later we allocate more space if we find that there are multiple elements
    // for a value.
    data_out->length(2 + (3 * num_prop));

    size_t out_index = 0;
    (*data_out)[out_index] = Tango::string_dup(obj_name);
    out_index++;
    (*data_out)[out_index] = to_corba_string(num_prop);
    out_index++;

    auto obj = std::find_if(free_objects.begin(), free_objects.end(), hasName<t_free_object>(obj_name));
    std::vector<t_property *> empty_prop_list;

    // If the free object isn't in the database, then we pretend we are
    // referencing a free object with no properties.  This is the same behaviour
    // as TangoDatabase.
    const auto &prop_list = obj != free_objects.end() ? (*obj)->properties : empty_prop_list;

    for(CORBA::ULong j = 1; j < data_in->length(); ++j)
    {
        const char *prop_name = (*data_in)[j].in();
        (*data_out)[out_index] = Tango::string_dup(prop_name);
        out_index++;

        auto prop = std::find_if(prop_list.begin(), prop_list.end(), hasName<t_property>(prop_name));

        if(prop == prop_list.end())
        {
            (*data_out)[out_index] = Tango::string_dup("0");
            out_index++;
            // Even though we say 0 elements here, we add a " ".  This is
            // inline with what TangoDatabase does when it cannot find the
            // property.
            (*data_out)[out_index] = Tango::string_dup(" ");
            out_index++;
        }
        else
        {
            const auto &value = (*prop)->value;
            size_t value_len = value.size();
            if(value_len > 1)
            {
                // Correct our assumption above
                data_out->length(data_out->length() + value_len - 1);
            }

            (*data_out)[out_index] = to_corba_string(value_len);
            out_index++;

            for(size_t i = 0; i < value_len; ++i)
            {
                (*data_out)[out_index] = Tango::string_dup(value[i].c_str());
                out_index++;
            }
        }
    }

    any.inout() <<= data_out;
    return any;
}

//--------------------------------------------------------
/**
 *	Command DbPutProperty related method
 *	Description: Create / Update free object property(ies)
 *
 *	@param argin Str[0] = Object name
 *               Str[1] = Property number
 *               Str[2] = Property name
 *               Str[3] = Property value number
 *               Str[4] = Property value 1
 *               Str[n] = Property value n
 *               ....
 */
//--------------------------------------------------------
CORBA::Any_var FileDatabase::DbPutProperty(CORBA::Any &send)
{
    const DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbPutProperty" << endl;

    if(!(send >>= data_in))
    {
        std::stringstream ss;
        ss << "Incorrect type passed to FileDatabase::DbPutProperty. Expecting "
           << tango_type_traits<DevVarStringArray>::type_value() << ", found " << detail::corba_any_to_type_name(send);
        TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, ss.str().c_str());
    }

    if(data_in->length() < 2)
    {
        std::stringstream ss;
        ss << "Invalid number of arguments passed to FileDatabase::DbPutProperty. Expecting at least 2, found "
           << data_in->length();
        TANGO_THROW_EXCEPTION(API_InvalidCorbaAny, ss.str().c_str());
    }

    auto &free_objects = m_server.free_objects;
    const char *obj_name = (*data_in)[0].in();

    auto obj = std::find_if(free_objects.begin(), free_objects.end(), hasName<t_free_object>(obj_name));

    if(obj == free_objects.end())
    {
        obj = free_objects.insert(free_objects.end(), new t_free_object{obj_name, {}});
    }

    auto &prop_list = (*obj)->properties;

    unsigned int n_properties = 0;
    sscanf((*data_in)[1], "%6u", &n_properties);
    unsigned int index = 2;

    for(unsigned int i = 0; i < n_properties; i++)
    {
        const char *prop_name = (*data_in)[index].in();
        index++;

        auto prop = std::find_if(prop_list.begin(), prop_list.end(), hasName<t_property>(prop_name));

        if(prop == prop_list.end())
        {
            prop = prop_list.insert(prop_list.end(), new t_property{prop_name, {}});
        }

        unsigned int n_values = 0;
        sscanf((*data_in)[index], "%6u", &n_values);
        index++;

        auto &value = (*prop)->value;
        value.resize(n_values);

        for(unsigned int j = 0; j < n_values; j++)
        {
            value[j] = (*data_in)[index];
            index++;
        }
    }

    write_file();

    return {};
}

//--------------------------------------------------------
/**
 *	Command DbDeleteProperty related method
 *	Description: Delete free property from database
 *
 *	@param argin Str[0]  = Object name
 *               Str[1] = Property name
 *               Str[n] = Property name
 */
//--------------------------------------------------------
CORBA::Any_var FileDatabase::DbDeleteProperty(CORBA::Any &send)
{
    const DevVarStringArray *data_in = nullptr;

    TANGO_LOG_DEBUG << "FILEDATABASE: entering DbDeleteProperty" << endl;

    if(!(send >>= data_in))
    {
        std::stringstream ss;
        ss << "Incorrect type passed to FileDatabase::DbDeleteProperty. Expecting "
           << tango_type_traits<DevVarStringArray>::type_value() << ", found " << detail::corba_any_to_type_name(send);
        TANGO_THROW_EXCEPTION(API_InvalidArgs, ss.str().c_str());
    }

    if(data_in->length() < 2)
    {
        std::stringstream ss;
        ss << "Invalid number of arguments passed to FileDatabase::DbDeleteProperty. Expecting at least 2, found "
           << data_in->length();
        TANGO_THROW_EXCEPTION(API_InvalidArgs, ss.str().c_str());
    }

    auto &free_objects = m_server.free_objects;
    const char *obj_name = (*data_in)[0].in();

    auto obj = std::find_if(free_objects.begin(), free_objects.end(), hasName<t_free_object>(obj_name));

    if(obj == free_objects.end())
    {
        return {};
    }

    auto &prop_list = (*obj)->properties;

    for(CORBA::ULong i = 1; i < data_in->length(); ++i)
    {
        const char *prop_name = (*data_in)[i].in();

        auto prop = std::find_if(prop_list.begin(), prop_list.end(), hasName<t_property>(prop_name));

        if(prop == prop_list.end())
        {
            continue;
        }

        delete *prop;
        prop_list.erase(prop);
    }

    write_file();

    return {};
}

CORBA::Any_var FileDatabase ::DbGetAliasDevice(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceAlias(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetAttributeAlias(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetDeviceAliasList(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase ::DbGetAttributeAliasList(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbGetClassPipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbGetDevicePipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbDeleteClassPipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbDeleteDevicePipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbPutClassPipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

CORBA::Any_var FileDatabase::DbPutDevicePipeProperty(CORBA::Any &)
{
    TANGO_THROW_EXCEPTION(API_NotSupported, "Call to a Filedatabase not implemented.");
}

//-----------------------------------------------------------------------------
//
// method :            FileDatabase::write_event_channel_ior() -
//
// description :     Method to write the event channel ior into the file
//
// argument : in : ior_string : The event channel IOR
//
//-----------------------------------------------------------------------------

void FileDatabase::write_event_channel_ior(const string &ior_string)
{
    //
    // Do we already have this info in file?
    //

    unsigned int i;

    for(i = 0; i < m_server.classes.size(); i++)
    {
        if(equalsIgnoreCase(NOTIFD_CHANNEL, m_server.classes[i]->name))
        {
            //
            // Yes, we have it, simply replace the old IOR by the new one (as device name!)
            //

            m_server.classes[i]->devices[0]->name = ior_string;
            break;
        }
    }

    if(i == m_server.classes.size())
    {
        //
        // Add the pseudo notifd channel class
        //

        t_device *ps_dev = new t_device;
        ps_dev->name = ior_string;
        t_tango_class *tg_cl = new t_tango_class;
        tg_cl->devices.push_back(ps_dev);
        tg_cl->name = NOTIFD_CHANNEL;

        m_server.classes.push_back(tg_cl);
    }
}

} // end of namespace Tango
