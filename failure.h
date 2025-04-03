#ifndef FAILURE_H
#define FAILURE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>

// Structure for the "failure" subcommand options.
// Command-line syntax (after any optional server name):
//    failure <ServiceName> [reset= <ErrorFreePeriod>] [reboot= <BroadcastMessage>]
//                 [command= <CommandLine>] [actions= {"" | {[run/<MS>] | [restart/<MS>] | [reboot/<MS>]}[/...]]]
struct FailureOptions
{
    std::string serverName;  // Optional server name. If empty or "\\local", assume local.
    std::string serviceName; // Required: service name.
    int reset = 0;           // Error free period (in seconds). (reset=)
                             // Note: The reset parameter is required if actions= is provided.
    std::string reboot;      // Broadcast message (reboot=).
    std::string command;     // Command-line command to run on failure (command=).
    std::string actions;     // Failure actions string; for example: "restart/5000/reboot/10000" or "".
};

// Parse function for the failure subcommand options.
// The tokens in args should start with the service name (if the token does not contain '=' then it is assumed to be the service name)
// and then key/value pairs (with keys ending in '='). If any parameter is missing or malformed, an std::invalid_argument is thrown.
void ParseFailureOptions(const std::vector<std::string> &args, FailureOptions &opts);

// failure function: Configures the service failure actions by calling ChangeServiceConfig2A.
void failure(const FailureOptions &opts);

#endif // FAILURE_H
