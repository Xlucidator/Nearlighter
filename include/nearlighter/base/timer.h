#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>
#include <iostream>

class Timer {
public:
    Timer(const std::string& name = "Timer"): name(name) { start(); }

    void start() {
        start_time = std::chrono::steady_clock::now();
        running = true;
    }

    void stop() {
        end_time = std::chrono::steady_clock::now();
        running = false;
    }

    void reply(bool need_stop = true) {
        float duration = elapsed<std::chrono::milliseconds>();
        if (need_stop) stop();
        std::clog << name << " elapsed time: " << duration << "ms" << std::endl;
    }

private:
    std::string name;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    bool running;

    template <typename T>
    float elapsed() const {
        auto end = running ? std::chrono::steady_clock::now() : end_time;
        return std::chrono::duration_cast<T>(end - start_time).count();
    }
};

#endif // TIMER_H 