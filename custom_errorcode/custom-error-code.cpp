/*
custom-error-code.cpp
                      frank Jul 12, 2018
*/
#include <type_traits>
#include <iostream>
#include <asio.hpp>
#include "my_error.h" // this header file demonstraits everthing needed to  define custom error codes in c++11

using namespace std;

int main()
{
	static_assert(std::is_same<asio::error_category, std::error_category>::value, "not same");
	asio::error_code ec;
	ec = error_1;

	std::cout<<ec.message()<<"\n";

	return 0;
}


