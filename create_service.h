#ifndef CREATE_SERVICE_H
#define CREATE_SERVICE_H

#include <string>
#include <windows.h>
#include <vector>
// Structure for the "create" subcommand.
struct CreateOptions
{
    std::string serverName;
    std::string serviceName;
    // The "type" parameter may appear twice.
    // The first occurrence sets the service type.
    // If it is "interact", then a second type parameter is expected to specify the process type.
    std::string serviceType = "own"; // Allowed: own, share, kernel, filesys, rec, interact
    std::string interactType = "";   // if serviceType==interact then this must be set. Allowed: own, share

    std::string startType = "demand";    // Allowed: boot, system, auto, demand, disabled, delayed-auto
    std::string errorControl = "normal"; // Allowed: normal, severe, critical, ignore
    std::string binpath = "";            // Must be provided
    std::string group = "";              // Load order group
    std::string tag = "no";              // Allowed: yes, no
    std::string depend = "";             // Dependencies (separated by forward slashes)
    std::string obj = "LocalSystem";     // Account name (default LocalSystem)
    std::string displayname = "";        // Friendly display name
    std::string password = "";           // Password (if needed)
};

// Function declaration for creating the service.
void createService(const CreateOptions &opts);

void ParseCreateOptions(const std::vector<std::string> &args, CreateOptions &opts);

#endif // CREATE_SERVICE_H