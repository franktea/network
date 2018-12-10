/*
demonstrate usage of poll post run and poll
                      frank Jul 7, 2018
*/

#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>
#include <stdint.h>
#include "asio.hpp"

using namespace std;

// 单线程，为了主循环不阻塞，可以调用io_context::poll
void OneThreadWithPoll()
{
	asio::io_context ios;

	uint16_t s = 0;
	while(++s)
	{
		ios.restart(); // 注意调restart，否则后面的回调都不会执行
		ios.poll();  // poll不会阻塞，可以在自己的主循环里面调用
		ios.post([s]() { cout<<s; cout.flush(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

// 在单独一个线程中调用run，在其它线程中post
void MultiThreadRun()
{
	asio::io_context io; // 无论是post还是dispatch，其待调用的函数都是在io_context::run的线程里面运行
	asio::strand<asio::io_context::executor_type> str(io.get_executor()); // strand的用法，这里用来防止cout输出错乱

	thread t1([&io, &str]() {
		(void)str; // unused warning
		//asio::executor_work_guard<asio::io_context::executor_type> wg
		auto wg = asio::make_work_guard(io) ; // executor_work_guard可以防止io.run()在事件处理完以后退出
		io.run(); // 从任何线程发起的post/dispatch的函数都在调用run/poll的线程中执行
	});

	thread t2([&io, &str]() {
//		cout<<"thread2 id:"<<std::this_thread::get_id()<<"\n";

		for(int i = 0; i < 10; ++i)
		{
			// 如果是调用dispatch的线程和调用io_context::run的所有线程都不是同一个线程(因为可以在多个线程中调用run)，则dispatch和post的效果是一样的。
			// 此处就是一样, dispatch就等于post，都会被post到另一个线程中去执行
			io.dispatch(asio::bind_executor(str, []() {
				cout<<"dispatch, threadid:"<<std::this_thread::get_id()<<"\n";
			}));
			io.post(asio::bind_executor(str, []() {
				cout<<"post, threadid:"<<std::this_thread::get_id()<<"\n";
			}));

			//下面就不一样了，首先是回调函数被post到t1中去执行
			//后来再执行内部的dispatch和post时，已经是在t1中了，所以会立即执行dispatch，而post会最后执行
			//得到的效果就是所有的disatch都执行完了，才执行post
			io.post([&io, &str]() {
				io.dispatch(asio::bind_executor(str, []() {
					cout<<"dispatch will run first, threadid:"<<std::this_thread::get_id()<<"\n";
				}));
				io.post(asio::bind_executor(str, []() {
					cout<<"post will run after all dispatch, threadid:"<<std::this_thread::get_id()<<"\n";
				}));
			});
		}
	});

	t2.join();
	t1.join();

}

int main()
{
	// OneThreadWithPoll();

	MultiThreadRun();

	return 0;
}


