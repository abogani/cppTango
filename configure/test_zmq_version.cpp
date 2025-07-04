#include <zmq.h>

// Check libzmq version using plain C++03

#define MINIMUM_VERSION ZMQ_MAKE_VERSION(MINIMUM_VERSION_MAJOR, MINIMUM_VERSION_MINOR, MINIMUM_VERSION_PATCH)
#define FOUND_VERSION ZMQ_MAKE_VERSION(ZMQ_VERSION_MAJOR, ZMQ_VERSION_MINOR, ZMQ_VERSION_PATCH)

#if FOUND_VERSION < MINIMUM_VERSION
  #error "Old version"
#endif

int main(int, char **)
{
    return 0;
}
