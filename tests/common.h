#ifndef Common_H
#define Common_H

#include <iostream>
#include <tango/tango.h>

#ifndef TS_ASSERT
#include <assert.h>
#endif

#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace std;
using namespace Tango;

#include "logging.h"

#endif // Common_H
