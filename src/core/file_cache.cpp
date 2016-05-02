/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "file_cache.h"
#include "asio_post.h"
#include "branch.h"
#include "make_unique.h"

#include <boost/filesystem.hpp>
#include <chrono>
#include <fstream>

using namespace boost::filesystem;

namespace petrel {

thread_local std::unique_ptr<file_cache::file_map_type> file_cache::m_file_map_local;

file_cache::file::file(const std::string& name, bool read_from_disk) {
    path p(name);
    if (exists(p)) {
        if (is_regular_file(p)) {
            m_size = file_size(p);
            m_time = last_write_time(p);
            if (read_from_disk) {
                std::ifstream is(name, std::ifstream::binary);
                if (is) {
                    m_data.reserve(m_size);
                    std::copy(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(),
                              std::back_inserter(m_data));
                    m_good = true;
                } else {
                    log_err("can't open file " << name);
                }
            }
        } else {
            log_err(name << " is no file");
        }
    } else {
        log_err(name << " does not exists");
    }
}

file_cache::file_cache(int refresh_time) {
    m_thread = std::thread([this, refresh_time] {
        while (!m_stop) {
            // relax
            std::this_thread::sleep_for(std::chrono::seconds(refresh_time));
            // look for new objects
            std::unordered_set<std::string> dirs;
            {
                std::lock_guard<std::mutex> lock(m_dir_mtx);
                dirs = m_directories;
            }
            for (auto& dir : dirs) {
                scan_directory(dir);
            }
            // update/remove existing objects
            std::vector<std::string> to_remove;
            std::vector<std::string> to_add;
            {
                std::lock_guard<std::mutex> lock(m_file_mtx);
                for (auto& pair : m_file_map) {
                    path p(pair.first);
                    if (exists(p)) {
                        if (*pair.second != file(pair.first)) {
                            // changed file
                            to_add.push_back(pair.first);
                        }
                    } else {
                        // file does not exists, remove it
                        to_remove.push_back(pair.first);
                    }
                }
            }
            for (auto& f : to_add) {
                add_file(f);
            }
            for (auto& f : to_remove) {
                remove_file(f);
            }
        }
    });
}

file_cache::~file_cache() {
    m_stop = true;
    m_thread.join();
}

void file_cache::register_io_service(ba::io_service* iosvc) {
    m_iosvcs.push_back(iosvc);
    iosvc->post([] { m_file_map_local = std::make_unique<file_map_type>(); });
}

void file_cache::unregister_io_service(ba::io_service* iosvc) {
    for (auto it = m_iosvcs.begin(); it != m_iosvcs.end(); it++) {
        if (*it == iosvc) {
            m_iosvcs.erase(it);
            break;
        }
    }
    io_service_post_wait(iosvc, [] { m_file_map_local.reset(); });
}

bool file_cache::scan_directory(const std::string& name) {
    path p(name);
    if (exists(p) && is_directory(p)) {
        for (directory_entry& entry : directory_iterator(p)) {
            if (is_directory(entry.status())) {
                scan_directory(entry.path().string());
            } else {
                std::shared_ptr<file> f = get_file_main(entry.path().string());
                if (nullptr == f) {
                    // new file
                    add_file(entry.path().string());
                }
            }
        }
        return true;
    }
    log_debug(name << " is no direcotory or does not exists");
    return false;
}

bool file_cache::add_directory(const std::string& name) {
    if (scan_directory(name)) {
        log_debug("added directory " << name);
        std::lock_guard<std::mutex> lock(m_dir_mtx);
        m_directories.insert(name);
        return true;
    }
    log_err(name << " not added");
    return false;
}

bool file_cache::add_file(const std::string& name) {
    auto f = std::make_shared<file>(name, true);
    log_debug("loaded object " << name << " (" << f->size() << " bytes)");
    if (f->good()) {
        // update the local caches
        for (auto* iosvc : m_iosvcs) {
            iosvc->post([this, name, f] { (*m_file_map_local)[name] = f; });
        }
        // update the main map
        std::lock_guard<std::mutex> lock(m_file_mtx);
        m_file_map[name] = f;
        return true;
    }
    return false;
}

void file_cache::remove_file(const std::string& name) {
    log_debug("removing object " << name);
    // update the local caches
    for (auto* iosvc : m_iosvcs) {
        iosvc->post([name] {
            auto it = m_file_map_local->find(name);
            if (m_file_map_local->end() != it) {
                m_file_map_local->erase(it);
            }
        });
    }
    // update the main map
    std::lock_guard<std::mutex> lock(m_file_mtx);
    auto it = m_file_map.find(name);
    if (m_file_map.end() != it) {
        m_file_map.erase(it);
    }
}

std::shared_ptr<file_cache::file> file_cache::get_file(const std::string& name) {
    // thread local lookup
    bool update_local_map = false;
    if (likely(nullptr != m_file_map_local)) {
        auto it = m_file_map_local->find(name);
        if (m_file_map_local->end() != it) {
            return it->second;
        }
        update_local_map = true;
    }
    // falling back to main map
    auto file = get_file_main(name);
    if (nullptr != file && update_local_map) {
        // update the local cache
        (*m_file_map_local)[name] = file;
    }
    return file;
}

std::shared_ptr<file_cache::file> file_cache::get_file_main(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_file_mtx);
    auto it = m_file_map.find(name);
    if (m_file_map.end() != it) {
        return it->second;
    }
    return nullptr;
}

}  // petrel
