/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

namespace petrel {

class options {
  public:
    static bpo::variables_map opts;
    static bpo::options_description desc;

    static bool parse(int argc, const char** argv);

    /// Return true if an option is defined.
    static inline bool is_set(const std::string& name) { return opts.count(name); }

    /// Return an option integer value.
    ///
    /// @param name The option name
    /// @param default_val The default value to return if the option is not defined
    static inline int get_int(const std::string& name, int default_val = 0) {
        if (is_set(name)) {
            return opts[name].as<int>();
        }
        return default_val;
    }

    /// Return an option string value.
    ///
    /// @param name The option name
    /// @param default_val The default value to return if the option is not defined
    static inline std::string get_string(const std::string& name, const std::string& default_val = "") {
        if (is_set(name)) {
            return opts[name].as<std::string>();
        }
        return default_val;
    }
};

}  // petrel

#endif  // OPTIONS_H
