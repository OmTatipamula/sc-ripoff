#pragma comment(lib, "advapi32.lib")

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <iomanip>

#include "query.h"


void help() {
    std::cout << "Usage:\n"
              << "  sc.exe [<servername>] query [<servicename>] \n"
              << "       [type= <driver|service|all>] [type= <own|share|interact|kernel|filesys|rec|adapt>]\n"
              << "       [state= <active|inactive|all>] [bufsize= <Buffersize>]\n"
              << "       [ri= <Resumeindex>] [group= <groupname>]\n";
}

// Parse all tokens (arguments) following the subcommand for the "query" subcommand.
// This function itself decides if the service name is provided as the first token.
int ParseQueryOptions(const std::vector<std::string>& tokens, QueryOptions& opts) {
    size_t index = 0;

    if(tokens.size() == 0)
    {
        query(opts);
        return true;
    }
    // If the first token does not contain '=' then treat it as the optional service name.
    if(tokens[index].find('=') == std::string::npos && tokens.size() > 1) {
        std::cerr << "Error: service name cannot be used with any other flags" << "\n";
        help();
        return EXIT_FAILURE;
    }
    if (index < tokens.size() && tokens[index].find('=') == std::string::npos) {
        opts.serviceName = tokens[index];
        query(opts);
        return true;
        //++index;
    }

    bool firstTypeFound = false;
    // Process remaining tokens as key-value pairs.
    while (index < tokens.size()) {
        // Each option is expected as a key token ending with '=' followed by its value.
        const std::string& keyToken = tokens[index];
        if (keyToken.size() < 2 || keyToken.back() != '=') {
            std::cerr << "Error: Option token '" << keyToken 
                      << "' is not correctly formatted. Expected key= followed by its value.\n";
            return false;
        }
        std::string key = keyToken.substr(0, keyToken.size() - 1);
        ++index; // Move to value token.
        if (index >= tokens.size()) {
            std::cerr << "Error: Missing value for option '" << key << "='\n";
            return false;
        }
        std::string value = tokens[index];
        ++index;

        if (key == "type") {
            if (!firstTypeFound) {
                if (value != "driver" && value != "service" && value != "all") {
                    std::cerr << "Error: Invalid value for type=. Allowed: driver, service, all.\n";
                    help();
                    return EXIT_FAILURE;
                }
                opts.enumType = value;
                firstTypeFound = true;
            } else {
                if (value != "own" && value != "share" && value != "interact" &&
                    value != "kernel" && value != "filesys" && value != "rec" && value != "adapt") {
                    std::cerr << "Error: Invalid value for second type=. Allowed: own, share, interact, kernel, filesys, rec, adapt.\n";
                    help();
                    return EXIT_FAILURE;
                }
                opts.type2Provided = true;
                opts.serviceType = value;
            }
        }
        else if (key == "state") {
            if (value != "active" && value != "inactive" && value != "all") {
                std::cerr << "Error: Invalid value for state=. Allowed: active, inactive, all.\n";
                help();
                return EXIT_FAILURE;
            }
            opts.state = value;
        }
        else if (key == "bufsize") {
            try {
                opts.bufsize = std::stoul(value);
            } catch (...) {
                std::cerr << "Error: Invalid numeric value for bufsize=.\n";
                help();
                return EXIT_FAILURE;
            }
        }
        else if (key == "ri") {
            try {
                opts.resumeIndex = std::stoul(value);
            } catch (...) {
                std::cerr << "Error: Invalid numeric value for ri=.\n";
                help();
                return EXIT_FAILURE;
            }
        }
        else if (key == "group") {
            opts.group = value;
        }
        else {
            std::cerr << "Error: Unknown option '" << key << "='\n";
            help();
            return EXIT_FAILURE;
        }
    }

    query(opts);
    return true;
}


/*
void query(const QueryOptions &opts) {
    std::cout << "Parsed Query Options:\n";
    std::cout << "  Server Name:  " << opts.serverName << "\n";
    std::cout << "  Service Name: " << opts.serviceName << "\n";
    std::cout << "  Enum Type:    " << opts.enumType << "\n";
    std::cout << "  Service Type: " << opts.serviceType << "\n";
    std::cout << "  State:        " << opts.state << "\n";
    std::cout << "  Bufsize:      " << opts.bufsize << "\n";
    std::cout << "  Resume Index: " << opts.resumeIndex << "\n";
    std::cout << "  Group:        " << opts.group << "\n";
}
*/

// Define our bit masks for convenience.
constexpr DWORD OWN_BIT       = SERVICE_WIN32_OWN_PROCESS;      // 0x10
constexpr DWORD SHARE_BIT     = SERVICE_WIN32_SHARE_PROCESS;    // 0x20
constexpr DWORD EXTRA_MASK    = 0xC0;                           // bits 0x40 and 0x80
constexpr DWORD INTERACTIVE_BIT = SERVICE_INTERACTIVE_PROCESS;  // 0x100

// Helper: Convert a numeric service state into a string.
std::string StateToString(DWORD state)
{
    switch (state)
    {
    case SERVICE_STOPPED:          return "STOPPED";
    case SERVICE_START_PENDING:    return "START_PENDING";
    case SERVICE_STOP_PENDING:     return "STOP_PENDING";
    case SERVICE_RUNNING:          return "RUNNING";
    case SERVICE_CONTINUE_PENDING: return "CONTINUE_PENDING";
    case SERVICE_PAUSE_PENDING:    return "PAUSE_PENDING";
    case SERVICE_PAUSED:           return "PAUSED";
    default:                       return "UNKNOWN";
    }
}

// Helper: Decode the dwServiceType field into a human–readable string.
// This function distinguishes among:
//   - Driver types: KERNEL_DRIVER, FILE_SYSTEM_DRIVER, RECOGNIZER_DRIVER
//   - Win32 service types:
//       • If both OWN and SHARE bits are set with no extra bits, return "WIN32" (as sc.exe does for WinHttpAutoProxySvc, type 0x30).
//       • If OWN is set (and SHARE is not) return "WIN32_OWN_PROCESS" (plus " INTERACTIVE" if applicable).
//       • If SHARE is set (and OWN is not) return either "WIN32_SHARE_PROCESS" or "USER_SHARE_PROCESS" (if bit 0x40 is set)
//         and append " INSTANCE" if bit 0x80 is set.
//       • If both OWN and SHARE are set but extra bits are present (as with 0xF0 for WpnUserService_a35a6), return "ERROR".
std::string DecodeServiceType(DWORD type)
{
    // Check for driver types.
    if (type & SERVICE_KERNEL_DRIVER)
        return "KERNEL_DRIVER";
    if (type & SERVICE_FILE_SYSTEM_DRIVER)
        return "FILE_SYSTEM_DRIVER";
    if (type & SERVICE_RECOGNIZER_DRIVER)
        return "RECOGNIZER_DRIVER";
    
    bool own   = (type & OWN_BIT) != 0;
    bool share = (type & SHARE_BIT) != 0;
    bool interactive = (type & INTERACTIVE_BIT) != 0;
    DWORD extras = type & EXTRA_MASK;

    // If both OWN and SHARE are set...
    if (own && share)
    {
        // If the only bits set are OWN and SHARE (ignoring interactive), return "WIN32".
        if (((type & ~(INTERACTIVE_BIT)) == (OWN_BIT | SHARE_BIT)))
            return interactive ? "WIN32 INTERACTIVE" : "WIN32";
        // Otherwise (if extras are present) return "ERROR" to match sc.exe for WpnUserService_a35a6.
        return "ERROR";
    }
    // If only OWN is set:
    if (own)
        return interactive ? "WIN32_OWN_PROCESS INTERACTIVE" : "WIN32_OWN_PROCESS";
    // If only SHARE is set:
    if (share)
    {
        std::string result;
        if (type & 0x40)
            result = "USER_SHARE_PROCESS";
        else
            result = "WIN32_SHARE_PROCESS";
        if (type & 0x80)
            result += " INSTANCE";
        if (interactive)
            result += " INTERACTIVE";
        return result;
    }
    return "ERROR";
}

// Updated PrintServiceStatus function.
void PrintServiceStatus(const std::string &serviceName, const std::string &displayName,
                        const SERVICE_STATUS_PROCESS &ssp)
{
    std::cout << "\n";
    std::cout << "SERVICE_NAME: " << serviceName << "\n";
    std::cout << "DISPLAY_NAME: " << displayName << "\n";

    // Print dwServiceType in hexadecimal without extra leading zeroes.
    std::ostringstream hexStream;
    hexStream << std::hex << std::nouppercase << ssp.dwServiceType;
    std::string typeHex = hexStream.str();

    // Decode the service type text.
    std::string typeDesc = DecodeServiceType(ssp.dwServiceType);
    std::cout << "        TYPE               : " << typeHex << "   " << typeDesc << "\n";

    // Print state.
    std::string stateStr = StateToString(ssp.dwCurrentState);
    std::cout << "        STATE              : " << ssp.dwCurrentState << "  " << stateStr << "\n";

    // Build a list of accepted control flags.
    std::vector<std::string> controls;
    // STOP control.
    if (ssp.dwControlsAccepted & SERVICE_ACCEPT_STOP)
        controls.push_back("STOPPABLE");
    else
        controls.push_back("NOT_STOPPABLE");
    // PAUSE control.
    if (ssp.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
        controls.push_back("PAUSABLE");
    else
        controls.push_back("NOT_PAUSABLE");
    // Shutdown controls.
    if (ssp.dwControlsAccepted & SERVICE_ACCEPT_PRESHUTDOWN)
    {
        controls.push_back("ACCEPTS_PRESHUTDOWN");
    }
    else
    {
        if (ssp.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN)
            controls.push_back("ACCEPTS_SHUTDOWN");
        else
            controls.push_back("IGNORES_SHUTDOWN");
    }
    
    // Print the control flags.
    std::cout << "                                (";
    for (size_t i = 0; i < controls.size(); ++i)
    {
        std::cout << controls[i];
        if (i != controls.size() - 1)
            std::cout << ", ";
    }
    std::cout << ")\n";

    std::cout << "        WIN32_EXIT_CODE    : " << ssp.dwWin32ExitCode
              << "  (0x" << std::hex << ssp.dwWin32ExitCode << std::dec << ")\n";
    std::cout << "        SERVICE_EXIT_CODE  : " << ssp.dwServiceSpecificExitCode
              << "  (0x" << std::hex << ssp.dwServiceSpecificExitCode << std::dec << ")\n";
    std::cout << "        CHECKPOINT         : 0x" << std::hex << ssp.dwCheckPoint << std::dec << "\n";
    std::cout << "        WAIT_HINT          : 0x" << std::hex << ssp.dwWaitHint << std::dec << "\n";
}


//
// The query function uses low-level Win32 APIs (ANSI versions) to query services similar to sc.exe.
// It uses the QueryOptions settings and applies the following logic:
//  - If a service name is provided, query that service only.
//  - Otherwise, enumerate services filtered by the "enumType" and "state" options.
//    If no second type= parameter was provided (type2Provided == false), then for "service"
//    enumeration we use SERVICE_WIN32 (both own and share) so that services like OneSyncSvc_a35a6 are not omitted.
//
void query(const QueryOptions &opts)
{
    if (!opts.serviceName.empty())
    {
        // Query a specific service.
        SC_HANDLE hSCManager = OpenSCManagerA(opts.serverName.c_str(), nullptr, SC_MANAGER_CONNECT);
        if (!hSCManager)
        {
            std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
            return;
        }
        SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
        if (!hService)
        {
            std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
            CloseServiceHandle(hSCManager);
            return;
        }

        SERVICE_STATUS_PROCESS ssp;
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssp),
                                  sizeof(ssp), &bytesNeeded))
        {
            std::cerr << "QueryServiceStatusEx failed, error: " << GetLastError() << "\n";
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }

        // Retrieve configuration (to get the display name).
        LPQUERY_SERVICE_CONFIGA config = nullptr;
        DWORD bytesNeeded2 = 0;
        QueryServiceConfigA(hService, nullptr, 0, &bytesNeeded2);
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            config = reinterpret_cast<LPQUERY_SERVICE_CONFIGA>(LocalAlloc(LPTR, bytesNeeded2));
            if (config && !QueryServiceConfigA(hService, config, bytesNeeded2, &bytesNeeded2))
            {
                std::cerr << "QueryServiceConfig failed, error: " << GetLastError() << "\n";
                LocalFree(config);
                CloseServiceHandle(hService);
                CloseServiceHandle(hSCManager);
                return;
            }
        }
        std::string displayName = (config && config->lpDisplayName) ? config->lpDisplayName : opts.serviceName;

        PrintServiceStatus(opts.serviceName, displayName, ssp);

        if (config)
            LocalFree(config);
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
    }
    else
    {
        // Enumerate services.
        SC_HANDLE hSCManager = OpenSCManagerA(opts.serverName.c_str(), nullptr, SC_MANAGER_ENUMERATE_SERVICE);
        if (!hSCManager)
        {
            std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
            return;
        }

        DWORD dwServiceType = 0;
        if (opts.enumType == "driver")
        {
            if (opts.serviceType == "kernel")
                dwServiceType = SERVICE_KERNEL_DRIVER;
            else if (opts.serviceType == "filesys")
                dwServiceType = SERVICE_FILE_SYSTEM_DRIVER;
            else if (opts.serviceType == "rec")
                dwServiceType = SERVICE_RECOGNIZER_DRIVER;
            else if (opts.serviceType == "adapt")
                dwServiceType = SERVICE_DRIVER;
            else
                dwServiceType = SERVICE_DRIVER;
        }
        else if (opts.enumType == "service")
        {
            // If a second type was explicitly provided, narrow by the provided type.
            // Otherwise, enumerate all Win32 services (both own and share) so that shared services such as OneSyncSvc_a35a6 are included.
            if (opts.type2Provided)
            {
                if (opts.serviceType == "own")
                    dwServiceType = SERVICE_WIN32_OWN_PROCESS;
                else if (opts.serviceType == "share")
                    dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
                else if (opts.serviceType == "interact")
                    dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
                else
                    dwServiceType = SERVICE_WIN32_OWN_PROCESS;
            }
            else
            {
                dwServiceType = SERVICE_WIN32; // Enumerate both own and share.
            }
        }
        else if (opts.enumType == "all")
        {
            dwServiceType = SERVICE_DRIVER | SERVICE_WIN32;
        }

        DWORD dwServiceState = 0;
        if (opts.state == "active")
            dwServiceState = SERVICE_ACTIVE;
        else if (opts.state == "inactive")
            dwServiceState = SERVICE_INACTIVE;
        else if (opts.state == "all")
            dwServiceState = SERVICE_STATE_ALL;

        DWORD bytesNeeded = 0, servicesReturned = 0;
        DWORD resumeHandle = opts.resumeIndex;
        BOOL success = EnumServicesStatusExA(
            hSCManager,
            SC_ENUM_PROCESS_INFO,
            dwServiceType,
            dwServiceState,
            nullptr,
            0,
            &bytesNeeded,
            &servicesReturned,
            &resumeHandle,
            opts.group.empty() ? nullptr : opts.group.c_str()
        );
        if (!success && GetLastError() != ERROR_MORE_DATA)
        {
            std::cerr << "EnumServicesStatusEx failed, error: " << GetLastError() << "\n";
            CloseServiceHandle(hSCManager);
            return;
        }

        std::vector<BYTE> buffer(bytesNeeded);
        success = EnumServicesStatusExA(
            hSCManager,
            SC_ENUM_PROCESS_INFO,
            dwServiceType,
            dwServiceState,
            buffer.data(),
            bytesNeeded,
            &bytesNeeded,
            &servicesReturned,
            &resumeHandle,
            opts.group.empty() ? nullptr : opts.group.c_str()
        );
        if (!success)
        {
            std::cerr << "EnumServicesStatusEx (second call) failed, error: " << GetLastError() << "\n";
            CloseServiceHandle(hSCManager);
            return;
        }

        LPENUM_SERVICE_STATUS_PROCESSA services = reinterpret_cast<LPENUM_SERVICE_STATUS_PROCESSA>(buffer.data());
        for (DWORD i = 0; i < servicesReturned; i++)
        {
            PrintServiceStatus(services[i].lpServiceName, services[i].lpDisplayName,
                               services[i].ServiceStatusProcess);
        }
        CloseServiceHandle(hSCManager);
    }
}