#include "start.h"

#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

// Wait constants.
static const int WAIT_INTERVAL_MS = 500;     // 500 milliseconds between polls.
static const int MAX_WAIT_MS = 30000;         // Wait up to 30 seconds.

// Helper: Determines the machine name to pass to OpenSCManagerA.
// If opts.serverName is empty or equals "\\\\local" (case-insensitive), returns NULL.
static const char* getMachineName(const std::string &serverName) {
    if (serverName.empty() || serverName == "\\\\local" || serverName == "\\local")
        return NULL;
    return serverName.c_str();
}

// Starts the specified service.
// Opens the SCM and service handle, calls StartServiceA, then waits until the service is RUNNING.
bool startService(const StartStopOptions &opts) {
    const char* machineName = getMachineName(opts.serverName);

    SC_HANDLE hSCManager = OpenSCManagerA(machineName, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
        return false;
    }

    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_START | SERVICE_QUERY_STATUS);
    if (!hService) {
        std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hSCManager);
        return false;
    }

    // Check current service status.
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD bytesNeeded = 0;
    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus),
                              sizeof(ssStatus), &bytesNeeded))
    {
        std::cerr << "QueryServiceStatusEx failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        std::cout << "Service is already running.\n";
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return true;
    }

    // Attempt to start the service.
    if (!StartServiceA(hService, 0, nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            std::cerr << "StartService failed, error: " << err << "\n";
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return false;
        }
    } else {
        std::cout << "StartService succeeded.\n";
    }

    // Wait for the service to reach the RUNNING state.
    auto startTime = std::chrono::steady_clock::now();
    while (ssStatus.dwCurrentState != SERVICE_RUNNING) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL_MS));
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus),
                                  sizeof(ssStatus), &bytesNeeded))
        {
            std::cerr << "QueryServiceStatusEx failed, error: " << GetLastError() << "\n";
            break;
        }
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > MAX_WAIT_MS) {
            std::cerr << "Timeout waiting for service to start.\n";
            break;
        }
    }

    bool result = (ssStatus.dwCurrentState == SERVICE_RUNNING);
    if (result)
        std::cout << "Service started successfully.\n";
    else
        std::cerr << "Service failed to start.\n";

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

// Stops the specified service.
// Opens the SCM and service handle, sends a SERVICE_CONTROL_STOP command, and waits until the service is STOPPED.
bool stopService(const StartStopOptions &opts) {
    const char* machineName = getMachineName(opts.serverName);

    SC_HANDLE hSCManager = OpenSCManagerA(machineName, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
        return false;
    }

    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!hService) {
        std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hSCManager);
        return false;
    }

    // Query current service status.
    SERVICE_STATUS_PROCESS ssStatus;
    DWORD bytesNeeded = 0;
    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus),
                              sizeof(ssStatus), &bytesNeeded))
    {
        std::cerr << "QueryServiceStatusEx failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
        std::cout << "Service is already stopped.\n";
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return true;
    }

    // Issue a stop command.
    if (!ControlService(hService, SERVICE_CONTROL_STOP, reinterpret_cast<LPSERVICE_STATUS>(&ssStatus))) {
        std::cerr << "ControlService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    } else {
        std::cout << "Stop command sent.\n";
    }

    // Wait for the service to reach the STOPPED state.
    auto startTime = std::chrono::steady_clock::now();
    while (ssStatus.dwCurrentState != SERVICE_STOPPED) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL_MS));
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus),
                                  sizeof(ssStatus), &bytesNeeded))
        {
            std::cerr << "QueryServiceStatusEx failed, error: " << GetLastError() << "\n";
            break;
        }
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > MAX_WAIT_MS) {
            std::cerr << "Timeout waiting for service to stop.\n";
            break;
        }
    }

    bool result = (ssStatus.dwCurrentState == SERVICE_STOPPED);
    if (result)
        std::cout << "Service stopped successfully.\n";
    else
        std::cerr << "Service failed to stop.\n";

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}
