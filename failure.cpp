#include "failure.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <windows.h>

// Helper function: Splits a string by a given delimiter.
static std::vector<std::string> splitString(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// EnableShutdownPrivilege enables the SE_SHUTDOWN_NAME privilege for the current process.
bool EnableShutdownPrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken failed, error: " << GetLastError() << "\n";
        return false;
    }
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &luid)) {
        std::cerr << "LookupPrivilegeValue failed, error: " << GetLastError() << "\n";
        CloseHandle(hToken);
        return false;
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        std::cerr << "AdjustTokenPrivileges failed, error: " << GetLastError() << "\n";
        CloseHandle(hToken);
        return false;
    }
    CloseHandle(hToken);
    return (GetLastError() == ERROR_SUCCESS);
}

// ParseFailureOptions: Parse command-line tokens into a FailureOptions struct.
void ParseFailureOptions(const std::vector<std::string>& args, FailureOptions &opts) {
    if (args.empty()) {
        throw std::invalid_argument("Error: failure requires a service name.");
    }
    size_t index = 0;
    // The first token is assumed to be the service name.
    if (args[0].find('=') == std::string::npos) {
        opts.serviceName = args[0];
        index++;
    } else {
        throw std::invalid_argument("Error: Service name is required as the first argument for failure.");
    }
    // Process remaining tokens as key/value pairs.
    while (index < args.size()) {
        std::string token = args[index];
        if (token.size() < 2 || token.back() != '=') {
            throw std::invalid_argument("Error: Invalid option format '" + token + "'. Expected key= followed by a value.");
        }
        std::string key = token.substr(0, token.size() - 1);
        index++;
        if (index >= args.size()) {
            throw std::invalid_argument("Error: Missing value for option '" + key + "='.");
        }
        std::string value = args[index];
        index++;
        
        if (key == "reset") {
            try {
                opts.reset = std::stoi(value);
            } catch (...) {
                throw std::invalid_argument("Error: reset must be an integer.");
            }
        }
        else if (key == "reboot") {
            opts.reboot = value;
        }
        else if (key == "command") {
            opts.command = value;
        }
        else if (key == "actions") {
            opts.actions = value;
        }
        else {
            throw std::invalid_argument("Error: Unknown option '" + key + "='.");
        }
    }
    // If actions is provided, reset must be non-zero.
    if (!opts.actions.empty() && opts.actions != "\"\"" && opts.reset == 0) {
        throw std::invalid_argument("Error: When actions= is specified, reset= (error-free period) must be provided and non-zero.");
    }
}

// failure: Configures service failure actions using ChangeServiceConfig2A.
void failure(const FailureOptions &opts) {
    // For local queries, if serverName is empty or "\\local", pass NULL.
    const char* machineName = (opts.serverName.empty() || opts.serverName == "\\\\local")
                              ? NULL : opts.serverName.c_str();

    // Open the Service Control Manager with all access.
    SC_HANDLE hSCManager = OpenSCManagerA(machineName, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        std::cerr << "OpenSCManager failed, error: " << GetLastError() << "\n";
        return;
    }

    // Open the service with all access.
    SC_HANDLE hService = OpenServiceA(hSCManager, opts.serviceName.c_str(), SERVICE_ALL_ACCESS);
    if (!hService) {
        std::cerr << "OpenService failed, error: " << GetLastError() << "\n";
        CloseServiceHandle(hSCManager);
        return;
    }

    // Use the ANSI version of the structure to match our LPSTR strings.
    SERVICE_FAILURE_ACTIONSA sfa;
    sfa.dwResetPeriod = opts.reset; // seconds
    sfa.lpRebootMsg = opts.reboot.empty() ? NULL : const_cast<LPSTR>(opts.reboot.c_str());
    sfa.lpCommand = opts.command.empty() ? NULL : const_cast<LPSTR>(opts.command.c_str());

    // Build the actions list.
    std::vector<SC_ACTION> actionsVector;
    if (opts.actions == "\"\"" || opts.actions.empty()) {
        // Supply a do-nothing action so that the reset period is applied.
        SC_ACTION noneAction;
        noneAction.Type = SC_ACTION_NONE;
        noneAction.Delay = 0;
        actionsVector.push_back(noneAction);
        sfa.cActions = 1;
        sfa.lpsaActions = actionsVector.data();
    } else {
        // The actions string is expected to be in the form, e.g., "restart/5000/reboot/30000"
        std::vector<std::string> tokens = splitString(opts.actions, '/');
        if (tokens.size() % 2 != 0) {
            throw std::invalid_argument("Error: actions parameter must consist of pairs of action and delay values.");
        }
        for (size_t i = 0; i < tokens.size(); i += 2) {
            SC_ACTION act;
            std::string actionType = tokens[i];
            if (actionType == "run") {
                if (opts.command.empty()) {
                    throw std::invalid_argument("Error: 'run' action specified but command parameter is missing (command= parameter).");
                }
                act.Type = SC_ACTION_RUN_COMMAND;
            } else if (actionType == "restart") {
                act.Type = SC_ACTION_RESTART;
            } else if (actionType == "reboot") {
                act.Type = SC_ACTION_REBOOT;
            } else {
                throw std::invalid_argument("Error: Unknown action type '" + actionType + "'. Allowed values: run, restart, reboot.");
            }
            try {
                act.Delay = static_cast<DWORD>(std::stoul(tokens[i+1]));
            } catch (...) {
                throw std::invalid_argument("Error: Invalid delay value in actions parameter.");
            }
            actionsVector.push_back(act);
        }
        sfa.cActions = static_cast<DWORD>(actionsVector.size());
        sfa.lpsaActions = actionsVector.data();
    }

    // If a reboot action is being configured, enable the shutdown privilege.
    if (opts.actions.find("reboot") != std::string::npos) {
        if (!EnableShutdownPrivilege()) {
            std::cerr << "Failed to enable shutdown privilege.\n";
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return;
        }
    }
    
    // Call ChangeServiceConfig2A to set the failure actions.
    if (!ChangeServiceConfig2A(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa)) {
        std::cerr << "ChangeServiceConfig2A failed, error: " << GetLastError() << "\n";
    } else {
        std::cout << "[SC] ChangeServiceConfig2 SUCCESS\n";
        std::cout << "SERVICE_NAME: " << opts.serviceName << "\n";
    }
    
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}
