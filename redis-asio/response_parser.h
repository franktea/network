/*
 * protocol_parser.h
 *
 *  Created on: Jul 26, 2018
 *      Author: frank
 */

#ifndef REDIS_ASIO_RESPONSE_PARSER_H_
#define REDIS_ASIO_RESPONSE_PARSER_H_

#include <memory>
#include <string>
#include <vector>

// response protocol parser
// response format: https://redis.io/topics/protocol

class ArrayItem;
class SimpleStringItem;
class ErrString;
class NumberItem;
class BulkString;

enum class ParseResult
{
    PR_INIT, PR_CONTINUE, PR_FINISHED, PR_ERROR
};

class AbstractReplyItem: public std::enable_shared_from_this<AbstractReplyItem>
{
public:
    virtual std::string ToString() = 0;
    virtual ParseResult Feed(char c) = 0;
    virtual ~AbstractReplyItem()
    {
    }
    // factory method
    static std::shared_ptr<AbstractReplyItem> CreateItem(char c);
};

// array,  start with *
class ArrayItem: public AbstractReplyItem
{
    enum class AI_STATUS
    {
        PARSING_LENGTH, EXPECT_LF, // parsing length, got \r, expect \n
        PARSING_SUB_ITEM_HEADER, // expect $ + - :
        PARSEING_SUB_ITEM_CONTENT
    };
public:
    ArrayItem() :
            AbstractReplyItem(), status_(AI_STATUS::PARSING_LENGTH)
    {
    }

    std::string ToString() override
    {
        // item_count_ == -1 is not considered here
        std::string result("[");
        for (size_t i = 0; i < items_.size(); ++i)
        {
            if (i > 0)
                result.append(", ");
            result.append(items_[i]->ToString());
        }
        result.append("]");
        return result;
    }

    ParseResult Feed(char c) override;
private:
    int item_count_;
    std::string count_;
    AI_STATUS status_;
    std::shared_ptr<AbstractReplyItem> current_item_;
    std::vector<std::shared_ptr<AbstractReplyItem>> items_;
};

//simple line of string, start with +, -, :
class OneLineString: public AbstractReplyItem
{
    enum class OLS_STATUS
    {
        PARSING_STRING, EXPECT_LF // got \r, expect \n
    };
public:
    std::string ToString() override
    {
        return content_;
    }

    ParseResult Feed(char c) override;
protected:
    OLS_STATUS status_ = OLS_STATUS::PARSING_STRING;
    std::string content_;
};

// start with +
class SimpleStringItem: public OneLineString
{
};

// start with -
class ErrString: public OneLineString
{
public:
    std::string ToString() override
    {
        return std::string("(error) ") + content_;
    }
};

// string with length, start with $
class BulkString: public AbstractReplyItem
{
    enum class BULK_STATUS
    {
        BS_EXPECT_LENGTH, // first line, 0 for empty string, -1 for null
        BS_EXPECT_LENGTH_LF, // first line of \n
        BS_EXPECT_CONTENT,
        BS_EXPECT_CONTENT_LF // next line of \n
    };
public:
    std::string ToString() override
    {
        if (length_ == -1)
            return "(nil)";

        return std::string("\"") + content_ + std::string("\"");
    }

    ParseResult Feed(char c) override;
private:
    BULK_STATUS status_ = BULK_STATUS::BS_EXPECT_LENGTH;
    int length_ = 0;
    std::string length_line_; // length, save temporarily as a string
    std::string content_;
};

// number item, start with :
class NumberItem: public OneLineString
{
public:
    std::string ToString() override
    {
        return std::string("(integer) ") + std::to_string(number_);
    }

    ParseResult Feed(char c) override;
private:
    int number_ = -1;
};

#endif /* REDIS_ASIO_RESPONSE_PARSER_H_ */
