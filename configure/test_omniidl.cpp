#include "test_omniidl.h"
#include <iostream>

int main()
{
    // omniidl 4.2.1 has a bug in the c++ generation of IDL unions.
    // The union's copy constructor will not check the _pd__initialised flag
    // before accessing the other union to do the copy.
    //
    // Here we check for this by constructing a zeroed union and trying to copy
    // it.  The first member (with value 0) of the union is an IDL sequence so the
    // active member of the zeroed struct is a null pointer.  Therefore, this copy
    // will result in a segmentation fault.

    TestOmniidl::AttrValUnion zeroed;
    memset(static_cast<void *>(&zeroed), 0, sizeof(TestOmniidl::AttrValUnion));
    TestOmniidl::AttrValUnion copy{zeroed};

    // Print something so that the above is not optimized away.
    std::cout << "Active union memebr: " << static_cast<int>(copy._d()) << "\n";
}
