#pragma once
#define THREADPOOL_H


#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool {
    public:
        // A constructor that creates a thread pool with given thread count
        ThreadPool(size_t threadCount = std::thread::hardware_concurrency()) {
            if (threadCount == 0) {
                threadCount = 4;
            }

            // creating  worker threads
            for (size_t i = 0; i < threadCount; ++i) {
                threads.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;

                        // the code belows purpose is to unlock the queue before execution thus allowing other threads to perform enqueue tasks
                        {
                            // locks the queue so that data can be shared
                            std::unique_lock<std::mutex> lock (queueMutex);

                            // wait until there's a task to execute or the pool is stopped
                            signal.wait(lock, [this] {
                                return !jobQueue.empty() || signalShutdown;
                            });

                            // exit thread in case the pool stopped and there's no task
                            if (signalShutdown && jobQueue.empty()) {
                                return;
                            }

                            task = move(jobQueue.front());
                            jobQueue.pop();
                        }

                        task();
                    }
                });
            }
        }

        // Destructor
        ~ThreadPool() {
            {
                // lock queue to update signal shutdown
                std::unique_lock<std::mutex> lock(queueMutex);
                signalShutdown = true;
            }

            // notify all threads
            signal.notify_all();

            // join all worker threads to ensure the complete their tasks
            for (auto& thread : threads) {
                thread.join();
            }
        }

        // enqueue tasks for execution
        void enqueue(std::function<void()> task) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                jobQueue.emplace(std::move(task));
            }

            signal.notify_one();
        }

    private: 
        // vector storing worker threads
        std::vector<std::thread> threads;

        // queue of tasks/jobs
        std::queue<std::function<void()>> jobQueue;

        // mutex to synchroniz access to shared data
        std::mutex queueMutex;

        // condition variable to signal changes in state of tasks queue
        std::condition_variable signal;

        // flags to indicate whether the thread pool should stop
        bool signalShutdown = false;
};