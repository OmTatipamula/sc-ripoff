// Ensure _WIN32_WINNT and WINVER are defined early.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef WINVER
#define WINVER 0x0501
#endif

#include "qdescription.h"
#include <windows.h>
#include <winsvc.h>
#include <iostream>
#include <sstream>
#include <vector>

// If QUERY_SERVICE_CONFIG2A is not defined by the SDK, define it here.
#ifndef QUERY_SERVICE_CONFIG2A
typedef struct _QUERY_SERVICE_CONFIG2A
{
    LPSTR lpServiceSid;
    LPSTR lpDescription;
} QUERY_SERVICE_CONFIG2A, *LPQUERY_SERVICE_CONFIG2A;
#endif

// Also, if LPQUERY_SERVICE_CONFIG2A is not defined, define it.
#ifndef LPQUERY_SERVICE_CONFIG2A
typedef QUERY_SERVICE_CONFIG2A *LPQUERY_SERVICE_CONFIG2A;
#endif


void printQdescriptionHelp()
{
    std::cout << R"(DESCRIPTION:
        Retrieves the description string of a service.
USAGE:
        sc <server> qdescription [service name] <bufferSize>
)";
}


// Parse the command-line arguments (tokens) after "qdescription".
// Expected usage:
//    qdescription <serviceName>
// or
//    qdescription <serverName> <serviceName>
// If extra tokens are present, or if the serviceName is missing, throw an error.
void ParseQdescriptionOptions(const std::vector<std::string> &args, QdescriptionOptions &opts)
{
    if (args.empty())
    {
        throw std::invalid_argument("Error: qdescription requires a service name.");
    }

    size_t index = 0;
    // If the first token starts with "\\" then treat it as the server name.
    if (args[0].compare(0, 2, "\\\\") == 0)
    {
        opts.serverName = args[0];
        index++;
        if (index >= args.size())
        {
            throw std::invalid_argument("Error: qdescription requires a service name after the server name.");
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
        throw std::invalid_argument("Error: qdescription does not accept extra arguments.");
    }
    qdescription(opts);
}

void qdescription(const QdescriptionOptions &opts)
{
    // For the local machine, we pass NULL to OpenSCManagerA
    LPCSTR machineName = (opts.serverName == "\\\\local") ? NULL : opts.serverName.c_str();

    // Open a handle to the Service Control Manager.
    SC_HANDLE hSCManager = OpenSCManagerA(machineName, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        std::cerr << "Failed to open Service Control Manager. Error: " << GetLastError() << std::endl;
        return;
    }

    // Open the specified service with the SERVICE_QUERY_CONFIG access right.
    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_QUERY_CONFIG);
    if (!hService)
    {
        std::cerr << "Failed to open service \"" << opts.serviceName << "\". Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCManager);
        return;
    }

    // Allocate a buffer for the service description.
    std::vector<BYTE> buffer(opts.bufsize);
    DWORD bytesNeeded = 0;

    // Query the service description (SERVICE_CONFIG_DESCRIPTION)
    BOOL success = QueryServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, buffer.data(), opts.bufsize, &bytesNeeded);
    if (!success)
    {
        DWORD err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER)
        {
            // Resize buffer to the needed size and try again.
            buffer.resize(bytesNeeded);
            success = QueryServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, buffer.data(), bytesNeeded, &bytesNeeded);
        }
        if (!success)
        {
            std::cerr << "QueryServiceConfig2 failed. Error: " << GetLastError() << std::endl;
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }
    }

    // Cast the buffer to a SERVICE_DESCRIPTIONA pointer.
    SERVICE_DESCRIPTIONA *pDesc = reinterpret_cast<SERVICE_DESCRIPTIONA *>(buffer.data());
    if (pDesc && pDesc->lpDescription)
    {
        std::cout << "Service Description: " << pDesc->lpDescription << std::endl;
    }
    else
    {
        std::cout << "No description available for the service." << std::endl;
    }

    // Clean up open handles.
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}