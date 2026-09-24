#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0502
    #endif
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#endif

#include "CLI.hpp"
#include "SliceCommand.hpp"

#include "libslic3r/libslic3r.h"
#include "libslic3r/PrintConfig.hpp"
#include "libslic3r/Utils.hpp"

#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/cenv.hpp>
#include <boost/nowide/iostream.hpp>

namespace Slic3r {
namespace CLI {
namespace {

struct CliCommandPlan
{
    std::string command;
    int error_code {CLI_SUCCESS};
    std::string error_message;

    bool valid() const
    {
        return error_code == CLI_SUCCESS;
    }
};

bool is_cli_command_parameter(const std::string &action)
{
    static const std::set<std::string> parameters
    {
        "pipe", "load_slicedata", "load_defaultfila", "mtcpp", "mstpp",
        "no_check", "normative_check", "export_slicedata"
    };

    return parameters.find(action) != parameters.end();
}

CliCommandPlan make_cli_command_plan(const std::vector<std::string> &actions)
{
    CliCommandPlan plan;
    for (const std::string &action : actions)
    {
        if (is_cli_command_parameter(action))
            continue;

        if (!plan.command.empty())
        {
            plan.error_code = CLI_INVALID_PARAMS;
            plan.error_message = "Only one CLI command may be specified";
            return plan;
        }

        plan.command = action;
    }

    if (plan.command.empty())
    {
        plan.error_code = CLI_INVALID_PARAMS;
        plan.error_message = "No CLI command was specified";
    }

    return plan;
}

struct CommandContext
{
    DynamicPrintAndCommandLineConfig config;
    DynamicPrintConfig               print_config;
    DynamicPrintConfig               extra_config;
    std::vector<std::string>         input_files;
    std::vector<std::string>         actions;
    std::vector<Model>               models;
};

void initialize_cli_data_directory(const DynamicPrintAndCommandLineConfig &config)
{
    const std::string configured_data_dir = config.opt_string("datadir");
    if (!configured_data_dir.empty()) {
        set_data_dir(configured_data_dir);
        return;
    }

    if (!data_dir().empty())
        return;

    boost::filesystem::path app_data_root;
#if defined(_WIN32)
    if (const char *app_data = boost::nowide::getenv("APPDATA"); app_data != nullptr && *app_data != '\0')
        app_data_root = boost::filesystem::path(app_data) / SLIC3R_APP_KEY;
#elif defined(__APPLE__)
    if (const char *user_home = boost::nowide::getenv("HOME"); user_home != nullptr && *user_home != '\0')
        app_data_root = boost::filesystem::path(user_home) / "Library" / "Application Support" / SLIC3R_APP_KEY;
#else
    if (const char *xdg_config_home = boost::nowide::getenv("XDG_CONFIG_HOME");
        xdg_config_home != nullptr && *xdg_config_home != '\0') {
        app_data_root = boost::filesystem::path(xdg_config_home) / SLIC3R_APP_KEY;
    } else if (const char *user_home = boost::nowide::getenv("HOME"); user_home != nullptr && *user_home != '\0') {
        app_data_root = boost::filesystem::path(user_home) / ".config" / SLIC3R_APP_KEY;
    }
#endif

    if (!app_data_root.empty()) {
        set_data_dir(app_data_root.string());
        BOOST_LOG_TRIVIAL(info) << "CLI data directory initialized from GUI default: " << data_dir();
    }
}

void attach_console_on_demand()
{
#ifdef _WIN32
    static bool console_attached = false;

    if (!console_attached) {
        if (AttachConsole(ATTACH_PARENT_PROCESS))
        {
            console_attached = true;
        }
        else if (GetLastError() == ERROR_ACCESS_DENIED)
        {
            console_attached = true;
        }
        else if (AllocConsole())
        {
            console_attached = true;
        }

        if (console_attached)
        {
            FILE* fp = nullptr;
            if (freopen_s(&fp, "CONOUT$", "w", stdout) == 0)
                setvbuf(stdout, nullptr, _IONBF, 0);

            if (freopen_s(&fp, "CONOUT$", "w", stderr) == 0)
                setvbuf(stderr, nullptr, _IONBF, 0);

            if (freopen_s(&fp, "CONIN$", "r", stdin) == 0)
            {
            }

            std::ios::sync_with_stdio(true);
            std::cout.clear();
            std::cerr.clear();
            std::cin.clear();
            boost::nowide::cout.clear();
            boost::nowide::cerr.clear();
            boost::nowide::cin.clear();
        }
    }
#endif
}

void print_help()
{
    attach_console_on_demand();

    boost::nowide::cout
        << SLIC3R_APP_KEY << "-" << SLIC3R_VERSION << ":"
        << std::endl
        << "Usage: CrealityPrint --cli [ OPTIONS ] [ file.3mf/file.stl ... ]" << std::endl
        << std::endl
        << "OPTIONS:" << std::endl;
    cli_misc_config_def.print_cli_help(boost::nowide::cout, false);
    cli_actions_config_def.print_cli_help(boost::nowide::cout, false);

    boost::nowide::cout
        << std::endl
        << "Configuration priority:" << std::endl
        << "\t3MF: command-line print overrides > settings embedded in the project" << std::endl
        << "\tSTL/OBJ: command-line print overrides > --load_settings/--load_filaments presets" << std::endl;
    boost::nowide::cout.flush();
    boost::nowide::cerr.flush();
}

bool parse_arguments(CommandContext &context, int argc, char **argv)
{
    t_config_option_keys option_order;
    if (!context.config.read_cli(argc, argv, &context.input_files, &option_order))
    {
        boost::nowide::cerr << std::endl;
        print_help();
        return false;
    }

    for (const auto &option_key : option_order)
    {
        if (cli_actions_config_def.has(option_key))
            context.actions.emplace_back(option_key);
    }

    const std::map<std::string, std::string> validity = context.config.validate(true);

    for (const t_optiondef_map *options : {
             &cli_actions_config_def.options,
             &cli_misc_config_def.options})
    {
        for (const t_optiondef_map::value_type &definition : *options)
            context.config.option(definition.first, true);
    }

    initialize_cli_data_directory(context.config);

    if (!validity.empty())
    {
        boost::nowide::cerr << "Params in command line error:" << std::endl;
        for (const auto &item : validity)
            boost::nowide::cerr << item.first << ": " << item.second << std::endl;
        return false;
    }

    context.extra_config.apply(context.config, true);
    context.extra_config.normalize_fdm();
    return true;
}

int dispatch_command(CommandContext &context)
{
    const CliCommandPlan command_plan = make_cli_command_plan(context.actions);
    if (!command_plan.valid())
    {
        boost::nowide::cerr << "error: " << command_plan.error_message << std::endl;
        return command_plan.error_code;
    }

    if (command_plan.command == "help")
    {
        print_help();
        return CLI_SUCCESS;
    }

    if (command_plan.command == "slice")
    {
        SliceCommand command(
            context.config,
            context.print_config,
            context.extra_config,
            context.input_files,
            context.actions,
            context.models);

        return command.run();
    }

    boost::nowide::cerr << "error: unsupported CLI command: "
                        << command_plan.command << std::endl;
    return CLI_UNSUPPORTED_OPERATION;
}

} // namespace

int run(int argc, char **argv)
{
    CommandContext context;

    if (!parse_arguments(context, argc, argv))
    {
        boost::nowide::cerr << "CLI parameter parsing failed" << std::endl;
        return CLI_INVALID_PARAMS;
    }

    return dispatch_command(context);
}

} // namespace CLI
} // namespace Slic3r
