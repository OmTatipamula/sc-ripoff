#ifndef START_H
#define START_H

#include <string>

// Structure holding options for starting or stopping a service.
struct StartStopOptions
{
    std::string serverName;  // If empty or "\\\\local", local machine is used.
    std::string serviceName; // The service name (key name)
};

// Starts the specified service.
// Returns true on success, false on failure.
bool startService(const StartStopOptions &opts);

// Stops the specified service.
// Returns true on success, false on failure.
bool stopService(const StartStopOptions &opts);

#endif // START_H