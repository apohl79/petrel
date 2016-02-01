/*
 * Copyright (c) 2016 Andreas Pohl
 * Licensed under MIT (see COPYING)
 *
 * Author: Andreas Pohl
 */

#include "file_cache.h"

#include <fstream>
#include <chrono>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

namespace petrel {

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
                    is.read(m_data.data(), m_size);
                    if (is) {
                        m_good = true;
                    } else {
                        log_err("can't read complete file " << name);
                    }
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
                            log_debug("reloading changed object " << pair.first);
                            to_add.push_back(pair.first);
                        }
                    } else {
                        // file does not exists, remove it
                        log_debug("removing object " << pair.first);
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

bool file_cache::scan_directory(const std::string& name) {
    path p(name);
    if (exists(p) && is_directory(p)) {
        for (directory_entry& entry : directory_iterator(p)) {
            if (is_directory(entry.status())) {
                scan_directory(entry.path().string());
            } else {
                std::shared_ptr<file> f = get_file(entry.path().string());
                if (nullptr == f) {
                    // new file
                    log_debug("loading object " << entry.path().string());
                    add_file(entry.path().string());
                }
            }
        }
        return true;
    }
    return false;
}

void file_cache::add_directory(const std::string& name) {
    if (scan_directory(name)) {
        log_debug("added directory " << name);
        std::lock_guard<std::mutex> lock(m_dir_mtx);
        m_directories.insert(name);
    } else {
        log_err(name << " not added");
    }
}

bool file_cache::add_file(const std::string& name) {
    auto f = std::make_shared<file>(name, true);
    if (f->good()) {
        std::lock_guard<std::mutex> lock(m_file_mtx);
        m_file_map[name] = f;
        return true;
    }
    return false;
}

void file_cache::remove_file(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_file_mtx);
    auto it = m_file_map.find(name);
    if (m_file_map.end() != it) {
        m_file_map.erase(it);
    }
}

std::shared_ptr<file_cache::file> file_cache::get_file(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_file_mtx);
    auto it = m_file_map.find(name);
    if (m_file_map.end() != it) {
        return it->second;
    }
    return nullptr;
}

}  // petrel
