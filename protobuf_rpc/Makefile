all:
	clang++ -std=c++14 -DASIO_STANDALONE -fcoroutines-ts -lstdc++ \
	calc.pb.cc pbrpc.cpp rpc_pkg.pb.cc calc-server.cpp \
	/usr/local/Cellar/protobuf\@2.6/2.6.1/lib/libprotobuf.a \
	 -o calc-server \
	-I../asio/asio/include -I/usr/local/Cellar/protobuf\@2.6/2.6.1/include/
	
	clang++ -std=c++14 -DASIO_STANDALONE -fcoroutines-ts -lstdc++ \
	calc.pb.cc pbrpc.cpp rpc_pkg.pb.cc calc-client.cpp \
	/usr/local/Cellar/protobuf\@2.6/2.6.1/lib/libprotobuf.a \
	 -o calc-client \
	-I../asio/asio/include -I/usr/local/Cellar/protobuf\@2.6/2.6.1/include/
	
	