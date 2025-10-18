/**
 * @file async_libcurl.cpp
 * @author frankt (could.net@gmail.com)
 * @brief libcurl with asio completion token
 * @version 0.1
 * @date 2023-05-13
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <chrono>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <fstream>
#include <memory>
#include "asio.hpp"

using std::ostream;
using std::unordered_map;
using std::cout;
using std::string;
using std::ofstream;
using namespace std::placeholders;

class Session;
struct SocketItem;

// multi info, singleton
class MultiInfo {
    friend class Session;
    friend SocketItem;

    CURLM* multi_;
    asio::io_context ioc_;
    asio::steady_timer timer_;
    unordered_map<uint32_t, std::shared_ptr<SocketItem>> socket_map_;
    unordered_map<uint32_t, std::shared_ptr<Session>> session_map_;
public:
    static MultiInfo* Instance()
    {
        static MultiInfo* p = new MultiInfo;
        return p;
    }

    void Run()
    {
        ioc_.run();
    }

    asio::io_context& IoContext() { return ioc_; }
private:
    // libcurl的回调函数
    static int socket_callback(CURL* easy,      /* easy handle */
                               curl_socket_t s, /* socket */
                               int what,        /* what to wait for */
                               void* userp,     /* private callback pointer */
                               void* socketp);  /* private socket pointer */

    static int timer_callback(CURLM* multi,    /* multi handle */
                              long timeout_ms, /* see above */
                              void* userp)     /* private callback pointer */
    {
        cout << "-------->timeout=" << timeout_ms << "\n";

        asio::steady_timer& timer = MultiInfo::Instance()->timer_;

        timer.cancel();
        if (timeout_ms > 0) {
            // update timer
            timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
            timer.async_wait(std::bind(&asio_timer_callback, _1));
        }
        else if (timeout_ms == 0) {
            // call timeout function immediately
            //asio::error_code error;
            //asio_timer_callback(error);
            // 2023-05-11 新版的libcurl不允许直接在libcurl的回调函数中调用任何libucl的api，因此改成post
            MultiInfo::Instance()->IoContext().post([multi](){
                asio::error_code error;
                asio_timer_callback(error);
            });
        }

        return 0;
    }

private:
    // callback of asio
    static void asio_timer_callback(const asio::error_code error)
    {
        if (!error) {
            // libcurl读写数据
            int running_handles;
            CURLMcode mc = curl_multi_socket_action(
                MultiInfo::Instance()->multi_, CURL_SOCKET_TIMEOUT, 0,
                &running_handles);
            if (mc != CURLM_OK) {
                cout << "curl_multi_socket_action error, mc=" << mc << "\n";
            }

            check_multi_info();
        }
    }

    static void asio_socket_callback(const asio::error_code& ec,
                                     CURL* easy,      /* easy handle */
                                     curl_socket_t s, /* socket */
                                     int what,        /* describes the socket */
                                     uint32_t session_id,
                                     uint32_t socket_id);

private:
    static void check_multi_info();

private:
    MultiInfo() : ioc_(1), timer_(ioc_)
    {
        multi_ = curl_multi_init();
        curl_multi_setopt(multi_, CURLMOPT_SOCKETFUNCTION, MultiInfo::socket_callback);
        curl_multi_setopt(multi_, CURLMOPT_TIMERFUNCTION, MultiInfo::timer_callback);
    }
    MultiInfo(const MultiInfo&) = delete;
    MultiInfo(MultiInfo&&) = delete;
    MultiInfo& operator=(const MultiInfo&) = delete;
    MultiInfo& operator=(MultiInfo&&) = delete;
};

// one http request and response
class Session : public std::enable_shared_from_this<Session> {
    friend class MultiInfo;
    uint32_t id_;
    CURL* easy_;
    string url_;  // the request url
    string html_; // the result
    char error_[CURL_ERROR_SIZE+1];
//    asio::ip::tcp::socket socket_;
    bool finished_ = false; // finished以后才可以delete
    asio::any_completion_handler<void(asio::error_code, string)> handler_;
public:
    Session(const string url)
        : easy_(nullptr)
        , url_(url)
//        , finish_callback_(finish_cb)
//        , socket_(MultiInfo::Instance()->ioc_)
    {
        static int id = 0;
        id_ = ++id;
        if(id == 0) {
            id_ = ++id;
        }
    }

    ~Session()
    {
        if (easy_) {
            curl_multi_remove_handle(MultiInfo::Instance()->multi_, easy_);
            curl_easy_cleanup(easy_);
        }
    }

    friend ostream& operator<<(ostream& os, const Session& s)
    {
        os<<s.url_<<"|";
        return os;
    }

    template<asio::completion_token_for<void(asio::error_code, string)> CompletionToken>
    auto async_fetch(CompletionToken&& token)
    {
        return asio::async_initiate<CompletionToken, void(asio::error_code, string)>(
            [self=shared_from_this(), this](auto completion_handler) {
                int ret = this->Init();
                if(0 != ret) {
                    std::cout<<this->url_<<" init error: "<<ret<<"\n";
                    std::move(completion_handler)(asio::error::connection_refused, "");
                    return;
                }
                this->handler_ = std::move(completion_handler);
            },
            token
        );
    }
private:
    int Init()
    {
        easy_ = curl_easy_init();
        if (!easy_) {
            cout << "error to init easy\n";
            return -1;
        }

        curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION,
                         Session::write_callback); // 某个连接收到数据了，需要保存数据在此回调函数中保存
        curl_easy_setopt(easy_, CURLOPT_WRITEDATA, (void*)(this->id_));
        //curl_easy_setopt(easy_, CURLOPT_VERBOSE, 1L); // curl输出连接中的更多信息
        curl_easy_setopt(easy_, CURLOPT_ERRORBUFFER, error_);
        curl_easy_setopt(easy_, CURLOPT_PRIVATE,
                         (void*)(this->id_)); // 存储一个指针，通过curl_easy_getinfo函数的CURLINFO_PRIVATE参数来取
        //curl_easy_setopt(easy_, CURLOPT_NOPROGRESS, 1L);
        //curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 3L);
        //curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);
        MultiInfo::Instance()->session_map_.insert(std::make_pair(id_, shared_from_this()));
        CURLMcode mc = curl_multi_add_handle(MultiInfo::Instance()->multi_, easy_);
        return mc;
    }

    // 收到数据了，在这个函数里面保存数据
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        size_t written = size * nmemb;
        string str(static_cast<const char*>(ptr), written);
        uint32_t id = (uint32_t)(size_t)(userdata);
        auto it = MultiInfo::Instance()->session_map_.find(id);
        assert(it != MultiInfo::Instance()->session_map_.end());
        auto s = it->second;
        s->html_.append(str);
        return written;
    }
};

struct SocketItem : std::enable_shared_from_this<SocketItem> {
    SocketItem(curl_socket_t fd) : socket(MultiInfo::Instance()->ioc_),
        sockfd(fd),
        events(0) 
    {
        static uint32_t id_num = 0;
        id = ++id_num;
        if(id == 0) {
            id = ++id_num;
        }
        socket.assign(asio::ip::tcp::v4(), fd);
    }

    ~SocketItem() {
    }

    asio::ip::tcp::socket socket;
    curl_socket_t sockfd;
    int events;
    uint32_t id;
};

// 当libcurl中的某个socket需要监听的事件发生改变的时候，都会调用此函数；
// 比如说从没有变成可写、从可写变成可读等变化的时候都会调用此函数；
// 此函数的what传的是当前socket需要关注的全部事件，包括以前曾经通知过并且现在还需要的。
// 注意，因为asio每次wait都是一次性的，所以需要把以前的事件保存起来，每次wait之后都要再次调用wait。
int MultiInfo::socket_callback(CURL* easy,      /* easy handle */
                               curl_socket_t s, /* socket */
                               int what,        /* what to wait for */
                               void* userp,     /* private callback pointer */
                               void* socketp)
{
    void* session_ptr = nullptr;
    if(CURLE_OK != curl_easy_getinfo(easy, CURLINFO_PRIVATE, &session_ptr)) {
        std::cout<<"get private info error\n";
        return 0;
    }

    //Session* session = (Session*)session_ptr;
    uint32_t session_id = (uint32_t)(size_t)(session_ptr);
    cout<<"socket_callback with session_id="<<session_id<<"\n";
    auto it = MultiInfo::Instance()->session_map_.find(session_id);
    assert(it != MultiInfo::Instance()->session_map_.end());
    auto session = it->second;
    //cout<<*session<<"socket callback, easy="<<(void*)easy<<", easy in session="<<session->easy_<<"\n";
    assert(session->easy_ == easy);

    cout << *session << "========>socket_callback, s=" << s << ", what=" << what << "\n";

    switch(what) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
        {
            //SocketItem* item = socketp ? MultiInfo::Instance()->socket_map_.find((uint32_t)(size_t)socketp)->second : std::make_shared<SocketItem>(s);
            auto socket_item = [socketp, s](){
                if(socketp) {
                    uint32_t socket_id = (uint32_t)(size_t)(socketp);
                    return MultiInfo::Instance()->socket_map_.find(socket_id)->second;
                } else {
                    auto item = std::make_shared<SocketItem>(s);
                    MultiInfo::Instance()->socket_map_.insert(std::make_pair(item->id, item));
                    return item;
                }
            }();
            socket_item->events = what;
            assert(socket_item->sockfd == s);
            curl_multi_assign(MultiInfo::Instance()->multi_, s, (void*)(socket_item->id));

            if (what & CURL_POLL_IN) {
                socket_item->socket.async_wait(asio::ip::tcp::socket::wait_read,
                    [easy, s, session_id, socket_id=socket_item->id](asio::error_code ec){ MultiInfo::asio_socket_callback(ec, easy, s, CURL_POLL_IN, session_id, socket_id); });
            }

            if (what & CURL_POLL_OUT) {
                socket_item->socket.async_wait(asio::ip::tcp::socket::wait_write,
                    [easy, s, session_id, socket_id=socket_item->id](asio::error_code ec){ MultiInfo::asio_socket_callback(ec, easy, s, CURL_POLL_OUT, session_id, socket_id); });
            }
        }
        break;
        case CURL_POLL_REMOVE:
        {
            if(socketp) {
                uint32_t socket_id = (uint32_t)(size_t)(socketp);
                auto item = MultiInfo::Instance()->socket_map_.find(socket_id)->second;
                item->socket.close();
                MultiInfo::Instance()->socket_map_.erase(item->id);
                curl_multi_assign(MultiInfo::Instance()->multi_, s, NULL);
            }
        }
        break;
        default:
        abort();
    }

    return 0;
}

inline void MultiInfo::asio_socket_callback(const asio::error_code& ec,
                                            CURL* easy,      /* easy handle */
                                            curl_socket_t s, /* socket */
                                            int what, /* describes the socket */
                                            uint32_t session_id,
                                            uint32_t socket_id)
{
    if(MultiInfo::Instance()->socket_map_.find(socket_id) == MultiInfo::Instance()->socket_map_.end()) {
        return;
    }

    if(ec) { // 因为这个回调函数之前放进asio的队列中的，有可能此时对应的item已经被释放掉了，对应的socket也关掉了，关掉socket会传一个ec
        return;
    }

    void* session_ptr = nullptr;
    if(CURLE_OK != curl_easy_getinfo(easy, CURLINFO_PRIVATE, &session_ptr)) {
        std::cout<<"get private info error\n";
    }
    uint32_t sid = (uint32_t)(size_t)(session_ptr);
    assert(sid == session_id);
    auto socket_it = MultiInfo::Instance()->socket_map_.find(socket_id);
    auto socket_item = socket_it->second;
    assert(socket_item->sockfd == s);

    MultiInfo* multi = MultiInfo::Instance();

    auto it = multi->session_map_.find(session_id);
    if(it == multi->session_map_.end()) { // 已经被释放了
        return;
    }
    auto session = it->second;

    assert(session->easy_ == easy);
    cout << *session << "........>asio_socket_callback, ec=" << ec.value() << ", s=" << s
         << ", what=" << what << "\n";

    int running = 0;
    CURLMcode rc = curl_multi_socket_action(multi->multi_, s, what, &running);
    if (rc != CURLM_OK) {
        cout << *session << "curl_multi_socket_action error, rc=" << int(rc) << "\n";
    }

    check_multi_info();

    if(session->finished_) {
        multi->session_map_.erase(it); // 从map中删除
        return;
    }

    // 继续监听相关的事件，因为asio的wait函数都是一次性的，而libcurl对同一种事件没有发生变化时不会再次通知。
    // 最新需要关注的事件已经保存在newest_event_里面了，这里只要根据newest_event_的值进行添加就可以了
    if(MultiInfo::Instance()->socket_map_.find(socket_id) != MultiInfo::Instance()->socket_map_.end()) {
        if (what == CURL_POLL_IN && (socket_item->events & CURL_POLL_IN)) {
            socket_item->socket.async_wait(asio::ip::tcp::socket::wait_read,
                                            std::bind(MultiInfo::asio_socket_callback, _1,
                                                    easy, s, CURL_POLL_IN, session_id, socket_id));
        }

        if (what == CURL_POLL_OUT && (socket_item->events & CURL_POLL_OUT)) {
            socket_item->socket.async_wait(asio::ip::tcp::socket::wait_write,
                                            std::bind(MultiInfo::asio_socket_callback, _1,
                                                    easy, s, CURL_POLL_OUT, session_id, socket_id));
        }
    }
}

void MultiInfo::check_multi_info()
{
    int msgs_left;
    for (CURLMsg* msg = curl_multi_info_read(MultiInfo::Instance()->multi_, &msgs_left);
         msg != nullptr;
         msg = curl_multi_info_read(MultiInfo::Instance()->multi_, &msgs_left)) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            void* session_ptr = nullptr;
            if(CURLE_OK != curl_easy_getinfo(easy, CURLINFO_PRIVATE, &session_ptr)) {
                std::cout<<"get private info error\n";
            }
            uint32_t session_id = (uint32_t)(size_t)(session_ptr);
            auto it = MultiInfo::Instance()->session_map_.find(session_id);
            if(it == MultiInfo::Instance()->session_map_.end()) { // 已经被释放了
                return;
            }
            auto session = it->second;
            session->handler_(asio::error_code{}, session->html_);
            session->finished_ = true;
        }
    }
}

asio::awaitable<void> FetchHtml(string url)
{
    auto session = std::make_shared<Session>(url);
    try {
        string html = co_await session->async_fetch(asio::use_awaitable);
        std::cout<<url<<"| finished $$$$$$$$$$$$$$$$$$\n";
    } catch (asio::error_code ec) {
        cout<<*session<<" get error: "<<ec.message()<<"\n";
    }
}

int main()
{
    string urls[] =
    { "https://curl.se/libcurl/c/multi-uv.html",
      "https://curl.se/libcurl/c/multi-event.html",
      "https://en.cppreference.com/w/cpp/container/vector",
      "https://www.boost.org/",
      "https://www.codeproject.com/Tags/Cplusplus",
      "https://isocpp.org",
    };

    for (const string& url : urls)
    {
        asio::co_spawn(MultiInfo::Instance()->IoContext(), FetchHtml(url), asio::detached);
    }

    MultiInfo::Instance()->Run();
    return 0;
}

