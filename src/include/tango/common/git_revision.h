namespace Tango
{

/// @brief The git revision of the source tree used to build this library.
///
/// A small, helper function returning the output of the "git describe" command
/// at the moment of building this library. If there was any error when trying
/// to obtain this information then it returns "unknown".
const char *git_revision();

} // namespace Tango
