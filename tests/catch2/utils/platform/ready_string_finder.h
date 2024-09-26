#ifndef TANGO_TESTS_CATCH2_UTILS_PLATFORM
#define TANGO_TESTS_CATCH2_UTILS_PLATFORM

#include <string>
#include <fstream>

namespace TangoTest::platform
{
class ReadyStringFinder
{
  public:
    /** Searches through the given file for a "ready string" .
     *
     * If the ready string isn't found, the ReadyStringFinder will remember
     * where it is in the file, so that you can try again later once you
     * know there is more data in the file.
     *
     * Example:
     *
     *  ReadyStringFinder finder(my_file);
     *  bool found = false;
     *  do {
     *    if (finder.check_for_ready_string("my ready string")) {
     *      found = true;
     *      break;
     *    }
     *    update_timeout(timeout);
     *  } while(wait_for_new_data(timeout))
     *
     *
     * @param filename -- Path to the file to search
     */
    explicit ReadyStringFinder(const std::string &filename);

    /** Returns true if a line containing the ready_string is found.
     *
     * @param ready_string -- line to search for.
     */
    bool check_for_ready_string(const std::string &ready_string);

  private:
    std::ifstream m_file;
};

} // namespace TangoTest::platform

#endif
