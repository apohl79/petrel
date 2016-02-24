/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <boost/core/noncopyable.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "log.h"

namespace petrel {

/// file_cache class
/// The cache is loading and refreshing directories contents and
/// provides access to the file contents. We use polling for now to
/// keep things portable as static file delivery is not our main
/// purpose.
class file_cache : boost::noncopyable {
    set_log_tag("file_cache");

  public:
    class file {
        set_log_tag("file_cache::file");

      public:
        using buffer_type = std::vector<char>;

        file(const std::string& name, bool read_from_disk = false);

        /// Return the file data
        const buffer_type& data() const { return m_data; }

        /// Return the size of the file
        std::size_t size() const { return m_size; }

        /// Return the last time the file was written.
        std::time_t time() const { return m_time; }

        /// Return true if the file wasy loaded successfully.
        bool good() const { return m_good; }

      private:
        std::string m_name;
        buffer_type m_data;
        std::size_t m_size = 0;
        std::time_t m_time = 0;
        bool m_good = false;
    };

    file_cache(int refresh_time = 5);
    ~file_cache();

    /// Add a directory to the cache
    ///
    /// @param name The absolute path of the directory
    /// @return true on success
    bool add_directory(const std::string& name);

    /// Add a single file to the cache
    ///
    /// @param name The absolute path of the file
    /// @return true on success
    bool add_file(const std::string& name);

    /// Remove a file from the cache
    ///
    /// @param name The absolute path of the file
    void remove_file(const std::string& name);

    /// Get a file object from the cache
    ///
    /// @param name The absolute path to the file
    std::shared_ptr<file> get_file(const std::string& name);

  private:
    std::unordered_set<std::string> m_directories;
    std::mutex m_dir_mtx;
    std::unordered_map<std::string, std::shared_ptr<file>> m_file_map;
    std::mutex m_file_mtx;
    std::thread m_thread;
    bool m_stop = false;

    bool scan_directory(const std::string& name);
};

inline bool operator==(const file_cache::file& lhs, const file_cache::file& rhs) {
    return lhs.size() == rhs.size() && lhs.time() == rhs.time();
}

inline bool operator!=(const file_cache::file& lhs, const file_cache::file& rhs) {
    return lhs.size() != rhs.size() || lhs.time() != rhs.time();
}

}  // petrel

#endif  // FILE_CACHE_H
