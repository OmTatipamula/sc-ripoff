#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>

// Structure for the "config" subcommand options.
// Command-line syntax:
//   sc.exe [<servername>] config <servicename>
//         [type= {own | share | kernel | filesys | rec | adapt | interact type= {own | share}}]
//         [start= {boot | system | auto | demand | disabled | delayed-auto}]
//         [error= {normal | severe | critical | ignore}]
//         [binpath= <binarypathname>]
//         [group= <loadordergroup>]
//         [tag= {yes | no}]
//         [depend= <dependencies>]
//         [obj= {<accountname> | <objectname>}]
//         [displayname= <displayname>]
//         [password= <password>]
struct ConfigOptions
{
    std::string serverName;              // Optional server name; if empty or "\\local", assume local.
    std::string serviceName;             // Required service name.
    std::string serviceType = "own";     // Allowed: own, share, kernel, filesys, rec, adapt, interact.
    std::string interactType = "";       // If serviceType == "interact", must be provided: allowed: own, share.
    std::string startType = "demand";    // Allowed: boot, system, auto, demand, disabled, delayed-auto.
    std::string errorControl = "normal"; // Allowed: normal, severe, critical, ignore.
    std::string binpath = "";            // Path to the service binary (required for create; optional for config).
    std::string group = "";              // Load order group.
    std::string tag = "no";              // Allowed: yes, no.
    std::string depend = "";             // Dependencies (separated by forward slashes).
    std::string obj = "LocalSystem";     // Account name.
    std::string displayname = "";        // Friendly display name.
    std::string password = "";           // Password.
};

// Parse function for "config" options. The tokens in args start with the service name and then
// appear in key/value pairs (a token ending with '=' then a value token). If any token is missing or invalid,
// an std::invalid_argument exception is thrown.
void ParseConfigOptions(const std::vector<std::string> &args, ConfigOptions &opts);

// config function: reconfigures the service by calling ChangeServiceConfigA (and, for delayed-auto, ChangeServiceConfig2A).
void config(const ConfigOptions &opts);

#endif // CONFIG_H
