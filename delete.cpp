#include "delete.h"
#include <iostream>
#include <sstream>
#include <vector>


void printDeleteHelp()
{
    std::cout << R"(DESCRIPTION:
        Deletes a service entry from the registry.
        If the service is running, or another process has an
        open handle to the service, the service is simply marked
        for deletion.
USAGE:
        sc <server> delete [service name]
)";
}

// ParseDeleteOptions: Parses tokens provided after the "delete" subcommand.
// If the first token starts with "\\" it is treated as the server name; otherwise,
// the first token is the service name and the server defaults to "\\local".
// If extra tokens exist, an exception is thrown.
void ParseDeleteOptions(const std::vector<std::string> &args, DeleteOptions &opts)
{
    if (args.empty())
    {
        throw std::invalid_argument("Error: delete requires a service name.");
    }

    size_t index = 0;
    // If the first token starts with "\\" then treat it as a server name.
    if (args[0].compare(0, 2, "\\\\") == 0)
    {
        opts.serverName = args[0];
        index++;
        if (index >= args.size())
        {
            throw std::invalid_argument("Error: delete requires a service name after the server name.");
        }
        opts.serviceName = args[index];
        index++;
    }
    else
    {
        // No server name provided; default to local.
        opts.serverName = "\\\\local";
        opts.serviceName = args[0];
        index++;
    }

    if (index < args.size())
    {
        throw std::invalid_argument("Error: delete does not accept extra arguments.");
    }
}

// deleteService: Deletes the service specified in opts using the Win32 API.
void deleteService(const DeleteOptions &opts)
{
    // For local queries, pass NULL to OpenSCManagerA.
    const char *machineName = ((opts.serverName == "\\\\local") || (opts.serverName.empty()))
                                  ? NULL
                                  : opts.serverName.c_str();

    // Open the Service Control Manager with connect rights.
    SC_HANDLE hSCManager = OpenSCManagerA(machineName, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
        return;
    }

    // Open the service with DELETE access.
    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), DELETE);
    if (!hService)
    {
        std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hSCManager);
        return;
    }

    // Call DeleteService.
    if (!DeleteService(hService))
    {
        std::cerr << "DeleteService failed, error: " << GetLastError() << "\n";
    }
    else
    {
        std::cout << "[SC] DeleteService SUCCESS\n";
        std::cout << "SERVICE_NAME: " << opts.serviceName << "\n";
    }

    // Cleanup.
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}