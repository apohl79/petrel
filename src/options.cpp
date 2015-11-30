#include "options.h"
#include "log.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace petrel {

bpo::variables_map options::opts;
bpo::options_description options::desc;

bool options::parse(int argc, char** argv) {
    // clang-format off
    bpo::options_description desc_help("General options");
    desc_help.add_options()
        ("help,h", "Show help screen")
        ("config,c", bpo::value<std::string>(), "Config file")
        ("test,t", "Test a config file and exit")
        ;
    bpo::options_description desc_srv("Server options");
    desc_srv.add_options()
        ("server.http2",
           "Run a HTTP2 server")
        ("server.workers,w", bpo::value<int>()->default_value(1),
           "Number of worker threads. By setting this to 0, 1 worker per CPU gets created.")
        ("server.listen,l", bpo::value<std::string>()->default_value(""),
           "Listen on the given IP/name.")
        ("server.port,p", bpo::value<std::string>()->default_value("8585"),
           "Server port for TCP")
        ("server.backlog,b", bpo::value<int>()->default_value(0),
           "Listen backlog. Set this to 0 for max.")
        ("server.http2.tls",
           "Enable SSL/TLS support for HTTP2.")
        ("server.http2.key-file", bpo::value<std::string>(),
           "SSL/TLS private key file")
        ("server.http2.crt-file", bpo::value<std::string>(),
           "SSL/TLS certificate file")
        ("server.dns-cache-ttl", bpo::value<int>()->default_value(5),
           "DNS cache TTL in minutes")
        ;
    bpo::options_description desc_lua("LUA options");
    desc_lua.add_options()
        ("lua.root,r", bpo::value<std::string>()->required(),
           "The lua script root. All .lua files will be loaded.")
        ("lua.statebuffer", bpo::value<int>()->default_value(500),
           "The lua state buffer controlls how many lua state objects will be kept available by "
           "the lua engine for request handling to avoid creating states at handling time.")
        ("lua.devmode",
           "Activate the lua devmode. In this mode the server will reload the lua scripts on "
           "every single request.")
        ;
    bpo::options_description desc_log("Logging options");
    std::ostringstream level_msg;
    level_msg << "Log level: "
              << std::to_string(log::to_int(log_priority::emerg)) << "=emerg, "
              << std::to_string(log::to_int(log_priority::alert)) << "=alert, "
              << std::to_string(log::to_int(log_priority::crit)) << "=crit, "
              << std::to_string(log::to_int(log_priority::err)) << "=err, "
              << std::to_string(log::to_int(log_priority::warn)) << "=warn, "
              << std::to_string(log::to_int(log_priority::notice)) << "=notice, "
              << std::to_string(log::to_int(log_priority::info)) << "=info, "
              << std::to_string(log::to_int(log_priority::debug)) << "=debug";
    desc_log.add_options()
        ("log.syslog",
           "Log to syslog.")
        ("log.level", bpo::value<int>()->default_value(log::to_int(log_priority::info)),
           level_msg.str().c_str())
        ;
    bpo::options_description desc_metrics("Metrics options");
    desc_metrics.add_options()
        ("metrics.log", bpo::value<int>()->default_value(0),
           "Enable metrics logging every N seconds.")
        ("metrics.graphite",
           "Enable sending metrics to graphite.")
        ("metrics.graphite.interval", bpo::value<int>()->default_value(10),
           "Send the metrics every N seconds.")
        ("metrics.graphite.host", bpo::value<std::string>(),
           "The graphite host")
        ("metrics.graphite.port", bpo::value<std::string>()->default_value("2003"),
           "The graphite port")
        ("metrics.graphite.prefix", bpo::value<std::string>()->default_value("petrel"),
           "The name prefix for metrics send to graphite. A metric name will be constructed as "
           "follows: <prefix>.<hostname>.<metricname>")
        ;
    desc.add(desc_help).add(desc_srv).add(desc_lua).add(desc_log).add(desc_metrics);
    // clang-format on

    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), opts);
        if (opts.count("help")) {
            return true;
        }
        if (opts.count("config")) {
            bpo::options_description desc_file;
            desc_file.add(desc_srv).add(desc_lua).add(desc_log).add(desc_metrics);
            std::fstream f(opts["config"].as<std::string>());
            bpo::store(bpo::parse_config_file(f, desc_file), opts);
        }
        bpo::notify(opts);
        return true;
    } catch (bpo::error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return false;
}

}  // petrel
