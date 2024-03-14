#pragma once

#include <thread>
#include <map>
#include <mutex>
#include <iostream>

namespace sali {

class ThreadInfo {
public:
    static int omp_get_thread_num() {
        static std::map<std::thread::id, int> thread_id_;
        static std::mutex thread_mtx_;

        std::lock_guard<std::mutex> guard{thread_mtx_};
        std::thread::id this_id = std::this_thread::get_id();

        if (thread_id_.find(this_id) != thread_id_.end()) {
            return thread_id_[this_id];
        }

        int sz = thread_id_.size();
        // std::cout << "this id = " << sz << std::endl;

        thread_id_[this_id] = sz;

        return sz;
    }
};

}