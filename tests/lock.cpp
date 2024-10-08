// NOLINTBEGIN(*)

#ifdef WIN32
  #include <process.h>
  #define WEXITSTATUS(w) w
#else
  #include <sys/wait.h>
#endif

#include <tango/internal/net.h>
#include "old_common.h"

#include "locked_device_cmd.h"

int main(int argc, char **argv)
{
    DeviceProxy *device;

    if((argc < 3) || (argc > 4))
    {
        TEST_LOG << "usage: lock <device1> <device2>" << endl;
        exit(-1);
    }

    string device_name = argv[1];
    string device2_name = argv[2];

    try
    {
        device = new DeviceProxy(device_name);
    }
    catch(CORBA::Exception &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    TEST_LOG << endl << "new DeviceProxy(" << device->name() << ") returned" << endl << endl;

    try
    {
        // Impossible to lock admin device

        string adm_name = device->adm_name();
        DeviceProxy *admin = new DeviceProxy(adm_name);
        bool except = false;
        try
        {
            admin->lock();
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), API_DeviceUnlockable) == 0)
            {
                except = true;
            }
        }

        assert(except == true);
        TEST_LOG << "  Admin device unlockable --> OK" << endl;

        // Unlocking a non locked device does nothing

        device->unlock();
        TEST_LOG << "  Unlock a non-locked device --> OK" << endl;

        // Lock validity checks

        except = false;
        try
        {
            device->lock(-1);
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), API_MethodArgument) == 0)
            {
                except = true;
            }
        }

        assert(except == true);

        except = false;
        try
        {
            device->lock(1);
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), API_MethodArgument) == 0)
            {
                except = true;
            }
        }

        assert(except == true);

        TEST_LOG << "  Basic test on lock validity argument --> OK" << endl;

        // Lock the device, and do some basic test

        bool bool_ret;
        LockerInfo the_locker;

        bool_ret = device->is_locked();
        TEST_LOG << "Passed 10" << endl;
        assert(bool_ret == false);

        bool_ret = device->is_locked_by_me();
        TEST_LOG << "Passed 11" << endl;
        assert(bool_ret == false);

        bool_ret = device->get_locker(the_locker);
        TEST_LOG << "Passed 12" << endl;
        assert(bool_ret == false);

        device->lock();

        bool_ret = device->is_locked();
        TEST_LOG << "Passed 13" << endl;
        assert(bool_ret == true);

        bool_ret = device->is_locked_by_me();
        TEST_LOG << "Passed 14" << endl;
        assert(bool_ret == true);

        bool_ret = device->get_locker(the_locker);
        TEST_LOG << "Passed 15" << endl;
        assert(bool_ret == true);
        assert(the_locker.ll == Tango::CPP);
#ifdef WIN32
        assert(the_locker.li.LockerPid == _getpid());
#else
        assert(the_locker.li.LockerPid == getpid());
#endif
        char h_name[Tango::detail::TANGO_MAX_HOSTNAME_LEN];
        gethostname(h_name, Tango::detail::TANGO_MAX_HOSTNAME_LEN);
        string my_host(h_name);

        string::size_type pos;
        string only_host;
        if((pos = the_locker.locker_host.find('.')) != string::npos)
        {
            only_host = the_locker.locker_host.substr(0, pos);
        }
        else
        {
            only_host = the_locker.locker_host;
        }
        assert(only_host == my_host);

        except = false;
        try
        {
            device->lock(2);
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), API_MethodArgument) == 0)
            {
                except = true;
            }
        }

        assert(except == true);

        DeviceData din, dout;
        din << (short) 2;
        dout = device->command_inout("IOShort", din);
        short result;
        dout >> result;

        assert(result == 4);

        string sub_process_cmd(LOCKED_DEVICE_CMD);
        sub_process_cmd = sub_process_cmd + device_name;
        int ret = system(sub_process_cmd.c_str());

        TEST_LOG << "Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        device->unlock();
        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "Locked_device returned value after unlock = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        bool_ret = device->is_locked();
        assert(bool_ret == false);

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == false);

        bool_ret = device->get_locker(the_locker);
        assert(bool_ret == false);

        TEST_LOG << "  Basic Lock/Unlock --> OK" << endl;

        // Check the allowed commands

        device->lock();
        string sub_proc(ALLOWED_CMD);
        sub_proc = sub_proc + device_name;
        ret = system(sub_proc.c_str());

        TEST_LOG << "allowed_cmd returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        TEST_LOG << "  Allowed command while device is locked --> OK" << endl;
        device->unlock();

        // Re-entrant lock

        device->lock();
        device->lock();

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Re-entrant lock) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        device->unlock();

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Re-entrant lock) Locked_device returned value after first unlock = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        device->unlock();

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Re-entrant lock) Locked_device returned value after second unlock = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        TEST_LOG << "  Re-Entrant Lock/Unlock --> OK" << endl;

        // Destroying the device unlock it

        device->lock();
        device->lock();

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Destroying device) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        delete device;
        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Destroying device) Locked_device returned value after the delete = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        TEST_LOG << "  Destroy the DeviceProxy unlocks the device --> OK" << endl;
        device = new DeviceProxy(device_name);

        // Restarting a device keep the lock

        device->lock(2);

        din << device_name;
        admin->command_inout("DevRestart", din);

        device->state();
        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Restarting device) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        TEST_LOG << "  Restart a locked device keeps the lock --> OK" << endl;

        // Impossible to restart a locked device by another client

        string restart_cmd(RESTART_DEVICE_CMD);
        restart_cmd = restart_cmd + device_name + " DevRestart";

        ret = system(restart_cmd.c_str());

        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        TEST_LOG << "  Impossible to restart a device locked by another client --> OK" << endl;

        // Impossible to do polling related commands on a locked device

        string base_restart("restart_device ");
        base_restart = base_restart + device_name;

        string res = base_restart + " AddObjPolling";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        res = base_restart + " RemObjPolling";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        res = base_restart + " UpdObjPollingPeriod";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        TEST_LOG << "  Impossible to change polling on a device locked by another client --> OK" << endl;

        // Impossible to do logging related commands on a locked device

        res = base_restart + " AddLoggingTarget";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        res = base_restart + " RemoveLoggingTarget";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        res = base_restart + " SetLoggingLevel";

        ret = system(restart_cmd.c_str());
        TEST_LOG << "restart_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        TEST_LOG << "  Impossible to change logging on a device locked by another client --> OK" << endl;

        // Check that the locking thread is doing periodic re-lock

        DeviceProxy *device2 = new DeviceProxy(device2_name);
        device2->lock(6);

        std::this_thread::sleep_for(std::chrono::seconds(7));
        bool_ret = device->is_locked_by_me();
        assert(bool_ret == true);
        device->unlock();
        delete device2;

        TEST_LOG << "  Locking thread re-locks the device --> OK" << endl;

        // Unlock a device using the back door

        string unlock_cmd(UNLOCK_CMD);
        unlock_cmd = unlock_cmd + device_name;

        device->lock();
        device->lock();
        device->lock();

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == true);

        ret = system(unlock_cmd.c_str());

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == false);

        bool_ret = device->is_locked();
        assert(bool_ret == false);

        din << (short) 2;
        except = false;
        try
        {
            dout = device->command_inout("IOShort", din);
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[0].reason.in(), API_DeviceUnlocked) == 0)
            {
                except = true;
            }
        }

        assert(except == true);

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Back door) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        device->unlock();
        device->unlock();
        device->unlock();

        TEST_LOG << "  Another client unlocks the device using the back door --> OK" << endl;

        // Restarting a server breaks the lock

        device->lock();

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == true);

        admin->command_inout("RestartServer");

        std::this_thread::sleep_for(std::chrono::seconds(2));
        bool_ret = device->is_locked_by_me();
        assert(bool_ret == false);

        bool_ret = device->is_locked();
        assert(bool_ret == false);

        except = false;
        try
        {
            dout = device->command_inout("IOShort", din);
        }
        catch(Tango::DevFailed &e)
        {
            if(::strcmp(e.errors[1].reason.in(), API_DeviceUnlocked) == 0)
            {
                except = true;
            }
        }
        assert(except == true);

        except = false;
        try
        {
            dout = device->command_inout("IOShort", din);
        }
        catch(...)
        {
            except = true;
        }
        assert(except == false);

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Restart server) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        device->unlock();

        TEST_LOG << "  Restart the server breaks the lock --> OK" << endl;

        // Locking without relock kills the lock

        device->lock(2);

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == true);

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Without ReLock) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 1);

        ApiUtil *au = ApiUtil::instance();
        au->clean_locking_threads(false);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        bool_ret = device->is_locked_by_me();
        assert(bool_ret == false);

        ret = system(sub_process_cmd.c_str());

        TEST_LOG << "(Without ReLock) Locked_device returned value = " << WEXITSTATUS(ret) << endl;
        assert(WEXITSTATUS(ret) == 0);

        TEST_LOG << "  Lock validity --> OK" << endl;

        delete admin;
    }
    catch(Tango::DevFailed &e)
    {
        Except::print_exception(e);
        exit(1);
    }

    delete device;
    return 0;
}

// NOLINTEND(*)
