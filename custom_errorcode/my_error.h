/*
 * my_error.h
 *
 *  Created on: Jul 20, 2018
 *      Author: frank
 */

#ifndef CUSTOM_ERRORCODE_MY_ERROR_H_
#define CUSTOM_ERRORCODE_MY_ERROR_H_

#include <system_error>
#include <string>

enum my_errors
{
	error_1 = 1,
	error_2,
	error_3
};

class my_error_catetory_impl: public std::error_category
{
public:
	const char* name() const noexcept { return "my_errors"; }
	std::string message(int e) const
	{
		switch(e)
		{
		case error_1:
			return "error1 occured.";
			break;
		case error_2:
			return "error2 occured.";
			break;
		case error_3:
			return "error3 occured.";
			break;
		default:
			return "unknow error!";
			break;
		}
	}
};

inline const std::error_category& get_my_error_category()
{
	static my_error_catetory_impl impl;
	return impl;
}

static const std::error_category& my_error_category = get_my_error_category();

std::error_code make_error_code(my_errors e)
{
	return std::error_code(static_cast<int>(e), my_error_category);
}

namespace std {
template<> struct is_error_code_enum<my_errors>: public std::true_type
{
};
}


#endif /* CUSTOM_ERRORCODE_MY_ERROR_H_ */
