#ifndef DELETE_SERVICE_H
#define DELETE_SERVICE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <windows.h>

// Structure for the "delete" subcommand options.
struct DeleteOptions {
    std::string serverName;  // Optional server name. If empty, defaults to local.
    std::string serviceName; // Required service name.
};

// Parse function for delete options.
// Expected usages:
//    delete <serviceName>
// or
//    delete <serverName> <serviceName>
// Throws std::invalid_argument if the arguments are missing or extra.
void ParseDeleteOptions(const std::vector<std::string>& args, DeleteOptions &opts);

// deleteService function: deletes the specified service.
void deleteService(const DeleteOptions &opts);

#endif // DELETE_SERVICE_H
