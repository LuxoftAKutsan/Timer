//
//  Timer.hpp
//  Timer
//
//  Copyright (c) 2016 BlueCocoa
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//

#ifndef TIMER_HPP
#define TIMER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <type_traits>

class Timer {
public:
    /**
     *  @brief Constructor
     */
    Timer() : invaild(false), try_to_invaild(false) {
        
    };
    
    /**
     *  @brief Deconstructor
     */
    ~Timer() {
        invaildate();
    }
    
    /**
     *  @brief Invaildate current timer
     *
     *  @return *this
     */
    Timer& invaildate() {
        if (!invaild) {
            if (!try_to_invaild) {
                try_to_invaild = true;
                {
                    std::unique_lock<std::mutex> lock(mtx);
                    invaild_cond.wait(lock, [this]{
                        return invaild == true;
                    });
                    if (invaild == true) {
                        try_to_invaild = false;
                    }
                    invaild = false;
                }
            }
        }
        return *this;
    };
    
    /**
     *  @brief Schedule a timer
     *
     *  @param time_unit Which time unit does the interval value use
     *  @param Function  Derived automatically
     *  @param Args      Derived automatically
     *  @param interval  Every interval time units to call the function once
     *  @param repeat    Enable repeat
     *  @param func      Which function to be scheduled and called periodically
     *  @param args      Arguements that bind to the function
     *
     *  @return *this
     */
    template <class time_unit = std::chrono::milliseconds, class Function, class ... Args>
    Timer& schedule(uint64_t interval, bool repeat, Function&& func, Args&&... args) {
        if (!invaild) {
            typedef typename std::result_of<Function(Args...)>::type return_type;
            typedef std::packaged_task<return_type()> task;
            std::thread([=](){
                do {
                    auto t = std::make_shared<task>(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
                    std::this_thread::sleep_for(time_unit(interval));
                    auto ret = t->get_future();
                    {
                        (*t)();
                    }
                } while (!try_to_invaild && repeat); // do-while
                {
                    std::lock_guard<std::mutex> locker(mtx);
                    invaild = true;
                    invaild_cond.notify_one();
                }
            }).detach();
        }
        return *this;
    };
    
    /**
     *  @brief Schedule a timer with return value callback
     *
     *  @param time_unit Which time unit does the interval value use
     *  @param Function  Derived automatically
     *  @param Args      Derived automatically
     *  @param interval  Every interval time units to call the function once
     *  @param repeat    Enable repeat
     *  @param callback  The callback function
     *  @param func      Which function to be scheduled and called periodically
     *  @param args      Arguements that bind to the function
     *
     *  @return *this
     */
    template <class time_unit = std::chrono::milliseconds, class Function, class ... Args>
    Timer& schedule_callback(uint64_t interval, bool repeat, std::function<bool(typename std::result_of<Function(Args...)>::type)> callback, Function&& func, Args&&... args) {
        if (!invaild) {
            typedef typename std::result_of<Function(Args...)>::type return_type;
            typedef std::packaged_task<return_type()> task;
            std::thread([=](){
                bool keep_running = true;
                do {
                    auto t = std::make_shared<task>(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
                    std::this_thread::sleep_for(time_unit(interval));
                    auto ret = t->get_future();
                    {
                        (*t)();
                    }
                    keep_running = callback(ret.get());
                } while (!try_to_invaild  && keep_running && repeat); // do-while
                {
                    std::lock_guard<std::mutex> locker(mtx);
                    invaild = true;
                    invaild_cond.notify_one();
                }
            }).detach();
        }
        return *this;
    };
    
    /**
     *  @brief Synchronously waits for some time then call a function
     *
     *  @param time_unit Which time unit does the interval value use
     *  @param Function  Derived automatically
     *  @param Args      Derived automatically
     *  @param interval  Every interval time units to call the function once
     *  @param func      Which function to be scheduled and called
     *  @param args      Arguements that bind to the function
     *
     *  @return What the function returns
     */
    template <class time_unit = std::chrono::milliseconds, class Function, class ... Args>
    static typename std::result_of<Function(Args...)>::type wait_sync(uint64_t interval, Function&& func, Args&&... args) {
        typedef typename std::result_of<Function(Args...)>::type return_type;
        typedef std::packaged_task<return_type()> task;
        auto t = std::make_shared<task>(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
        std::this_thread::sleep_for(time_unit(interval));
        auto ret = t->get_future();
        {
            (*t)();
        }
        return ret.get();
    };
    
    /**
     *  @brief Asynchronously waits for some time then call a function
     *
     *  @param time_unit Which time unit does the interval value use
     *  @param Function  Derived automatically
     *  @param Args      Derived automatically
     *  @param interval  Every interval time units to call the function once
     *  @param callback  The callback function
     *  @param func      Which function to be scheduled and called
     *  @param args      Arguements that bind to the function
     *
     */
    template <class interval_class = std::chrono::milliseconds, class Function, class ... Args>
    static void wait_async(uint64_t interval, std::function<void(typename std::result_of<Function(Args...)>::type)> callback, Function&& func, Args&&... args) {
        typedef typename std::result_of<Function(Args...)>::type return_type;
        typedef std::packaged_task<return_type()> task;
        auto t = std::make_shared<task>(std::bind(std::forward<Function>(func), std::forward<Args>(args)...));
        std::thread([interval, callback, t](){
            std::this_thread::sleep_for(interval_class(interval));
            auto ret = t->get_future();
            {
                (*t)();
            }
            callback(ret.get());
        }).detach();
    }
private:
    std::mutex mtx;
    std::atomic_bool invaild, try_to_invaild;
    std::condition_variable invaild_cond;
};

#endif /* TIMER_HPP */
