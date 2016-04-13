/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <thread>

#include "log.h"

namespace petrel {

std::unique_ptr<log> log::m_instance = nullptr;
bool log::m_syslog = false;
int log::m_filter_priority;
std::string log_color::darkgray;
std::string log_color::red;
std::string log_color::red_bright;
std::string log_color::green;
std::string log_color::yellow;
std::string log_color::blue_bright;
std::string log_color::blue;
std::string log_color::magenta;
std::string log_color::magenta_bright;
std::string log_color::cyan;
std::string log_color::white;
std::string log_color::reset;

void format(std::ostream& os, const boost::string_ref& str, std::size_t width, const boost::string_ref& color) {
    if (log::m_syslog) {
        os << "[" << str << "] ";
    } else {
        os << std::right << color << std::setw(width) << str << (color.empty() ? "" : log_color::reset) << " | ";
    }
}

log::log(std::string ident, int facility, bool syslog, int priority) {
    m_facility = facility;
    m_priority = static_cast<int>(log_priority::info);
    strncpy(m_ident, ident.c_str(), sizeof(m_ident));
    m_ident[sizeof(m_ident) - 1] = '\0';
    m_syslog = syslog;
    m_filter_priority = priority;
    if (m_syslog) {
        openlog(m_ident, LOG_PID, m_facility);
    } else {
        log_color::darkgray = "\x1b[30;1m";
        log_color::red = "\x1b[31m";
        log_color::red_bright = "\x1b[31;1m";
        log_color::green = "\x1b[32m";
        log_color::yellow = "\x1b[33m";
        log_color::blue_bright = "\x1b[34;1m";
        log_color::blue = "\x1b[34m";
        log_color::magenta = "\x1b[35m";
        log_color::magenta_bright = "\x1b[35;1m";
        log_color::cyan = "\x1b[36m";
        log_color::white = "\x1b[37;1m";
        log_color::reset = "\x1b[0m";
    }
}

int log::sync() {
    if (m_buffer.length()) {
        if (m_syslog) {
            syslog(m_priority, "%s", m_buffer.c_str());
        } else {
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);
            std::string fmt = "%h %e %T";
            std::ostringstream tstr;
            std::use_facet<std::time_put<char>>(std::clog.getloc())
                .put(std::ostreambuf_iterator<char>(tstr), tstr, ' ', &tm, &fmt[0], &fmt[0] + fmt.size());
            format(std::cout, tstr.str(), 15, log_color::darkgray);
            std::cout << m_buffer;
        }
        m_buffer.erase();
        m_priority = LOG_DEBUG;  // default to debug for each message
    }
    return 0;
}

int log::overflow(int c) {
    if (c != EOF) {
        m_buffer += static_cast<char>(c);
    } else {
        sync();
    }
    return c;
}

std::ostream& operator<<(std::ostream& os, const log_priority& priority) {
    static_cast<log*>(os.rdbuf())->m_priority = static_cast<int>(priority);
    int width = 10;
    switch (priority) {
        case log_priority::emerg:
            format(os, "emergency", width, log_color::red_bright);
            break;
        case log_priority::alert:
            format(os, "alert", width, log_color::red);
            break;
        case log_priority::crit:
            format(os, "critical", width, log_color::red_bright);
            break;
        case log_priority::err:
            format(os, "error", width, log_color::red_bright);
            break;
        case log_priority::warn:
            format(os, "warning", width, log_color::magenta_bright);
            break;
        case log_priority::notice:
            format(os, "notice", width, log_color::green);
            break;
        case log_priority::info:
            format(os, "info", width, log_color::yellow);
            break;
        case log_priority::debug:
            format(os, "debug", width, log_color::blue_bright);
            break;
        case log_priority::none:
            format(os, "none", width, log_color::red);
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const log_tag& tag) {
    static std::size_t width = 17;
    if (tag.str.length() > width) {
        width = tag.str.length();
    }
    format(os, tag.str, width, log_color::white);
    return os;
}

}  // petrel
