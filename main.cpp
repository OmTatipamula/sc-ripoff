#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include  "create_service.h"
#include "query.h"
#include "qdescription.h"
#include "start.h"
#include "delete.h"
#include "config.h"
#include "failure.h"


// Function to display help text.
void printHelp() {
    std::cout << "Replace this with help later\n";
}


/*
void create(const CreateOptions &opts) {
    std::cout << "Called create() with options:\n";
    std::cout << "  Server Name: " << (opts.serverName.empty() ? "Local" : opts.serverName) << "\n";
    std::cout << "  Service Name: " << opts.serviceName << "\n";
    std::cout << "  Service Type: " << opts.serviceType;
    if (opts.serviceType == "interact") {
        std::cout << " (" << opts.interactType << ")";
    }
    std::cout << "\n";
    std::cout << "  Start Type: " << opts.startType << "\n";
    std::cout << "  Error Control: " << opts.errorControl << "\n";
    std::cout << "  Binpath: " << opts.binpath << "\n";
    std::cout << "  Group: " << opts.group << "\n";
    std::cout << "  Tag: " << opts.tag << "\n";
    std::cout << "  Depend: " << opts.depend << "\n";
    std::cout << "  Obj: " << opts.obj << "\n";
    std::cout << "  Display Name: " << opts.displayname << "\n";
    std::cout << "  Password: " << opts.password << "\n";
}
*/




bool StartsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return EXIT_FAILURE;
    }

    // Build a vector of tokens from command-line arguments (skip argv[0]).
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; ++i) {
        tokens.push_back(argv[i]);
    }

    size_t idx = 0;
    // Check for an optional server name (it should be in UNC format, i.e. start with "\\")
    std::string serverName = "";
    if (idx < tokens.size() && StartsWith(tokens[idx], "\\\\")) {
        serverName = tokens[idx];
        ++idx;
    }

    if (idx >= tokens.size()) {
        std::cerr << "Error: Missing subcommand.\n";
        printHelp();
        return EXIT_FAILURE;
    }

    // The next token is the subcommand.
    std::string subcommand = tokens[idx++];
    const std::vector<std::string> validSubcommands = {
        "query", "create", "qdescription", "start", "stop", "config", "failure", "delete"
    };
    if (std::find(validSubcommands.begin(), validSubcommands.end(), subcommand) == validSubcommands.end()) {
        std::cerr << "Error: Unknown subcommand '" << subcommand << "'.\n"
                  << "Allowed subcommands: query, create, qdescription, start, stop, config, failure.\n";
        return EXIT_FAILURE;
    }

    // Collect all remaining tokens for the subcommand parser.
    std::vector<std::string> subcommandArgs;
    for (; idx < tokens.size(); ++idx) {
        subcommandArgs.push_back(tokens[idx]);
    }

    // Dispatch based on the subcommand.
    if (subcommand == "query") {
        QueryOptions queryOpts;
        queryOpts.serverName = serverName;
        ParseQueryOptions(subcommandArgs, queryOpts);

        // For demonstration, print the parsed query options.

        // Here you would call the Win32 API functions using the values in queryOpts.
    }
    else if (subcommand == "qdescription") {
        QdescriptionOptions qdescriptionOpts;
        qdescriptionOpts.serverName = serverName;  // assuming serverName is already defined
        ParseQdescriptionOptions(subcommandArgs, qdescriptionOpts);
    }
    else if (subcommand == "stop") {
        StartStopOptions startStopOpts;
        startStopOpts.serverName = serverName;
        startStopOpts.serviceName = subcommandArgs[0];
        stopService(startStopOpts);
    }
    else if (subcommand == "start") {
        StartStopOptions startStopOpts;
        startStopOpts.serverName = serverName;
        startStopOpts.serviceName = subcommandArgs[0];
        startService(startStopOpts);
    }
    else if (subcommand == "create") {
        CreateOptions createOpts;
        createOpts.serverName = serverName;
        ParseCreateOptions(subcommandArgs, createOpts);
        createService(createOpts);
    }
    else if (subcommand == "delete") {
        DeleteOptions delOpts;
        delOpts.serverName = serverName;
        ParseDeleteOptions(subcommandArgs, delOpts);
        deleteService(delOpts);
    }
    else if (subcommand == "config") {
        ConfigOptions configOpts;
        configOpts.serverName = serverName;
        ParseConfigOptions(subcommandArgs, configOpts);
        config(configOpts);
    }
    else if (subcommand == "failure") {
        FailureOptions failOpts;
        failOpts.serverName = serverName;
        ParseFailureOptions(subcommandArgs, failOpts);
        failure(failOpts);
    }
    else {
        // For other subcommands, simply forward the arguments to the corresponding parsing function.
        // (e.g., if (subcommand == "create") { ParseCreateOptions(subcommandArgs, createOpts); ... } )
        std::cout << "Subcommand '" << subcommand 
                  << "' received with arguments:\n";
        for (const auto& arg : subcommandArgs) {
            std::cout << "  " << arg << "\n";
        }
        // Invoke the subcommand-specific parser and functionality here.
    }

    return EXIT_SUCCESS;
}