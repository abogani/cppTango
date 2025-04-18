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

#ifndef FILEDATABASE_H
#define FILEDATABASE_H

#include <tango/idl/tango.h>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

namespace Tango
{

#define _TG_NUMBER 1
#define _TG_STRING 2
#define _TG_COMA 3
#define _TG_COLON 4
#define _TG_SLASH 5
#define _TG_ASLASH 6
#define _TG_ARROW 7

class t_property
{
  public:
    std::string name;
    std::vector<std::string> value;
};

class t_attribute_property
{
  public:
    std::string attribute_name;
    std::vector<t_property *> properties;
};

class t_device
{
  public:
    std::string name;
    std::vector<t_property *> properties;
    std::vector<t_attribute_property *> attribute_properties;
};

class t_tango_class
{
  public:
    std::string name;
    std::string description;
    std::string title;
    std::vector<t_device *> devices;
    std::vector<t_property *> properties;
    std::vector<t_attribute_property *> attribute_properties;
};

class t_free_object
{
  public:
    std::string name;
    std::vector<t_property *> properties;
};

class t_server
{
  public:
    std::string name;
    std::string instance_name;
    std::vector<t_tango_class *> classes;
    std::vector<t_device *> devices;
    std::vector<t_free_object *> free_objects;
};

template <class T>
class hasName
{
    std::string name;

  public:
    hasName(std::string _name) :
        name(_name)
    {
    }

    bool operator()(T *obj);
};

template <class T>
class hasAttributeName
{
    std::string attribute_name;

  public:
    hasAttributeName(std::string _name) :
        attribute_name(_name)
    {
    }

    bool operator()(T *obj);
};

class FileDatabaseExt
{
  public:
    FileDatabaseExt();

    ~FileDatabaseExt() = default;
};

class FileDatabase
{
  public:
    FileDatabase(const std::string &file_name);
    ~FileDatabase();
    std::string parse_res_file(const std::string &file_name);
    void display();
    std::string get_display();
    void write_event_channel_ior(const std::string &);

    CORBA::Any_var DbInfo(CORBA::Any &);
    CORBA::Any_var DbImportDevice(CORBA::Any &);
    CORBA::Any_var DbExportDevice(CORBA::Any &);
    CORBA::Any_var DbUnExportDevice(CORBA::Any &);
    CORBA::Any_var DbAddDevice(CORBA::Any &);
    CORBA::Any_var DbDeleteDevice(CORBA::Any &);
    CORBA::Any_var DbAddServer(CORBA::Any &);
    CORBA::Any_var DbDeleteServer(CORBA::Any &);
    CORBA::Any_var DbExportServer(CORBA::Any &);
    CORBA::Any_var DbUnExportServer(CORBA::Any &);
    CORBA::Any_var DbGetServerInfo(CORBA::Any &);

    CORBA::Any_var DbGetDeviceProperty(CORBA::Any &);
    CORBA::Any_var DbPutDeviceProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteDeviceProperty(CORBA::Any &);
    CORBA::Any_var DbGetDeviceAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbPutDeviceAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteDeviceAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbGetClassProperty(CORBA::Any &);
    CORBA::Any_var DbPutClassProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteClassProperty(CORBA::Any &);
    CORBA::Any_var DbGetClassAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbPutClassAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteClassAttributeProperty(CORBA::Any &);
    CORBA::Any_var DbGetDeviceList(CORBA::Any &);
    CORBA::Any_var DbGetDeviceWideList(CORBA::Any &);

    CORBA::Any_var DbGetDeviceDomainList(CORBA::Any &);
    CORBA::Any_var DbGetDeviceMemberList(CORBA::Any &);
    CORBA::Any_var DbGetDeviceExportedList(CORBA::Any &);
    CORBA::Any_var DbGetDeviceFamilyList(CORBA::Any &);
    CORBA::Any_var DbGetProperty(CORBA::Any &);
    CORBA::Any_var DbPutProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteProperty(CORBA::Any &);
    CORBA::Any_var DbGetAliasDevice(CORBA::Any &);
    CORBA::Any_var DbGetDeviceAlias(CORBA::Any &);
    CORBA::Any_var DbGetAttributeAlias(CORBA::Any &);
    CORBA::Any_var DbGetDeviceAliasList(CORBA::Any &);
    CORBA::Any_var DbGetAttributeAliasList(CORBA::Any &);

    CORBA::Any_var DbGetClassPipeProperty(CORBA::Any &);
    CORBA::Any_var DbGetDevicePipeProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteClassPipeProperty(CORBA::Any &);
    CORBA::Any_var DbDeleteDevicePipeProperty(CORBA::Any &);
    CORBA::Any_var DbPutClassPipeProperty(CORBA::Any &);
    CORBA::Any_var DbPutDevicePipeProperty(CORBA::Any &);

    void write_file();

  private:
    std::string filename;
    t_server m_server;

    void read_char(std::ifstream &f);
    int class_lex(const std::string &word);
    void jump_line(std::ifstream &f);
    void jump_space(std::ifstream &f);
    std::string read_word(std::ifstream &f);
    void CHECK_LEX(int lt, int le);
    std::vector<std::string> parse_resource_value(std::ifstream &f);

    std::string read_full_word(std::ifstream &f);

    static const char *lexical_word_null;
    static const char *lexical_word_number;
    static const char *lexical_word_string;
    static const char *lexical_word_coma;
    static const char *lexical_word_colon;
    static const char *lexical_word_slash;
    static const char *lexical_word_backslash;
    static const char *lexical_word_arrow;
    static int ReadBufferSize;
    static int MaxWordLength;

    int CrtLine;
    int StartLine;
    char CurrentChar;
    char NextChar;

    std::string word;

    std::unique_ptr<FileDatabaseExt> ext;
};

} // end namespace Tango

#endif
