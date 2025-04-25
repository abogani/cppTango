#include "utils/platform/platform.h"

#include "utils/platform/ready_string_finder.h"

#define UNICODE
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <windows.h>
#include <shlwapi.h>
#include <signal.h>

#include <stdexcept>
#include <memory>
#include <sstream>
#include <cstring>
#include <filesystem>
#include <algorithm>

namespace TangoTest::platform
{

namespace
{

std::wstring to_wstring(std::string_view str)
{
    int len = MultiByteToWideChar(CP_UTF8,    // CodePage
                                  0,          // dwFlags,
                                  str.data(), // lpMultiByteStr
                                  str.size(), // cbMultiByte
                                  nullptr,    // lpWideCharStr
                                  0           // cchWideChar
    );
    std::wstring result(static_cast<size_t>(len), L'\0');

    MultiByteToWideChar(CP_UTF8,       // CodePage
                        0,             // dwFlags,
                        str.data(),    // lpMultiByteStr
                        str.size(),    // cbMultiByte
                        result.data(), // lpWideCharStr
                        len            // cchWideChar
    );

    return result;
}

std::string to_string(std::wstring_view wstr)
{
    size_t len = WideCharToMultiByte(CP_UTF8,     // CodePage
                                     0,           // dwFlags
                                     wstr.data(), // lpWideCharStr
                                     wstr.size(), // cchWideChar
                                     nullptr,     // lpMultiByteStr
                                     0,           // cbMultiByte
                                     nullptr,     // lpDefaultChar
                                     nullptr      // lpUsedDefaultChar
    );
    std::string result(static_cast<size_t>(len), '\0');

    WideCharToMultiByte(CP_UTF8,       // CodePage
                        0,             // dwFlags
                        wstr.data(),   // lpWideCharStr
                        wstr.size(),   // cchWideChar
                        result.data(), // lpMultiByteStr
                        len,           // cbMultiByte
                        nullptr,       // lpDefaultChar
                        nullptr        // lpUsedDefaultChar
    );

    return result;
}

void append_last_error(std::ostream &os)
{
    DWORD last_error = GetLastError();
    LPTSTR error_text = nullptr;
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, // dwFlags
        nullptr,                                                                                     // lpSource
        last_error,                                                                                  // dwMessageId
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),                                                   // dwLanguageId
        reinterpret_cast<LPTSTR>(&error_text),                                                       // lpBuffer
        0,                                                                                           // nSize
        nullptr                                                                                      // Arguments
    );

    if(error_text != nullptr)
    {
        os << ": " << to_string(error_text);
        LocalFree(error_text);
    }
    else
    {
        os << ": Unknown error";
    }
}

template <typename... Args>
[[noreturn]] void throw_last_error(Args... args)
{
    std::stringstream ss;
    (ss << ... << args);
    append_last_error(ss);
    throw std::runtime_error(ss.str());
}

ExitStatus convert_exit_code(DWORD exit_code)
{
    // It would be nice to treat unhandled Windows exceptions
    // as being Aborted here, but it doesn't seem there is a
    // reliable way to know if the exit_code from the process
    // is the value passed to ExitProcess or the Exception Code.
    // TODO: Workout a recipe to know when we can say Aborted here.
    ExitStatus result;
    result.kind = ExitStatus::Kind::Normal;
    result.code = static_cast<int>(exit_code);

    return result;
}

// All TestServer.exe's we start get assigned to this JobObject so that they
// can be killed whenever the test exits.
HANDLE s_job_object = INVALID_HANDLE_VALUE;

} // namespace

std::vector<std::string> default_env()
{
    return {
        std::string("SystemRoot=") + getenv("SystemRoot"), // Required for WinSock
        "ORBscanGranularity=1"                             // Makes shutdown faster, this is how frequently (in seconds)
                                                           // the ORB checks for client connections to be dead.
    };
}

void init()
{
    s_job_object = CreateJobObject(nullptr, nullptr);

    if(s_job_object == INVALID_HANDLE_VALUE)
    {
        throw_last_error("CreateJobObject");
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info = {0};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    BOOL success = SetInformationJobObject(s_job_object,                      // hJob,
                                           JobObjectExtendedLimitInformation, // JobObjectInformationClass
                                           &info,                             // lpJobObjectInformation
                                           sizeof(info)                       // cbJobObjectInformationLength
    );

    if(success == 0)
    {
        throw_last_error("SetInformationJobObject");
    }
}

StartServerResult start_server(const std::vector<std::string> &args,
                               const std::vector<std::string> &env,
                               const std::string &redirect_filename,
                               const std::string &ready_string,
                               std::chrono::milliseconds timeout)
{
    StartServerResult result;

    // Returns "<item0><sep><item1><sep>...<itemn>\0\0"
    auto make_concat_wide_buffer = [](wchar_t sep, const char *name, const std::vector<std::string> &items)
    {
        // Windows limit on the length of the command line/environment table. See
        // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa
        constexpr const size_t k_maximum_buffer_size = 32767;

        std::vector<std::wstring> wide_items;
        wide_items.reserve(items.size());

        for(const auto &item : items)
        {
            wide_items.push_back(to_wstring(item));
        }

        size_t buffer_size = 0;
        for(const auto &item : wide_items)
        {
            // + 1 for the separator or trailing '\0'
            buffer_size += item.size() + 1;
        }
        // For the additional trailing '\0' required by the environment table
        buffer_size += 1;

        if(buffer_size == 0)
        {
            return std::unique_ptr<wchar_t[]>{nullptr};
        }

        if(buffer_size > k_maximum_buffer_size)
        {
            std::stringstream ss;
            ss << name << " too long.  Total buffer size is " << buffer_size << " but must be less than "
               << k_maximum_buffer_size;
            throw std::runtime_error(ss.str());
        }

        auto buffer = std::make_unique<wchar_t[]>(buffer_size);
        std::memset(buffer.get(), 0, buffer_size);

        size_t i = 0;
        for(const auto &item : wide_items)
        {
            if(i != 0)
            {
                buffer[i] = sep;
                i += 1;
            }
            std::memcpy(&buffer[i], item.c_str(), sizeof(wchar_t) * item.size());
            i += item.size();
        }

        return buffer;
    };

    // The command_line and environment_table buffers must be mutable, so we
    // use a unique_ptr<wchar_t[]> rather than wstring here.
    std::unique_ptr<wchar_t[]> command_line = make_concat_wide_buffer(L' ', "Arguments", args);
    std::unique_ptr<wchar_t[]> environment_table = make_concat_wide_buffer(L'\0', "Environment table", env);

    auto redirect_file = [&redirect_filename]()
    {
        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;

        HANDLE handle = CreateFile(to_wstring(redirect_filename).c_str(),                  // lpFileName,
                                   GENERIC_WRITE,                                          // dwDesiredAccess,
                                   FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                                   &sa,                                                    // lpSecurityAttributes,
                                   CREATE_ALWAYS,                                          // dwCreationDisposition
                                   FILE_ATTRIBUTE_NORMAL,                                  // dwFlagsAndAttributes
                                   nullptr                                                 // hTemplateFile
        );

        if(handle == INVALID_HANDLE_VALUE)
        {
            throw_last_error("CreateFile");
        }

        return std::unique_ptr<void, decltype(&CloseHandle)>{handle, CloseHandle};
    }();

    ReadyStringFinder finder{redirect_filename};

    STARTUPINFO startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION process_info;
    ZeroMemory(&process_info, sizeof(process_info));

    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdOutput = redirect_file.get();
    startup_info.hStdError = redirect_file.get();

    // We create a new process group here so that we can later
    // GenerateConsoleCtrlEvent (which results in a SIGBREAK) to request that the
    // this specific server stops.  The process group id will match the process
    // id of this process and the process will be connected to a new console.

    BOOL success = CreateProcess(to_wstring(k_test_server_binary_path).c_str(),         // lpApplicationName
                                 command_line.get(),                                    // lpCommandLine
                                 nullptr,                                               // lpProcessAttributes
                                 nullptr,                                               // lpThreadAttributes
                                 TRUE,                                                  // bInheritHandles
                                 CREATE_NEW_PROCESS_GROUP | CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
                                 environment_table.get(),                               // lpEnvironment
                                 nullptr,                                               // lpCurrentDirectory
                                 &startup_info,                                         // lpStartupInfo
                                 &process_info                                          // lpProcessInformation
    );

    redirect_file.reset(nullptr);

    if(success == 0)
    {
        throw_last_error("CreateProcess");
    }

    CloseHandle(process_info.hThread);
    std::unique_ptr<void, decltype(&CloseHandle)> process = {process_info.hProcess, CloseHandle};

    success = AssignProcessToJobObject(s_job_object, process.get());

    if(success == 0)
    {
        throw_last_error("AssignProcessToJobObject");
    }

    auto end = std::chrono::steady_clock::now() + timeout;
    while(true)
    {
        constexpr const DWORD k_poll_period_ms = 10;

        using Kind = StartServerResult::Kind;

        if(finder.check_for_ready_string(ready_string))
        {
            result.kind = Kind::Started;
            result.handle = reinterpret_cast<TestServer::Handle *>(process.release());
            return result;
        }

        int remaining_timeout =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::steady_clock::now()).count();
        if(remaining_timeout < 0)
        {
            result.kind = Kind::Timeout;
            result.handle = reinterpret_cast<TestServer::Handle *>(process.release());
            return result;
        }

        DWORD loop_timeout = std::min(k_poll_period_ms, static_cast<DWORD>(remaining_timeout));
        DWORD retcode = WaitForSingleObject(process.get(), loop_timeout);

        if(retcode == WAIT_FAILED)
        {
            throw_last_error("WaitForSingleObject");
        }

        if(retcode == WAIT_ABANDONED)
        {
            // This should not be possible as process does not point to a mutex object
            throw std::runtime_error("WaitForSingleObject: Unexpected WAIT_ABANDONED");
        }

        if(retcode == WAIT_OBJECT_0)
        {
            DWORD exit_code;
            if(GetExitCodeProcess(process.get(), &exit_code) == 0)
            {
                throw_last_error("GetExitCodeProcess");
            }

            if(STILL_ACTIVE == exit_code)
            {
                continue; // spurious wake up
            }

            result.kind = Kind::Exited;
            result.exit_status = convert_exit_code(exit_code);
            return result;
        }
    }
}

std::vector<int> relevant_sendable_signals()
{
    return {SIGBREAK};
}

void send_signal(TestServer::Handle *handle, int signo)
{
    HANDLE child = static_cast<HANDLE>(handle);
    DWORD pid = GetProcessId(child);

    BOOL success;
    if(signo == SIGBREAK)
    {
        success = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);
    }
    else
    {
        throw_last_error("signal not supported in windows");
    }

    if(success == 0)
    {
        throw_last_error("GenerateConsoleCtrlEvent");
    }
}

StopServerResult stop_server(TestServer::Handle *handle)
{
    using Kind = StopServerResult::Kind;

    std::unique_ptr<void, decltype(&CloseHandle)> process = {static_cast<HANDLE>(handle), &CloseHandle};

    StopServerResult result;

    DWORD exit_code;
    if(GetExitCodeProcess(process.get(), &exit_code) == 0)
    {
        throw_last_error("GetExitCodeProcess");
    }

    if(STILL_ACTIVE != exit_code)
    {
        result.kind = Kind::ExitedEarly;
        result.exit_status = convert_exit_code(exit_code);
        return result;
    }

    DWORD pid = GetProcessId(process.get());
    if(pid == 0)
    {
        throw_last_error("GetProcessId");
    }

    BOOL success = GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);

    if(success == 0)
    {
        throw_last_error("GenerateConsoleCtrlEvent");
    }

    // We don't actually need to use the return value here as this was passed to us by the caller
    // and we are just using the unique_ptr in case we throw earlier.
    (void) process.release();
    result.kind = Kind::Exiting;
    return result;
}

WaitForStopResult wait_for_stop(TestServer::Handle *handle, std::chrono::milliseconds timeout)
{
    using Kind = WaitForStopResult::Kind;

    HANDLE process = static_cast<HANDLE>(handle);

    WaitForStopResult result;

    auto end = std::chrono::steady_clock::now() + timeout;
    while(true)
    {
        DWORD remaining_timeout =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - std::chrono::steady_clock::now()).count();
        DWORD retcode = WaitForSingleObject(process, timeout.count());

        if(retcode == WAIT_FAILED)
        {
            throw_last_error("WaitForSingleObject");
        }

        if(retcode == WAIT_ABANDONED)
        {
            // This should not be possible as process does not point to a mutex object
            throw std::runtime_error("WaitForSingleObject: Unexpected WAIT_ABANDONED");
        }

        if(retcode == WAIT_OBJECT_0)
        {
            DWORD exit_code;
            if(GetExitCodeProcess(process, &exit_code) == 0)
            {
                throw_last_error("GetExitCodeProcess");
            }

            if(STILL_ACTIVE == exit_code)
            {
                continue; // spurious wake up
            }

            result.kind = Kind::Exited;
            result.exit_status = convert_exit_code(exit_code);
            return result;
        }

        if(retcode == WAIT_TIMEOUT)
        {
            // We need to terminate the process here so that it releases the
            // redirect file handle.
            if(TerminateProcess(process, 0) == 0)
            {
                throw_last_error("TerminateProcess after timeout on stop");
            }

            // Block until the process has actually stopped.
            WaitForSingleObject(process, INFINITE);
            result.kind = Kind::Timeout;
            return result;
        }
    }
}
} // namespace TangoTest::platform
