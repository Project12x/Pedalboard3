#include "ScannerResponseWorker.h"

ScannerResponseWorker::ScannerResponseWorker() : thread([this] { run(); }) {}
ScannerResponseWorker::~ScannerResponseWorker() { shutdown(); }

bool ScannerResponseWorker::submit(Work work)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (stopping)
        return false;
    pending.push_back(std::move(work));
    condition.notify_one();
    return true;
}

std::vector<ScannerWorkerResponse> ScannerResponseWorker::drainResponses()
{
    std::lock_guard<std::mutex> lock(mutex);
    auto result = std::move(responses);
    responses.clear();
    return result;
}

void ScannerResponseWorker::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping)
            return;
        stopping = true;
        pending.clear();
    }
    condition.notify_all();
    if (thread.joinable())
        thread.join();
}

void ScannerResponseWorker::run()
{
    for (;;)
    {
        Work work;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [&] { return stopping || ! pending.empty(); });
            if (stopping && pending.empty())
                return;
            work = std::move(pending.front());
            pending.pop_front();
        }

        auto response = work();
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (! stopping)
                responses.push_back(std::move(response));
        }
    }
}
