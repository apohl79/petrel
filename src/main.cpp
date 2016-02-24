/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "log.h"
#include "options.h"
#include "server.h"
#include "server_impl.h"

void sig_handler(petrel::server& s, const petrel::bs::error_code& ec, int signal_number) {
    if (!ec) {
        set_log_tag("main");
        log_info("Received signal " << signal_number);
        s.impl()->stop();
    }
}

int main(int argc, const char** argv) {
    // Parse command line
    if (!petrel::options::parse(argc, argv)) {
        return 1;
    }
    if (petrel::options::opts.count("help")) {
        std::cout << "petrel version: petrel/" << PETREL_VERSION << std::endl
                  << "Usage: " << argv[0] << " <options>" << std::endl
                  << petrel::options::desc << std::endl;
        return 0;
    }
    if (petrel::options::opts.count("version")) {
        std::cout << "petrel version: petrel/" << PETREL_VERSION << std::endl;
        return 0;
    }
    if (petrel::options::opts.count("test")) {
        std::cout << "Config OK" << std::endl;
        return 0;
    }

    petrel::log::init(petrel::options::opts.count("log.syslog"), petrel::options::opts["log.level"].as<int>());

    try {
        petrel::server s;
        petrel::ba::io_service iosvc;
        petrel::ba::signal_set signals(iosvc, SIGINT, SIGTERM);
        signals.async_wait(std::bind(sig_handler, std::ref(s), std::placeholders::_1, std::placeholders::_2));
        s.impl()->init();
        s.impl()->start();
        iosvc.run();  // wait for a signal to stop
        s.impl()->join();
    } catch (std::exception& e) {
        set_log_tag("main");
        log_emerg(e.what());
        return 1;
    }

    return 0;
}
