/**
 * @file key_value_cache.h
 *   key value cache with c++20 coroutine
 * 
 * @author frankt (could.net@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <string>
#include <any>
#include <memory>
#include <deque>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <iostream>
#include <sstream>
#include <functional>

#include "asio.hpp"

template<typename Key, typename Value>
class AsyncCache
{
public:
    AsyncCache(asio::io_context& io_context) : io_context_(io_context) {}
    template<typename CompletionToken>
    auto async_get(Key key, CompletionToken&& token)
    {
        return asio::async_compose<CompletionToken, void(asio::error_code, Value)>(
            [this, &key](auto& self, asio::error_code ec, Value value) mutable -> void {
                asio::any_completion_handler<void(asio::error_code, Value)> h = std::move(self);
                asio::post(io_context_.get_executor(), [h = std::move(h)]() mutable { h(asio::error_code{}, std::string("hello")); });
            }, token
        );
    }
private:
    asio::io_context& io_context_;
};