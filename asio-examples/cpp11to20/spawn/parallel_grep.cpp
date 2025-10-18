#include "asio.hpp"
#include <fstream>
#include <iostream>
#include <string>

using asio::detached;
using asio::dispatch;
// using asio::spawn;
using asio::strand;
using asio::thread_pool;
// using asio::yield_context;

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: parallel_grep <string> <files...>\n";
            return 1;
        }

        // We use a fixed size pool of threads for reading the input files. The
        // number of threads is automatically determined based on the number of
        // CPUs available in the system.
        thread_pool pool;

        // To prevent the output from being garbled, we use a strand to
        // synchronise printing.
        strand<thread_pool::executor_type> output_strand(pool.get_executor());

        // Spawn a new coroutine for each file specified on the command line.
        std::string search_string = argv[1];
        for (int argn = 2; argn < argc; ++argn) {
            std::string input_file = argv[argn];
            asio::co_spawn(
                pool,
                [=]() -> asio::awaitable<void> {
                    std::ifstream is(input_file.c_str());
                    std::string line;
                    std::size_t line_num = 0;
                    while (std::getline(is, line)) {
                        // If we find a match, send a message to the output.
                        if (line.find(search_string) != std::string::npos) {
                            asio::dispatch(output_strand, [=] {
                                std::cout << input_file << ':' << line
                                          << std::endl;
                            });
                        }

                        // 2024-06-12 之前用有栈协程是用下面的几行代码让出时间片，
                        // 改成无栈协程之后，应该是要用co_await this_coro::sleep来取代，
                        // 否则看起来效果还是串行的。
                        // Every so often we yield control to another coroutine.
                        //if (++line_num % 10 == 0)
                        //    post(yield);
                    }

                    co_return;
                },
                asio::detached);
        }

        // Join the thread pool to wait for all the spawned tasks to complete.
        pool.join();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
