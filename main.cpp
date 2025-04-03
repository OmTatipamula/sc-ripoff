#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "create_service.h"
#include "query.h"
#include "qdescription.h"
#include "start.h"
#include "delete.h"
#include "config.h"
#include "failure.h"

void printHelp()
{
    std::cout << R"(DESCRIPTION:
        SC is a command line program used for communicating with the
        Service Control Manager and services.
USAGE:
        sc <server> [command] [service name] <option1> <option2>...


        The option <server> has the form "\\ServerName"
        Further help on commands can be obtained by typing: "sc [command]"
        Commands:
          query-----------Queries the status for a service, or
                          enumerates the status for types of services.
          queryex---------Queries the extended status for a service, or
                          enumerates the status for types of services.
          start-----------Starts a service.
          pause-----------Sends a PAUSE control request to a service.
          interrogate-----Sends an INTERROGATE control request to a service.
          continue--------Sends a CONTINUE control request to a service.
          stop------------Sends a STOP request to a service.
          config----------Changes the configuration of a service (persistent).
          description-----Changes the description of a service.
          failure---------Changes the actions taken by a service upon failure.
          failureflag-----Changes the failure actions flag of a service.
          sidtype---------Changes the service SID type of a service.
          privs-----------Changes the required privileges of a service.
          managedaccount--Changes the service to mark the service account
                          password as managed by LSA.
          qc--------------Queries the configuration information for a service.
          qdescription----Queries the description for a service.
          qfailure--------Queries the actions taken by a service upon failure.
          qfailureflag----Queries the failure actions flag of a service.
          qsidtype--------Queries the service SID type of a service.
          qprivs----------Queries the required privileges of a service.
          qtriggerinfo----Queries the trigger parameters of a service.
          qpreferrednode--Queries the preferred NUMA node of a service.
          qmanagedaccount-Queries whether a services uses an account with a
                          password managed by LSA.
          qprotection-----Queries the process protection level of a service.
          quserservice----Queries for a local instance of a user service template.
          delete----------Deletes a service (from the registry).
          create----------Creates a service. (adds it to the registry).
          control---------Sends a control to a service.
          sdshow----------Displays a service's security descriptor.
          sdset-----------Sets a service's security descriptor.
          showsid---------Displays the service SID string corresponding to an arbitrary name.
          triggerinfo-----Configures the trigger parameters of a service.
          preferrednode---Sets the preferred NUMA node of a service.
          GetDisplayName--Gets the DisplayName for a service.
          GetKeyName------Gets the ServiceKeyName for a service.
          EnumDepend------Enumerates Service Dependencies.

        The following commands don't require a service name:
        sc <server> <command> <option>
          boot------------(ok | bad) Indicates whether the last boot should
                          be saved as the last-known-good boot configuration
          Lock------------Locks the Service Database
          QueryLock-------Queries the LockStatus for the SCManager Database
EXAMPLE:
        sc start MyService


QUERY and QUERYEX OPTIONS:
        If the query command is followed by a service name, the status
        for that service is returned.  Further options do not apply in
        this case.  If the query command is followed by nothing or one of
        the options listed below, the services are enumerated.
    type=    Type of services to enumerate (driver, service, userservice, all)
             (default = service)
    state=   State of services to enumerate (inactive, all)
             (default = active)
    bufsize= The size (in bytes) of the enumeration buffer
             (default = 4096)
    ri=      The resume index number at which to begin the enumeration
             (default = 0)
    group=   Service group to enumerate
             (default = all groups)

SYNTAX EXAMPLES
sc query                - Enumerates status for active services & drivers
sc query eventlog       - Displays status for the eventlog service
sc queryex eventlog     - Displays extended status for the eventlog service
sc query type= driver   - Enumerates only active drivers
sc query type= service  - Enumerates only Win32 services
sc query state= all     - Enumerates all services & drivers
sc query bufsize= 50    - Enumerates with a 50 byte buffer
sc query ri= 14         - Enumerates with resume index = 14
sc queryex group= ""    - Enumerates active services not in a group
sc query type= interact - Enumerates all interactive services
sc query type= driver group= NDIS     - Enumerates all NDIS drivers
)";
}

bool StartsWith(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printHelp();
        return EXIT_FAILURE;
    }

    // Build a vector of tokens from command-line arguments (skip argv[0]).
    std::vector<std::string> tokens;
    for (int i = 1; i < argc; ++i)
    {
        tokens.push_back(argv[i]);
    }

    size_t idx = 0;
    // Check for an optional server name (it should be in UNC format, i.e. start with "\\")
    std::string serverName = "";
    if (idx < tokens.size() && StartsWith(tokens[idx], "\\\\"))
    {
        serverName = tokens[idx];
        ++idx;
    }

    if (idx >= tokens.size())
    {
        std::cerr << "Error: Missing subcommand.\n";
        printHelp();
        return EXIT_FAILURE;
    }

    // The next token is the subcommand.
    std::string subcommand = tokens[idx++];
    const std::vector<std::string> validSubcommands = {
        "query", "create", "qdescription", "start", "stop", "config", "failure", "delete"};
    if (std::find(validSubcommands.begin(), validSubcommands.end(), subcommand) == validSubcommands.end())
    {
        std::cerr << "Error: Unknown subcommand '" << subcommand << "'.\n"
                  << "Allowed subcommands: query, create, qdescription, start, stop, config, failure.\n";
        return EXIT_FAILURE;
    }

    // Collect all remaining tokens for the subcommand parser.
    std::vector<std::string> subcommandArgs;
    for (; idx < tokens.size(); ++idx)
    {
        subcommandArgs.push_back(tokens[idx]);
    }

    // Dispatch based on the subcommand.
    if (subcommand == "query")
    {
        QueryOptions queryOpts;
        queryOpts.serverName = serverName;
        ParseQueryOptions(subcommandArgs, queryOpts);

        // For demonstration, print the parsed query options.

        // Here you would call the Win32 API functions using the values in queryOpts.
    }
    else if (subcommand == "qdescription")
    {
        QdescriptionOptions qdescriptionOpts;
        qdescriptionOpts.serverName = serverName; // assuming serverName is already defined
        ParseQdescriptionOptions(subcommandArgs, qdescriptionOpts);
    }
    else if (subcommand == "stop")
    {
        StartStopOptions startStopOpts;
        startStopOpts.serverName = serverName;
        startStopOpts.serviceName = subcommandArgs[0];
        stopService(startStopOpts);
    }
    else if (subcommand == "start")
    {
        StartStopOptions startStopOpts;
        startStopOpts.serverName = serverName;
        startStopOpts.serviceName = subcommandArgs[0];
        startService(startStopOpts);
    }
    else if (subcommand == "create")
    {
        CreateOptions createOpts;
        createOpts.serverName = serverName;
        ParseCreateOptions(subcommandArgs, createOpts);
        createService(createOpts);
    }
    else if (subcommand == "delete")
    {
        DeleteOptions delOpts;
        delOpts.serverName = serverName;
        ParseDeleteOptions(subcommandArgs, delOpts);
        deleteService(delOpts);
    }
    else if (subcommand == "config")
    {
        ConfigOptions configOpts;
        configOpts.serverName = serverName;
        ParseConfigOptions(subcommandArgs, configOpts);
        config(configOpts);
    }
    else if (subcommand == "failure")
    {
        FailureOptions failOpts;
        failOpts.serverName = serverName;
        ParseFailureOptions(subcommandArgs, failOpts);
        failure(failOpts);
    }
    else
    {
        // For other subcommands, simply forward the arguments to the corresponding parsing function.
        // (e.g., if (subcommand == "create") { ParseCreateOptions(subcommandArgs, createOpts); ... } )
        std::cout << "Subcommand '" << subcommand
                  << "' received with arguments:\n";
        for (const auto &arg : subcommandArgs)
        {
            std::cout << "  " << arg << "\n";
        }
        // Invoke the subcommand-specific parser and functionality here.
    }

    return EXIT_SUCCESS;
}