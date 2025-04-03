#ifndef QUERY_H
#define QUERY_H
#define SERVICE_ADAPTER 0x00000004
#define SERVICE_RECOGNIZER_DRIVER 0x00000008

#include <string>
#include <windows.h>

// Our QueryOptions structure.
struct QueryOptions
{
    // Optional server name. If empty, assume local.
    std::string serverName = "\\\\local";
    // Optional service name.
    std::string serviceName = "";
    // The first appearance of type=; allowed: driver, service, all (default "service")
    std::string enumType = "service";
    // The second appearance of type=; allowed: own, share, interact, kernel, filesys, rec, adapt.
    // If not provided, we will enumerate all Win32 services.
    std::string serviceType = "own";
    // A flag indicating if a second type parameter was explicitly provided.
    bool type2Provided = false;
    // State filter; allowed: active, inactive, all (default "active")
    std::string state = "active";
    // Enumeration buffer size (in bytes); default is 1024.
    unsigned int bufsize = 1024;
    // Resume index; default is 0.
    unsigned int resumeIndex = 0;
    // Optional group name; if empty then all groups are enumerated.
    std::string group = "";
};

// Function declaration for creating the service.
void query(const QueryOptions &opts);
// Function declaration for parsing query options
int ParseQueryOptions(const std::vector<std::string> &tokens, QueryOptions &opts);
#endif // CREATE_SERVICE_H