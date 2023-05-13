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
    unordered_map<uint32_t, SocketItem*> socket_map_;

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
                                     Session* session,
                                     int id,
                                     SocketItem* item);

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

using FinishHttp = void (*)(Session*, const string& url, string&& html);

// one http request and response
class Session {
    friend class MultiInfo;
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
            [this](auto completion_handler) {
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
        curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);
        //curl_easy_setopt(easy_, CURLOPT_VERBOSE, 1L); // curl输出连接中的更多信息
        curl_easy_setopt(easy_, CURLOPT_ERRORBUFFER, error_);
        curl_easy_setopt(easy_, CURLOPT_PRIVATE,
                         this); // 存储一个指针，通过curl_easy_getinfo函数的CURLINFO_PRIVATE参数来取
        //curl_easy_setopt(easy_, CURLOPT_NOPROGRESS, 1L);
        //curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 3L);
        //curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);
        CURLMcode mc = curl_multi_add_handle(MultiInfo::Instance()->multi_, easy_);
        return mc;
    }

    // 收到数据了，在这个函数里面保存数据
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata)
    {
        size_t written = size * nmemb;
        string str(static_cast<const char*>(ptr), written);
        Session* s = static_cast<Session*>(userdata);
        s->html_.append(str);
        return written;
    }
};

struct SocketItem {
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
        MultiInfo::Instance()->socket_map_.insert(std::make_pair(id, this));
    }

    ~SocketItem() {
        MultiInfo::Instance()->socket_map_.erase(id);
    }

    asio::ip::tcp::socket socket;
    curl_socket_t sockfd;
    int events;
    int id;
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

    Session* session = (Session*)session_ptr;
    assert(session);
    //cout<<*session<<"socket callback, easy="<<(void*)easy<<", easy in session="<<session->easy_<<"\n";
    assert(session->easy_ == easy);

    cout << *session << "========>socket_callback, s=" << s << ", what=" << what << "\n";

    switch(what) {
        case CURL_POLL_IN:
        case CURL_POLL_OUT:
        case CURL_POLL_INOUT:
        {
            SocketItem* item = socketp ? (SocketItem*)socketp : new SocketItem(s);
            item->events = what;
            assert(item->sockfd == s);
            curl_multi_assign(MultiInfo::Instance()->multi_, s, (void*)item);

            if (what & CURL_POLL_IN) {
                item->socket.async_wait(asio::ip::tcp::socket::wait_read,
                    [easy, s, session, item](asio::error_code ec){ MultiInfo::asio_socket_callback(ec, easy, s, CURL_POLL_IN, session, item->id, item); });
            }

            if (what & CURL_POLL_OUT) {
                item->socket.async_wait(asio::ip::tcp::socket::wait_write,
                    [easy, s, session, item](asio::error_code ec){ MultiInfo::asio_socket_callback(ec, easy, s, CURL_POLL_OUT, session, item->id, item); });
            }
        }
        break;
        case CURL_POLL_REMOVE:
        {
            if(socketp) {
                SocketItem* item = (SocketItem*)socketp;
                item->socket.close();
                delete item;
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
                                            Session* session,
                                            int id,
                                            SocketItem* item)
{
    if(MultiInfo::Instance()->socket_map_.find(id) == MultiInfo::Instance()->socket_map_.end()) {
        return;
    }

    assert(session->easy_ == easy);
    cout << *session << "........>asio_socket_callback, ec=" << ec.value() << ", s=" << s
         << ", what=" << what << "\n";

    if(ec) { // 因为这个回调函数之前放进asio的队列中的，有可能此时对应的item已经被释放掉了，对应的socket也关掉了，关掉socket会传一个ec
        return;
    }

    void* session_ptr = nullptr;
    if(CURLE_OK != curl_easy_getinfo(easy, CURLINFO_PRIVATE, &session_ptr)) {
        std::cout<<"get private info error\n";
    }
    assert(session_ptr == (void*)session);
    assert(item->sockfd == s);

    MultiInfo* multi = MultiInfo::Instance();
    int running = 0;
    CURLMcode rc = curl_multi_socket_action(multi->multi_, s, what, &running);
    if (rc != CURLM_OK) {
        cout << *session << "curl_multi_socket_action error, rc=" << int(rc) << "\n";
    }

    check_multi_info();

    if(session->finished_) {
        delete session;
        return;
    }

    // 继续监听相关的事件，因为asio的wait函数都是一次性的，而libcurl对同一种事件没有发生变化时不会再次通知。
    // 最新需要关注的事件已经保存在newest_event_里面了，这里只要根据newest_event_的值进行添加就可以了
    if(MultiInfo::Instance()->socket_map_.find(id) != MultiInfo::Instance()->socket_map_.end()) {
        if (what == CURL_POLL_IN && (item->events & CURL_POLL_IN)) {
            item->socket.async_wait(asio::ip::tcp::socket::wait_read,
                                            std::bind(MultiInfo::asio_socket_callback, _1,
                                                    easy, s, CURL_POLL_IN, session, id, item));
        }

        if (what == CURL_POLL_OUT && (item->events & CURL_POLL_OUT)) {
            item->socket.async_wait(asio::ip::tcp::socket::wait_write,
                                            std::bind(MultiInfo::asio_socket_callback, _1,
                                                    easy, s, CURL_POLL_OUT, session, id, item));
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
            Session* s;
            if(CURLE_OK != curl_easy_getinfo(easy, CURLINFO_PRIVATE, &s)) {
                cout<<*s<< "curl_easy_getinfo error\n";
            }
            //s->finish_callback_(s, s->url_, std::move(s->html_));
            s->handler_(asio::error_code{}, s->html_);
            s->finished_ = true;
        }
    }
}

asio::awaitable<void> FetchHtml(string url)
{
    Session* session = new Session(url);
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

