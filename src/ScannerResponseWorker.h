#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct ScannerWorkerResponse
{
    int type = 0;
    std::string payload;
};

/** One-worker request executor. Shutdown rejects new work and joins before state destruction. */
class ScannerResponseWorker
{
  public:
    using Work = std::function<ScannerWorkerResponse()>;

    ScannerResponseWorker();
    ~ScannerResponseWorker();
    bool submit(Work);
    std::vector<ScannerWorkerResponse> drainResponses();
    void shutdown();

  private:
    void run();
    std::mutex mutex;
    std::condition_variable condition;
    std::deque<Work> pending;
    std::vector<ScannerWorkerResponse> responses;
    bool stopping = false;
    std::thread thread;
};
