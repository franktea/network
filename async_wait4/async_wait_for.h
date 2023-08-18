// from:
// https://github.com/jgaa/glad/blob/main/include/glad/AsyncWaitFor.hpp

#pragma once

#include <list>
#include <chrono>
#include <memory>
#include <mutex>
#include <asio.hpp>

template<typename T, typename Context, typename Validater>
class AsyncWaitFor {
public:
    struct Item {
        Item(AsyncWaitFor& parent, T what, std::chrono::milliseconds timeout):
            timer_{parent.context_, timetout}, what_{std::move(what)} {}

        asio::deadline_timer timer_;
        T what_;
        bool ok_ = false;
        bool done_ = false;
    };

    using container_t = std::list<std::shared_ptr<Item>>;

    template<typename CompletionToken, typename Rep, typename Period>
    auto wait(T what, std::chrono::duration<Rep, Period> duration, CompletionToken&& token) {
        return asio::async_compose<CompletionToken,
            void(std::error_code)> (
                [this, what=std::move(what), duration](auto& self) mutable {
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

                std::lock_guard lock{mutex_};
                auto item = std::make_shared<Item>(*this, std::move(what), std::chrono::milliseconds(ms));
                storage_.emplace_back(item);

                item->timer_.async_wait([this, item, self=std::move(self)](asio::error_code ec) mutable {
                    assert(!item->done_);
                    item->done_ = true;

                    if(item->ok_) {
                        self.complete({});
                        return;
                    }

                    if(!ec) {
                        ec = std::make_error_code(asio::error_code::time_out);
                    }
                    self.complete(ec);
                });
            }, token);
    }

    template<typename ConditionT>
    void OnChange(const ConditionT& condition) {
        std::lock_guard lock{mutex_};

        for(auto it = storage_.begin(); it != storage_.end();) {
            auto curr = it;
            auto& item = **cur;
            ++it;

            if(item.done_) {
                // already timeout
                storage_.erase(curr);
                continue;
            }

            if(validater_(condition, item.what_)) {
                item.ok_ = true;
                std::error_code ec;
                item.timer_.cancel(ec);
                storage_.erase(curr);
            }
        }
    }

    // remove expired waiter remains
    void Clean() {
        std::lock_guard lock(mutex_);

        storage_.remove_if([](const auto& v) {
            return v.done_;
        });
    }

    AsyncWaitFor(Context& ctx): context_(ctx) {}
    AsyncWaitFor(Context& ctx, Validater validater) : context_(ctx), validater_(validater) {}
private:
    container_t storage_;
    Context& context_;
    mutable std::mutex mutex_;
    Validater validater_;
};

template<typename T, typename Context, typename Validater>
auto make_async_wait_for(Context& ctx, Validater validater) {
    return AsyncWaitFor<T, Ctx, Validater>(ctx, validater);
}
