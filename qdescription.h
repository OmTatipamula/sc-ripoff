#ifndef QDESCRIPTION_H
#define QDESCRIPTION_H

#include <string>
#include <vector>
#include <stdexcept>

// Structure for the "qdescription" subcommand options.
struct QdescriptionOptions
{
    std::string serverName = "";
    std::string serviceName;
    int bufsize = 1024; // Default buffer size
};

// Parse function to validate and fill in QdescriptionOptions.
void ParseQdescriptionOptions(const std::vector<std::string> &args, QdescriptionOptions &opts);

// qdescription function to query the service description.
void qdescription(const QdescriptionOptions &opts);

#endif // QDESCRIPTION_H
