//
//  main.cpp
//  Timer
//
//  Created by BlueCocoa on 16/3/19.
//  Copyright © 2016 BlueCocoa. All rights reserved.
//

#include <stdio.h>
#include "Timer.hpp"

int print(const char * identifier) {
    static int counter = 0;
    counter++;
    printf("[%d] %s\n", counter, identifier);
    return counter;
}

bool callback(int return_value) {
    printf("print returns %d\n", return_value);
    if (return_value > 7) {
        return false;
    }
    return true;
}

int main(int argc, const char * argv[]) {
    Timer timer;
    timer.schedule(2000, true, print, "schedule");
    sleep(10);
    timer.invaildate();
    timer.schedule_callback(2000, true, callback, print, "schedule_callback");
    sleep(10);
    timer.invaildate();
    
    printf("wait_sync: %d\n", Timer::wait_sync(2000, print, "wait_sync"));
    Timer::wait_async(2000, [](int return_value) -> void {
        printf("received return value %d in lambda callback\n", return_value);
    }, print, "wait_async");
    sleep(3);
    return 0;
}
