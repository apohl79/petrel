/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef LOG_H
#define LOG_H

#include <syslog.h>
#include <ostream>
#include <iostream>
#include <iomanip>

#include <boost/utility/string_ref.hpp>

namespace petrel {

enum class log_priority : int {
    emerg = LOG_EMERG,    // system is unusable
    alert = LOG_ALERT,    // action must be taken immediately
    crit = LOG_CRIT,      // critical conditions
    err = LOG_ERR,        // error conditions
    warn = LOG_WARNING,   // warning conditions
    notice = LOG_NOTICE,  // normal, but significant, condition
    info = LOG_INFO,      // informational message
    debug = LOG_DEBUG,    // debug-level message
    none,                 // internal
};

struct log_tag {
    log_tag(const char* s) : str(s) {}
    log_tag(const std::string& s) : str(s) {}
    log_tag& operator=(const log_tag& t) {
        str = t.str;
        return *this;
    }
    boost::string_ref str{""};
};

struct log_color {
    static std::string darkgray;
    static std::string red;
    static std::string red_bright;
    static std::string green;
    static std::string yellow;
    static std::string blue_bright;
    static std::string blue;
    static std::string magenta;
    static std::string magenta_bright;
    static std::string cyan;
    static std::string white;
    static std::string reset;
};

std::ostream& operator<<(std::ostream& os, const log_priority& priority);
std::ostream& operator<<(std::ostream& os, const log_tag& tag);

class log : public std::basic_streambuf<char, std::char_traits<char> > {
  public:
    explicit log(std::string ident, int facility, bool syslog, int priority);
    static bool is_log_priority(int priority) noexcept {
        auto p = static_cast<log_priority>(priority);
        switch (p) {
            case log_priority::emerg:
            case log_priority::alert:
            case log_priority::crit:
            case log_priority::err:
            case log_priority::warn:
            case log_priority::notice:
            case log_priority::info:
            case log_priority::debug:
                return true;
            case log_priority::none:
            default:
                return false;
        }
    }
    static log_priority to_priority(int priority) {
        if (is_log_priority(priority)) {
            return static_cast<log_priority>(priority);
        } else {
            throw std::invalid_argument("no log_priority");
        }
    }
    static constexpr int to_int(const log_priority& priority) noexcept { return static_cast<int>(priority); }
    static bool m_syslog;
    static int m_filter_priority;

protected:
    int sync();
    int overflow(int c);

private:
    friend std::ostream& operator<<(std::ostream& os, const log_priority& priority);
    std::string m_buffer;
    int m_facility;
    int m_priority;
    char m_ident[50];
};

/// Convinience helper macros
#define decl_log_static()        \
    static std::string __petrel_log_tag; \
    static int __petrel_log_prio
#define init_log_static(T, P) \
    std::string T::__petrel_log_tag = #T; \
    int T::__petrel_log_prio = ::petrel::log::to_int(P)

#define set_log_tag(T) \
    std::string __petrel_log_tag { T }
#define get_log_tag() __petrel_log_tag
#define get_log_tag_static(T) T::__petrel_log_tag
#define update_log_tag(T) __petrel_log_tag = T

#define set_log_priority(P)                             \
    int __petrel_log_prio { ::petrel::log::to_int(P) }
#define get_log_priority() ::petrel::log::to_priority(__petrel_log_prio)
#define get_log_priority_static(T) ::petrel::log::to_priority(T::__petrel_log_prio)
#define update_log_priority(P) __petrel_log_prio = ::petrel::log::to_int(P)

#define set_log_tag_default_priority(T) \
    set_log_tag(T);                     \
    set_log_priority(::petrel::log_priority::info)

#define log_base(P, M)                                                                      \
    if (P <= ::petrel::log::m_filter_priority) {                                              \
        std::clog << ::petrel::log_tag(__petrel_log_tag) << ::petrel::log::to_priority(P) << M; \
    }
#define log_default(M) log_base(__petrel_log_prio, M << std::endl)
#define log_default_noln(M) log_base(__petrel_log_prio, M)
#define log_plain_noln(M) \
    if (__petrel_log_prio <= ::petrel::log::m_filter_priority) std::clog << M
#define log_plain(M) log_plain_noln(M << std::endl)
#define log_emerg_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::emerg), M)
#define log_emerg(M) log_emerg_noln(M << std::endl)
#define log_alert_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::alert), M)
#define log_alert(M) log_alter_noln(M << std::endl)
#define log_crit_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::crit), M)
#define log_crit(M) log_crit_noln(M << std::endl)
#define log_err_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::err), M)
#define log_err(M) log_err_noln(M << std::endl)
#define log_warn_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::warn), M)
#define log_warn(M) log_warn_noln(M << std::endl)
#define log_notice_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::notice), M)
#define log_notice(M) log_notice_noln(M << std::endl)
#define log_info_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::info), M)
#define log_info(M) log_info_noln(M << std::endl)
#define log_debug_noln(M) log_base(::petrel::log::to_int(::petrel::log_priority::debug), M)
#define log_debug(M) log_debug_noln(M << std::endl)

}  // petrel

#endif  // LOG_H
