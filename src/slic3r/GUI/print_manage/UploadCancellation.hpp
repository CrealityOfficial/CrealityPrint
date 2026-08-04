#ifndef REMOTE_PRINT_UPLOAD_CANCELLATION_HPP
#define REMOTE_PRINT_UPLOAD_CANCELLATION_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

namespace RemotePrint {

enum class UploadResourcePhase : unsigned char
{
    Preparing,
    FileInUse,
    FileReleased,
};

struct UploadCancellationState
{
    std::atomic_bool cancelled {false};
    std::atomic<UploadResourcePhase> resource_phase {UploadResourcePhase::Preparing};
};

using UploadCancelToken = std::shared_ptr<UploadCancellationState>;

inline UploadCancelToken make_upload_cancel_token()
{
    return std::make_shared<UploadCancellationState>();
}

inline bool is_upload_cancelled(const UploadCancelToken& token)
{
    return token && token->cancelled.load(std::memory_order_acquire);
}

inline void request_upload_cancel(const UploadCancelToken& token)
{
    if (token)
        token->cancelled.store(true, std::memory_order_release);
}

inline void set_upload_cancelled(const UploadCancelToken& token, bool cancelled)
{
    if (token)
        token->cancelled.store(cancelled, std::memory_order_release);
}

inline bool is_upload_file_in_use(const UploadCancelToken& token)
{
    return token &&
           token->resource_phase.load(std::memory_order_acquire) == UploadResourcePhase::FileInUse;
}

inline bool begin_upload_file_use(const UploadCancelToken& token)
{
    if (!token || is_upload_cancelled(token))
        return false;

    token->resource_phase.store(UploadResourcePhase::FileInUse, std::memory_order_release);
    if (is_upload_cancelled(token)) {
        token->resource_phase.store(UploadResourcePhase::FileReleased, std::memory_order_release);
        return false;
    }
    return true;
}

inline void end_upload_file_use(const UploadCancelToken& token)
{
    if (token)
        token->resource_phase.store(UploadResourcePhase::FileReleased, std::memory_order_release);
}

class UploadFileUseGuard
{
public:
    explicit UploadFileUseGuard(UploadCancelToken token)
        : m_token(std::move(token))
        , m_acquired(begin_upload_file_use(m_token))
    {
    }

    ~UploadFileUseGuard()
    {
        if (m_acquired)
            end_upload_file_use(m_token);
    }

    explicit operator bool() const { return m_acquired; }

    UploadFileUseGuard(const UploadFileUseGuard&) = delete;
    UploadFileUseGuard& operator=(const UploadFileUseGuard&) = delete;

private:
    UploadCancelToken m_token;
    bool m_acquired {false};
};

class UploadRequestCancelWatcher
{
public:
    UploadRequestCancelWatcher(UploadCancelToken token, std::function<void()> cancel_request)
        : m_token(std::move(token))
        , m_cancel_request(std::move(cancel_request))
        , m_thread([this] {
            while (!m_stopped.load(std::memory_order_acquire)) {
                if (is_upload_cancelled(m_token)) {
                    if (m_cancel_request)
                        m_cancel_request();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        })
    {
    }

    ~UploadRequestCancelWatcher()
    {
        m_stopped.store(true, std::memory_order_release);
        if (m_thread.joinable())
            m_thread.join();
    }

    UploadRequestCancelWatcher(const UploadRequestCancelWatcher&) = delete;
    UploadRequestCancelWatcher& operator=(const UploadRequestCancelWatcher&) = delete;

private:
    UploadCancelToken m_token;
    std::function<void()> m_cancel_request;
    std::atomic_bool m_stopped {false};
    std::thread m_thread;
};

} // namespace RemotePrint

#endif
