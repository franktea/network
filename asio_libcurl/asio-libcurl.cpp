/*
 * asio-libcurl.cpp
 *  doc: https://ec.haxx.se/libcurl-drive-multi-socket.html
 *  Created on: Nov 28, 2018
 *      Author: frank
 */

#include <functional>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <string>
#include "asio.hpp"
#include <curl/curl.h>

using namespace std;
using std::placeholders::_1;

//clang++ -std=c++17 -DASIO_STAND_ALONE asio-libcurl.cpp -lcurl -I../asio/asio/include

class Session;

// multi info, singleton
class MultiInfo
{
    friend class Session;

    CURLM* multi_;
    int still_running_;
    asio::io_context ioc_;
    asio::steady_timer timer_;
    unordered_map<curl_socket_t, Session*> socket_map_;
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
private: // libcurl的回调函数
    static int socket_callback(CURL *easy,      /* easy handle */
            curl_socket_t s, /* socket */
            int what,        /* what to wait for */
            void *userp,     /* private callback pointer */
            void *socketp);   /* private socket pointer */


    static int timer_callback(CURLM *multi,    /* multi handle */
            long timeout_ms, /* see above */
            void *userp)    /* private callback pointer */
    {
    	cout<<"-------->timeout="<<timeout_ms<<"\n";

    	asio::steady_timer& timer = MultiInfo::Instance()->timer_;

    	timer.cancel();
        if(timeout_ms > 0)
        {
            // update timer
			timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
			timer.async_wait(std::bind(&asio_timer_callback, _1));
        }
        else if(timeout_ms == 0)
        {
            // call timeout function immediately
            asio::error_code error;
            asio_timer_callback(error);
        }

        return 0;
    }
private: // callback of asio
    static void asio_timer_callback(const asio::error_code & error)
    {
        if (!error)
        {
            // libcurl读写数据
            CURLMcode mc = curl_multi_socket_action(MultiInfo::Instance()->multi_, CURL_SOCKET_TIMEOUT, 0,
                    &(MultiInfo::Instance()->still_running_));
            	if(mc != CURLM_OK)
            	{
            		cout<<"curl_multi_socket_action error, mc="<<mc<<"\n";
            	}

            check_multi_info();
        }
    }

    static void asio_socket_callback(const asio::error_code& ec,
    		CURL *easy,      /* easy handle */
            curl_socket_t s, /* socket */
            int what,        /* describes the socket */
			Session* session);
private:
    static void check_multi_info();
private:
    MultiInfo():still_running_(0), ioc_(1), timer_(ioc_)
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

using FinishHttp = void (*)(const string& url, string&& html);

// one http request and response
class Session
{
    friend class MultiInfo;

    CURL* easy_;
    string url_; // the request url
    string html_; // the result
    char error_[CURL_ERROR_SIZE];
    FinishHttp finish_callback_;
    asio::ip::tcp::socket* socket_;
    int newest_event_; //
public:
    Session(const string url, FinishHttp finish_cb) : easy_(nullptr), url_(url),
		finish_callback_(finish_cb), socket_(nullptr), newest_event_(0) {}

    ~Session()
    {
    	if(easy_)
    	{
            curl_multi_remove_handle(MultiInfo::Instance()->multi_, easy_);
            curl_easy_cleanup(easy_);
    	}

    	delete socket_;
    }

    int Init()
    {
        easy_ = curl_easy_init();
        if(!easy_)
        {
            cout<<"error to init easy\n";
            return -1;
        }

        curl_easy_setopt(easy_, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(easy_, CURLOPT_WRITEFUNCTION, Session::write_callback); // 某个连接收到数据了，需要保存数据在此回调函数中保存
        curl_easy_setopt(easy_, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(easy_, CURLOPT_VERBOSE, 1L); // curl输出连接中的更多信息
        curl_easy_setopt(easy_, CURLOPT_ERRORBUFFER, error_);
        curl_easy_setopt(easy_, CURLOPT_PRIVATE, this); // 存储一个指针，通过curl_easy_getinfo函数的CURLINFO_PRIVATE参数来取
        curl_easy_setopt(easy_, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_TIME, 3L);
        curl_easy_setopt(easy_, CURLOPT_LOW_SPEED_LIMIT, 10L);
        curl_easy_setopt(easy_, CURLOPT_OPENSOCKETFUNCTION, Session::opensocket_callback);
        curl_easy_setopt(easy_, CURLOPT_OPENSOCKETDATA, this);
        curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETFUNCTION, Session::closesocket_callback);
        curl_easy_setopt(easy_, CURLOPT_CLOSESOCKETDATA, this);
        CURLMcode mc = curl_multi_add_handle(MultiInfo::Instance()->multi_, easy_);
        return mc;
    }
private:
    // 收到数据了，在这个函数里面保存数据
    static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
    {
        size_t written = size * nmemb;
        string str(static_cast<const char*>(ptr), written);
        Session* s = static_cast<Session*>(userdata);
        s->html_.append(str);
        return written;
    }

    // set callback for opening sockets
    static curl_socket_t opensocket_callback(void *clientp,
            curlsocktype purpose,
            struct curl_sockaddr *address)
    {
    	curl_socket_t ret = CURL_SOCKET_BAD;

    	Session* s = static_cast<Session*>(clientp);
    	asio::io_context& ioc = MultiInfo::Instance()->ioc_;

    	// only for ipv4
        if (purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET)
        {
        	s->socket_ = new asio::ip::tcp::socket(ioc);
        	asio::error_code ec;
        	s->socket_->open(asio::ip::tcp::v4(), ec);
        	if(ec) // cannot open socket, maybe too many open files
        	{
        		std::cout<<"can't open file\n";
        	}
        	else
        	{
        		ret = s->socket_->native_handle();
        		MultiInfo::Instance()->socket_map_.insert(std::make_pair(ret, s));
        	}
        }

        return ret;
    }

    // callback to socket close replacement function
    static int closesocket_callback(void *clientp, curl_socket_t item)
    {
    	cout<<"********>closing fd="<<item<<"\n";
    	Session* s = static_cast<Session*>(clientp);
    	asio::error_code ec;
    	s->socket_->close(ec);
    	MultiInfo::Instance()->socket_map_.erase(item);
    	return ec ? ec.value() : 0;
    }
};

// 当libcurl中的某个socket需要监听的事件发生改变的时候，都会调用此函数；
// 比如说从没有变成可写、从可写变成可读等变化的时候都会调用此函数；
// 此函数的what传的是当前socket需要关注的全部事件，包括以前曾经通知过并且现在还需要的。
// 注意，因为asio每次wait都是一次性的，所以需要把以前的事件保存起来，每次wait之后都要再次调用wait。
int MultiInfo::socket_callback(CURL* easy, /* easy handle */
	curl_socket_t s, /* socket */
	int what, /* what to wait for */
	void* userp, /* private callback pointer */
	void* socketp)
{
	cout<<"========>socket_callback, s="<<s<<", what="<<what<<"\n";

	Session* session = MultiInfo::Instance()->socket_map_.find(s)->second;
	session->newest_event_ = what; // 目前最新的事件，保存在newest_event_成员中

	if(what == CURL_POLL_REMOVE)
	{
		return 0;
	}

	if(what & CURL_POLL_IN)
	{
		session->socket_->async_wait(asio::ip::tcp::socket::wait_read,
				std::bind(MultiInfo::asio_socket_callback, _1, easy, s, CURL_POLL_IN, session));
	}

	if(what & CURL_POLL_OUT)
	{
		session->socket_->async_wait(asio::ip::tcp::socket::wait_write,
				std::bind(MultiInfo::asio_socket_callback, _1, easy, s, CURL_POLL_OUT, session));
	}

	return 0;
}

inline void MultiInfo::asio_socket_callback(const asio::error_code& ec,
		CURL* easy, /* easy handle */
		curl_socket_t s, /* socket */
		int what, /* describes the socket */
		Session* session)
{
	cout<<"........>asio_socket_callback, ec="<<ec.value()<<", s="<<s<<", what="<<what<<"\n";

	if(ec) // asio socket error
	{
		what = CURL_CSELECT_ERR;
	}

	MultiInfo* multi = MultiInfo::Instance();
	CURLMcode rc = curl_multi_socket_action(multi->multi_, s, what, &multi->still_running_);
	if(rc != CURLM_OK)
	{
		cout<<"curl_multi_socket_action error, rc="<<int(rc)<<"\n";
	}

	check_multi_info();

	if(multi->still_running_ <= 0)
	{
		multi->timer_.cancel();
		return;
	}

	// 继续监听相关的事件，因为asio的wait函数都是一次性的，而libcurl对同一种事件没有发生变化时不会再次通知。
	// 上面调用了curl_multi_socket_action函数，对应的fd可能已经删除了，所以先检查一下，确认未删除，然后再重新监听事件
	if(!ec && multi->socket_map_.find(s) != multi->socket_map_.end())
	{
		// 最新需要关注的事件已经保存在newest_event_里面了，这里只要根据newest_event_的值进行添加就可以了
		if(what == CURL_POLL_IN && (session->newest_event_ & CURL_POLL_IN))
		{
			session->socket_->async_wait(asio::ip::tcp::socket::wait_read,
					std::bind(MultiInfo::asio_socket_callback, _1, easy, s, CURL_POLL_IN, session));
		}

		if(what == CURL_POLL_OUT && (session->newest_event_ & CURL_POLL_OUT))
		{
			session->socket_->async_wait(asio::ip::tcp::socket::wait_write,
					std::bind(MultiInfo::asio_socket_callback, _1, easy, s, CURL_POLL_OUT, session));
		}
	}
}

void MultiInfo::check_multi_info()
{
	int msgs_left;
	for(CURLMsg *msg = curl_multi_info_read(MultiInfo::Instance()->multi_, &msgs_left);
			msg != nullptr; msg = curl_multi_info_read(MultiInfo::Instance()->multi_, &msgs_left))
	{
		if (msg->msg == CURLMSG_DONE)
		{
			CURL *easy = msg->easy_handle;
			Session* s;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &s);
			s->finish_callback_(s->url_, std::move(s->html_));
			delete s;
		}
	}
}

void Finish(const string& url, string&& html)
{
	cout<<"finished, url="<<url<<", html:\n"; //<<html<<"\n";
}

int main()
{
	string urls[] = {"https://ec.haxx.se/libcurl-drive-multi-socket.html",
			//"https://ec.haxx.se/usingcurl-downloads.html",
			//"https://en.cppreference.com/w/cpp/container/vector",
	};

	for(const string& url: urls)
	{
		Session* session = new Session(url, Finish);
		int ret = session->Init();
		if(0 != ret)
		{
			cout<<"init error, ret="<<ret<<"\n";
			return -1;
		}
	}

	MultiInfo::Instance()->Run();
    return 0;
}
