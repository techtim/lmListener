#pragma once

#include <atomic>
#include <chrono>
#include <thread>
#include <typeinfo>

#include "easylogging++.h"

namespace LedMapper {

using namespace std::chrono;

class ThreadedClass {
public:
    ThreadedClass()
        : m_isRunning(false)
        , m_nanosecInFrame(s_secToNs / 100)
    {
    }

    ThreadedClass(const ThreadedClass &) = delete;
    ThreadedClass &operator=(const ThreadedClass &) = delete;
    ThreadedClass(ThreadedClass &&) = delete;
    ThreadedClass &operator=(ThreadedClass &&) = delete;

    virtual ~ThreadedClass() { stopThreading(); }

    void startThreading(size_t fps)
    {
        stopThreading();
        m_nanosecInFrame = milliseconds(s_secToMs / fps);
        m_isRunning = true;
        m_thread = std::thread([this]() {
            time_point<steady_clock> start;
            nanoseconds elapsed{ 0 };

            while (m_isRunning) {
                start = steady_clock::now();
                threadedUpdate();
                elapsed = duration_cast<nanoseconds>(steady_clock::now() - start);
                if (elapsed <= m_nanosecInFrame.load())
                    std::this_thread::sleep_for(m_nanosecInFrame.load() - elapsed);
#ifndef NDEBUG
                else
                    LOG(ERROR) << typeid(*this).name() << " threadedUpdate() took too much time: "
                               << duration_cast<milliseconds>(elapsed).count() << " ms";
#endif
            }
        });
    }

    void stopThreading() noexcept
    {
        m_isRunning = false;
        if (m_thread.joinable())
            m_thread.join();
    }

    bool isRunning() const noexcept { return m_isRunning.load(); }

private:
    virtual void threadedUpdate() = 0;

    std::thread m_thread;
    std::atomic<bool> m_isRunning;
    std::atomic<nanoseconds> m_nanosecInFrame;
};

} // namespace LedMapper
