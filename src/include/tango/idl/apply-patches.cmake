function(replace_in_file FILENAME SEARCH NEW_CONTENT)
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" ORIGINAL_DATA)
    string(REPLACE "${SEARCH}" "${NEW_CONTENT}" REPLACED_DATA "${ORIGINAL_DATA}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" "${REPLACED_DATA}")
endfunction()

function(add_after_in_file FILENAME SEARCH CONTENT)
    replace_in_file(${FILENAME} "${SEARCH}" "${SEARCH} ${CONTENT}")
endfunction()

function(nolint_file FILENAME)
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" ORIGINAL_DATA)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" "// Added by Tango team\n// NOLINTBEGIN\n\n")
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" "${ORIGINAL_DATA}")
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}" "// Added by Tango team\n// NOLINTEND\n")
endfunction()

nolint_file(tango.h)
nolint_file(tangoSK.cpp)
nolint_file(tangoDynSK.cpp)

add_after_in_file(tango.h "#define __tango_hh__" "\n\n\
//Added by Tango team\n\
#ifdef __clang_analyzer__\n\
#pragma clang system_header\n\
#endif\n\
#if defined(__GNUC__) && !defined(__clang__)\n\
#pragma GCC system_header\n\
#endif\n")

add_after_in_file(tango.h "typedef _CORBA_ConstrType_Variable_Var<AttributeValue_4> _var_type;" "\n\n\
    //Added by Tango team\n\
    virtual ~AttributeValue_4() {if (mut_ptr != nullptr)mut_ptr->unlock();}\n\
    AttributeValue_4() {mut_ptr=nullptr;}\n\
    void set_attr_mutex(omni_mutex *ptr) {mut_ptr=ptr;}\n\
    void rel_attr_mutex() {if (mut_ptr != nullptr){mut_ptr->unlock();mut_ptr=nullptr;}}\n\
    omni_mutex *get_attr_mutex() {return mut_ptr;}\n\
    omni_mutex *mut_ptr;\n")

add_after_in_file(tango.h "typedef _CORBA_ConstrType_Variable_Var<AttributeValue_5> _var_type;" "\n\n\
    //Added by Tango team\n\
    virtual ~AttributeValue_5() {if (mut_ptr != nullptr)mut_ptr->unlock();}\n\
    AttributeValue_5() {mut_ptr=nullptr;}\n\
    void set_attr_mutex(omni_mutex *ptr) {mut_ptr=ptr;}\n\
    void rel_attr_mutex() {if (mut_ptr != nullptr){mut_ptr->unlock();mut_ptr=nullptr;}}\n\
    omni_mutex *get_attr_mutex() {return mut_ptr;}\n\
    omni_mutex *mut_ptr;\n")

add_after_in_file(tango.h "typedef\ _CORBA_ConstrType_Variable_Var<DevPipeData> _var_type;" "\n\n\
    //Added by Tango team\n\
    virtual ~DevPipeData() {if (mut_ptr != nullptr)mut_ptr->unlock();}\n\
    DevPipeData() {mut_ptr=nullptr;}\n\
    void set_pipe_mutex(omni_mutex *ptr) {mut_ptr=ptr;}\n\
    void rel_pipe_mutex() {if (mut_ptr != nullptr){mut_ptr->unlock();mut_ptr=nullptr;}}\n\
    omni_mutex *mut_ptr;\n")
