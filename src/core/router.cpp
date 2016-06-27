/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "router.h"

namespace petrel {

router::router() {
    // 404 default
    m_default_func = [](request::pointer req) { req->send_error_response(404); };
}

}  // petrel
