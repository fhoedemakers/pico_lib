/*
 * author : Shuichi TAKANO
 * since  : Fri Jun 25 2021 04:00:28
 */
#ifndef _25D39DD1_F134_63AD_3D0E_530B55A72531
#define _25D39DD1_F134_63AD_3D0E_530B55A72531

#include "spinlock.h"
#include <hardware/sync.h>
#include <vector>
#include <mutex>
#include <assert.h>
#include <pico.h>

namespace util
{
#if 1
    template <class T>
    class Queue
    {
        // Lock protects head_, tail_, count_, and storage_ writes.
        SpinLock spinlock_;
        std::vector<T> storage_; // fixed capacity after ctor
        size_t head_ = 0;  // index of next element to pop
        size_t tail_ = 0;  // index of next slot to push
        size_t count_ = 0; // number of valid elements

    public:
        Queue(size_t n = 0)
        {
            storage_.resize(n); // actual size used as capacity storage
        }

        size_t capacity() const { return storage_.size(); }

        __attribute__((always_inline)) size_t size()
        {
            std::lock_guard lock(spinlock_);
            return count_;
        }

        __attribute__((always_inline)) bool empty()
        {
            std::lock_guard lock(spinlock_);
            return count_ == 0;
        }

        __attribute__((always_inline)) const T &peek()
        {
            std::lock_guard lock(spinlock_);
            assert(count_ > 0);
            return storage_[head_];
        }

        // Returns false if full
        __attribute__((always_inline)) bool enque(T &&v)
        {
            bool ok = false;
            {
                std::lock_guard lock(spinlock_);
                if (count_ < storage_.size())
                {
                    storage_[tail_] = std::move(v);
                    tail_ = (tail_ + 1) % storage_.size();
                    ++count_;
                    ok = true;
                }
            }
            if (ok) __sev();
            return ok;
        }

        // Non-blocking attempt. Returns true and assigns out if element present.
        __attribute__((always_inline)) bool tryDeque(T &out)
        {
            std::lock_guard lock(spinlock_);
            if (!count_) return false;
            out = std::move(storage_[head_]);
            head_ = (head_ + 1) % storage_.size();
            --count_;
            return true;
        }

        // Blocking dequeue; optional spin limit (0 = infinite) to detect stalls.
        __attribute__((always_inline)) T deque(uint32_t spinLimit = 0)
        {
            uint32_t spins = 0;
            while (true)
            {
                {
                    std::lock_guard lock(spinlock_);
                    if (count_)
                    {
                        T r = std::move(storage_[head_]);
                        head_ = (head_ + 1) % storage_.size();
                        --count_;
                        return r;
                    }
                }
                if (spinLimit && ++spins > spinLimit)
                {
                    // Diagnostic panic to avoid endless loop.
                    panic("Queue::deque timeout (capacity=%u)\n", (unsigned)storage_.size());
                }
                __wfe();
            }
        }

        void waitUntilContentAvailable(uint32_t spinLimit = 0)
        {
            uint32_t spins = 0;
            while (true)
            {
                {
                    std::lock_guard lock(spinlock_);
                    if (count_) return;
                }
                if (spinLimit && ++spins > spinLimit)
                {
                    panic("Queue::wait timeout (capacity=%u)\n", (unsigned)storage_.size());
                }
                __wfe();
            }
        }
    };
#else
     template <class T>
    class Queue
    {
        SpinLock spinlock_;
        std::vector<T> queue_;

    public:
        Queue(size_t n = 0)
        {
            queue_.reserve(n);
        }

        __attribute__((always_inline)) size_t size()
        {
            std::lock_guard lock(spinlock_);
            return queue_.size();
        }

        __attribute__((always_inline)) const T &peek() const
        {
            return queue_.front();
        }

        __attribute__((always_inline)) void enque(T &&v)
        {
            {
                std::lock_guard lock(spinlock_);
                assert(queue_.size() < queue_.capacity());
                queue_.push_back(std::move(v));
            }
            __sev();
        }

        __attribute__((always_inline)) T deque()
        {
            while (1)
            {
                {
                    std::lock_guard lock(spinlock_);
                    if (!queue_.empty())
                    {
                        auto r = std::move(queue_.front());
                        queue_.erase(queue_.begin());
                        return r;
                    }
                }
                __wfe();
            }
        }

        void waitUntilContentAvailable()
        {
            while (1)
            {
                {
                    std::lock_guard lock(spinlock_);
                    if (!queue_.empty())
                    {
                        return;
                    }
                }
                __wfe();
            }
        }
    };
#endif
}

#endif /* _25D39DD1_F134_63AD_3D0E_530B55A72531 */
