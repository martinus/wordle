#pragma once

#include <atomic>
#include <thread>
#include <vector>

#define ANKERL_PARALLEL_FOR_EACH_DEBUG() 0

#if ANKERL_PARALLEL_FOR_EACH_DEBUG()
#    include <iostream>
#endif

// in contrast to std::for_each(std::execution::par, ...), this creates a thread pool that sequentially takes one job after
// another. That means it is helpful to have larger jobs coming first.
//
// This strategy helps when there are lots of slow jobs and many fast jobs.
//
// Also, it is possible to stop early, when one worker returns Continue::no. All other workers will be stopped when they have
// finished their current job.

namespace ankerl::parallel {

enum class Continue : bool { no, yes };

namespace detail {

template <typename Op, typename T, typename It>
void loop(Op&& op, std::atomic<T>& atomicIdx, T size, int workerId, It begin) {
    for (auto myIdx = atomicIdx++; myIdx < size; myIdx = atomicIdx++) {
#if ANKERL_PARALLEL_FOR_EACH_DEBUG()
        std::cout << workerId << ": " << myIdx << "/" << size << std::endl;
#else
        (void)workerId;
#endif

        if constexpr (std::is_same_v<void, std::invoke_result_t<Op, decltype(*begin)>>) {
            op(*(begin + myIdx));
        } else {
            Continue c = op(*(begin + myIdx));
            if (Continue::no == c) {
                atomicIdx = size;
            }
        }
    }
#if ANKERL_PARALLEL_FOR_EACH_DEBUG()
    std::cout << workerId << " done!" << std::endl;
#endif
}

} // namespace detail

// loops until all is done, or until Op returns false.
template <typename It, typename Op>
void for_each(It begin, It end, Op&& op, int numThreads) {
    auto size = std::distance(begin, end);
    auto numWorkers = std::min<int>(numThreads, size);
    auto workers = std::vector<std::thread>();
    workers.reserve(numThreads);

    auto atomicIdx = std::atomic<decltype(size)>(0);
    for (auto workerId = int(0); workerId < numWorkers - 1; ++workerId) {
        workers.emplace_back([&atomicIdx, &begin, size, &op, workerId]() {
            detail::loop(op, atomicIdx, size, workerId + 1, begin);
        });
    }

    // this thread should work too!
    detail::loop(op, atomicIdx, size, 0, begin);

    for (auto& worker : workers) {
        worker.join();
    }
}

template <typename It, typename Op>
void for_each(It it, It end, Op&& op) {
    for_each(it, end, std::forward<Op>(op), std::thread::hardware_concurrency());
}

} // namespace ankerl::parallel
