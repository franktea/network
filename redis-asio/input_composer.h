/**
 redis-protocol.h
 frank Jun 19, 2018
 */

#ifndef REDIS_ASIO_REDIS_PROTOCOL_H_
#define REDIS_ASIO_REDIS_PROTOCOL_H_

#include <string>
#include <vector>

// redis protocol,
// see http://redisdoc.com/topic/protocol.html

// compose inputs to redis arrays, like this:
// *param_count\r\n$param_len1\r\nparam1\r\n$param_len2\r\n....
class InputComposer
{
public:
    void Clear()
    {
        param_count_ = 0;
        params_.clear();
    }

    // addline when receive data, if bulk complete, return true
    bool AddLine(std::string line)
    {
        switch (line[0])
        {
        case '*':
            line.erase(0);
            param_count_ = std::stoi(line);
            break;
        case '$':
            line.erase(0);
            params_.push_back(std::pair<int, std::string>(std::stoi(line), ""));
            break;
        default:
            params_[params_.size() - 1].second = line;
            if (static_cast<int>(params_.size()) == param_count_)
            {
                return true;
            }
            break;
        }

        return false;
    }

    static std::string ComposeInputToBulk(std::string input_line)
    {
        // split by space
        auto f =
                [](const std::string& str, const std::string& delimiters,
                        std::vector<std::string>& tokens)
                {
                    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
                    std::string::size_type pos = str.find_first_of(delimiters, lastPos);
                    while (std::string::npos != pos || std::string::npos != lastPos)
                    {
                        tokens.push_back(str.substr(lastPos, pos - lastPos));
                        lastPos = str.find_first_not_of(delimiters, pos);
                        pos = str.find_first_of(delimiters, lastPos);
                    }
                };

        std::vector<std::string> words;
        f(input_line, " ", words);
        std::string&& result("*");
        result += std::to_string(words.size());
        for (const std::string& s : words)
        {
            result += std::string("\r\n$") + std::to_string(s.size())
                    + std::string("\r\n") + s;
        }

        result += std::string("\r\n");
        return result;
    }
private:
    int param_count_;
    std::vector<std::pair<int, std::string>> params_; // pair<length, param> in each item
};

#endif /* REDIS_ASIO_REDIS_PROTOCOL_H_ */
