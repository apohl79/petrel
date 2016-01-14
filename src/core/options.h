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
    static bool parse(int argc, char** argv);
};

}  // petrel

#endif  // OPTIONS_H
