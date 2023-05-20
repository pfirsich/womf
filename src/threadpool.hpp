#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "function.hpp"

namespace womf {
class ThreadPool {
public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());

    ~ThreadPool();

    void push(Function<void()> task);

    template <typename Func, typename Result = std::invoke_result_t<Func>>
    std::future<Result> submit(Func func)
    {
        std::promise<Result> promise;
        auto future = promise.get_future();
        push([promise = std::move(promise), func = Function<Result()>(std::move(func))]() mutable {
            if constexpr (std::is_void_v<Result>) {
                promise.set_value();
            } else {
                promise.set_value(func());
            }
        });
        return future;
    }

private:
    void workerFunc();

    std::atomic<bool> running_;
    std::vector<std::thread> threads_;
    std::condition_variable tasksCv_;
    std::mutex tasksMutex_;
    std::queue<Function<void(void)>> tasks_;
};

ThreadPool& getDefaultThreadPool();
}
