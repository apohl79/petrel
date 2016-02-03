/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "request.h"

namespace petrel {

const std::string request::EMPTY;

bool operator==(request::header_iterator& lhs, request::header_iterator& rhs) {
    if (lhs.m_mode != rhs.m_mode) {
        throw std::runtime_error("invalid iterator modes");
    }
    switch (lhs.m_mode) {
        case request::mode::HTTP:
            return lhs.m_http_iter == rhs.m_http_iter;
        case request::mode::HTTP2:
            return lhs.m_http2_iter == rhs.m_http2_iter;
    }
    throw std::runtime_error("invalid mode");
}

bool operator!=(request::header_iterator& lhs, request::header_iterator& rhs) {
    if (lhs.m_mode != rhs.m_mode) {
        throw std::runtime_error("invalid iterator modes");
    }
    switch (lhs.m_mode) {
        case request::mode::HTTP:
            return lhs.m_http_iter != rhs.m_http_iter;
        case request::mode::HTTP2:
            return lhs.m_http2_iter != rhs.m_http2_iter;
    }
    throw std::runtime_error("invalid mode");
}

}  // petrel
