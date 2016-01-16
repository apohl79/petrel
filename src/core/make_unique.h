/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef MAKE_UNIQUE_H
#define MAKE_UNIQUE_H

#if __cplusplus <= 201103L

// make_unique is not yet available in C++11
namespace std {

template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

}

#endif

#endif  // MAKE_UNIQUE_H
