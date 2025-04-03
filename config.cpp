#include "config.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

void printConfigHelp()
{
    std::cout << R"(DESCRIPTION:
        Modifies a service entry in the registry and Service Database.
USAGE:
        sc <server> config [service name] <option1> <option2>...

OPTIONS:
NOTE: The option name includes the equal sign.
      A space is required between the equal sign and the value.
      To remove the dependency, use a single / as dependency value.
 type= <own|share|interact|kernel|filesys|rec|adapt|userown|usershare>
 start= <boot|system|auto|demand|disabled|delayed-auto>
 error= <normal|severe|critical|ignore>
 binPath= <BinaryPathName to the .exe file>
 group= <LoadOrderGroup>
 tag= <yes|no>
 depend= <Dependencies(separated by / (forward slash))>
 obj= <AccountName|ObjectName>
 DisplayName= <display name>
 password= <password>
)";
}

// --- ParseConfigOptions ---
// The first token is the service name.
// Then tokens must appear as key= followed by a value.
void ParseConfigOptions(const std::vector<std::string> &args, ConfigOptions &opts)
{
    if (args.empty())
    {
        throw std::invalid_argument("Error: config requires a service name.");
    }
    // The first token is the service name.
    opts.serviceName = args[0];

    // For the "type" option, we allow it to appear twice when the first occurrence is "interact".
    bool firstTypeProvided = false;
    bool secondTypeProvided = false;

    size_t i = 1;
    while (i < args.size())
    {
        std::string token = args[i];
        if (token.size() < 2 || token.back() != '=')
        {
            throw std::invalid_argument("Error: Invalid option format '" + token + "'. Expected key= followed by a value.");
        }
        std::string key = token.substr(0, token.size() - 1);
        i++;
        if (i >= args.size())
        {
            throw std::invalid_argument("Error: Missing value for option '" + key + "='.");
        }
        std::string value = args[i];
        i++;

        if (key == "type")
        {
            if (!firstTypeProvided)
            {
                if (value != "own" && value != "share" && value != "kernel" &&
                    value != "filesys" && value != "rec" && value != "adapt" && value != "interact")
                {
                    throw std::invalid_argument("Error: Invalid value for type. Allowed: own, share, kernel, filesys, rec, adapt, interact.");
                }
                opts.serviceType = value;
                firstTypeProvided = true;
            }
            else
            {
                // Second occurrence is allowed only if the first was "interact".
                if (opts.serviceType != "interact")
                {
                    throw std::invalid_argument("Error: Unexpected second type parameter when first type is not 'interact'.");
                }
                if (value != "own" && value != "share")
                {
                    throw std::invalid_argument("Error: Invalid value for second type. Allowed: own, share.");
                }
                opts.interactType = value;
                secondTypeProvided = true;
            }
        }
        else if (key == "start")
        {
            if (value != "boot" && value != "system" && value != "auto" &&
                value != "demand" && value != "disabled" && value != "delayed-auto")
            {
                throw std::invalid_argument("Error: Invalid start type. Allowed: boot, system, auto, demand, disabled, delayed-auto.");
            }
            opts.startType = value;
        }
        else if (key == "error")
        {
            if (value != "normal" && value != "severe" && value != "critical" && value != "ignore")
            {
                throw std::invalid_argument("Error: Invalid error control value. Allowed: normal, severe, critical, ignore.");
            }
            opts.errorControl = value;
        }
        else if (key == "binpath")
        {
            opts.binpath = value;
        }
        else if (key == "group")
        {
            opts.group = value;
        }
        else if (key == "tag")
        {
            if (value != "yes" && value != "no")
            {
                throw std::invalid_argument("Error: Invalid tag value. Allowed: yes, no.");
            }
            opts.tag = value;
        }
        else if (key == "depend")
        {
            opts.depend = value;
        }
        else if (key == "obj")
        {
            opts.obj = value;
        }
        else if (key == "displayname")
        {
            opts.displayname = value;
        }
        else if (key == "password")
        {
            opts.password = value;
        }
        else
        {
            throw std::invalid_argument("Error: Unknown option '" + key + "='.");
        }
    }

    // If the first type is "interact", then a second type must be provided.
    if (opts.serviceType == "interact" && !secondTypeProvided)
    {
        throw std::invalid_argument("Error: When type is 'interact', a second type parameter (own/share) must be provided.");
    }
}

namespace
{
    // Map the string service type to a DWORD value.
    DWORD MapServiceType(const ConfigOptions &opts)
    {
        if (opts.serviceType == "own")
        {
            return SERVICE_WIN32_OWN_PROCESS;
        }
        else if (opts.serviceType == "share")
        {
            return SERVICE_WIN32_SHARE_PROCESS;
        }
        else if (opts.serviceType == "kernel")
        {
            return SERVICE_KERNEL_DRIVER;
        }
        else if (opts.serviceType == "filesys")
        {
            return SERVICE_FILE_SYSTEM_DRIVER;
        }
        else if (opts.serviceType == "rec")
        {
            return SERVICE_RECOGNIZER_DRIVER;
        }
        else if (opts.serviceType == "adapt")
        {
            return SERVICE_DRIVER;
        }
        else if (opts.serviceType == "interact")
        {
            DWORD base = 0;
            if (opts.interactType == "own")
            {
                base = SERVICE_WIN32_OWN_PROCESS;
            }
            else if (opts.interactType == "share")
            {
                base = SERVICE_WIN32_SHARE_PROCESS;
            }
            else
            {
                throw std::invalid_argument("Error: Invalid interact type.");
            }
            return base | SERVICE_INTERACTIVE_PROCESS;
        }
        throw std::invalid_argument("Error: Unknown service type.");
    }

    // Map the string start type to a DWORD value.
    // Note: For "delayed-auto", we return SERVICE_AUTO_START and later set the delayed flag.
    DWORD MapStartType(const std::string &startType)
    {
        if (startType == "boot")
            return SERVICE_BOOT_START;
        if (startType == "system")
            return SERVICE_SYSTEM_START;
        if (startType == "auto")
            return SERVICE_AUTO_START;
        if (startType == "demand")
            return SERVICE_DEMAND_START;
        if (startType == "disabled")
            return SERVICE_DISABLED;
        if (startType == "delayed-auto")
            return SERVICE_AUTO_START;
        throw std::invalid_argument("Error: Invalid start type.");
    }

    // Map the string error control to a DWORD value.
    DWORD MapErrorControl(const std::string &errorControl)
    {
        if (errorControl == "normal")
            return SERVICE_ERROR_NORMAL;
        if (errorControl == "severe")
            return SERVICE_ERROR_SEVERE;
        if (errorControl == "critical")
            return SERVICE_ERROR_CRITICAL;
        if (errorControl == "ignore")
            return SERVICE_ERROR_IGNORE;
        throw std::invalid_argument("Error: Invalid error control value.");
    }
} // end anonymous namespace
// --- Delayed Auto-Start Definitions ---
// Some SDKs might not define these. We define them if needed.
#ifndef SERVICE_CONFIG_DELAYED_AUTO_START_INFO
#define SERVICE_CONFIG_DELAYED_AUTO_START_INFO 3
typedef struct _SERVICE_DELAYED_AUTO_START_INFO
{
    BOOL fDelayedAutostart;
} SERVICE_DELAYED_AUTO_START_INFO, *LPSERVICE_DELAYED_AUTO_START_INFO;
#endif

// --- config function ---
// This function opens the service and calls ChangeServiceConfigA with the provided options.
// If startType is "delayed-auto", then after ChangeServiceConfigA succeeds, it calls
// ChangeServiceConfig2A with SERVICE_CONFIG_DELAYED_AUTO_START_INFO to set the delayed flag.
void config(const ConfigOptions &opts)
{
    // For local queries, pass NULL instead of the server name.
    const char *machineName = (opts.serverName == "\\\\local" || opts.serverName.empty())
                                  ? NULL
                                  : opts.serverName.c_str();

    // Open the Service Control Manager.
    SC_HANDLE hSCManager = OpenSCManagerA(machineName, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
    {
        std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
        return;
    }

    // Open the target service with CHANGE_CONFIG access.
    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_ALL_ACCESS);
    if (!hService)
    {
        std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hSCManager);
        return;
    }

    // Map string options to DWORD values.
    DWORD dwServiceType = MapServiceType(opts);
    DWORD dwStartType = MapStartType(opts.startType);
    DWORD dwErrorControl = MapErrorControl(opts.errorControl);

    // If tag is "yes", a tag is requested.
    DWORD tagId = 0;
    LPDWORD lpdwTagId = (opts.tag == "yes") ? &tagId : nullptr;

    // Call ChangeServiceConfigA.
    BOOL result = ChangeServiceConfigA(
        hService,
        dwServiceType,
        dwStartType,
        dwErrorControl,
        opts.binpath.empty() ? NULL : opts.binpath.c_str(),
        opts.group.empty() ? NULL : opts.group.c_str(),
        lpdwTagId,
        opts.depend.empty() ? NULL : opts.depend.c_str(),
        opts.obj.empty() ? NULL : opts.obj.c_str(),
        opts.password.empty() ? NULL : opts.password.c_str(),
        opts.displayname.empty() ? NULL : opts.displayname.c_str());

    if (!result)
    {
        std::cerr << "ChangeServiceConfigA failed, error: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "[SC] ChangeServiceConfig SUCCESS\n";
        std::cout << "SERVICE_NAME: " << opts.serviceName << "\n";
    }

    // If the start type is "delayed-auto", call ChangeServiceConfig2A to set the delayed auto-start flag.
    if (opts.startType == "delayed-auto")
    {
        SERVICE_DELAYED_AUTO_START_INFO delayedInfo;
        delayedInfo.fDelayedAutostart = TRUE;
        if (!ChangeServiceConfig2A(hService, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo))
        {
            std::cerr << "ChangeServiceConfig2A (delayed-auto) failed, error: " << GetLastError() << "\n";
        }
        else
        {
            std::cout << "[SC] Delayed Auto-Start configured successfully.\n";
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}