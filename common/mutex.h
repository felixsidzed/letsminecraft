#pragma once

#include <semaphore>

namespace lmc {

    /// <summary>
    /// A small std::mutex alternative
    /// </summary>
    class mutex {
        std::binary_semaphore sem{ 1 };

    public:
        mutex() = default;
        ~mutex() = default;

        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;

        /// <summary>
        /// Acquire the lock
        /// </summary>
        void lock() noexcept {
            sem.acquire();
        }

        /// <summary>
        /// Release the lock
        /// </summary>
        void unlock() noexcept {
            sem.release();
        }

        /// <summary>
        /// Try to acquire the lock (non-blocking) 
        /// </summary>
        /// <returns>Whether the lock was acquired successfully</returns>
        bool try_lock() noexcept {
            return sem.try_acquire();
        }
    };


    /// <summary>
    /// An std::lock_guard&lt;std::mutex&gt; alternative
    /// </summary>
    class lock {
        mutex& m;

    public:
        explicit lock(mutex& m) noexcept
            : m(m)
        {
            m.lock();
        }

        ~lock() {
            m.unlock();
        }

        lock(const lock&) = delete;
        lock& operator=(const lock&) = delete;
    };

}
