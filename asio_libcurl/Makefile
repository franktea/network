all:
	clang++ -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c++2b asio-libcurl.cpp -lcurl -I ../build/_deps/asio-src/asio/include/ -o asio-libcurl
	clang++ -O1 -g -fsanitize=address -fno-omit-frame-pointer -std=c++2b async-libcurl.cpp -lcurl -I ../build/_deps/asio-src/asio/include/ -o async-libcurl

clean:
	rm -f asio-libcurl async-libcurl

.PHONEY: clean
