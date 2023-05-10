/*
 * protocol_parser.cpp
 *
 *  Created on: Jul 26, 2018
 *      Author: frank
 */

#include <map>
#include <functional>

#include "response_parser.h"

std::shared_ptr<AbstractReplyItem> AbstractReplyItem::CreateItem(char c)
{
    std::map<char, std::function<AbstractReplyItem*()>> factory_func =
    {
    { '*', []()
    {   return new ArrayItem;} },
    { '+', []()
    {   return new SimpleStringItem;} },
    { '-', []()
    {   return new ErrString;} },
    { ':', []()
    {   return new NumberItem;} },
    { '$', []()
    {   return new BulkString;} }, };

    auto it = factory_func.find(c);
    if (it != factory_func.end())
    {
        return std::shared_ptr<AbstractReplyItem>(it->second());
    }

    return nullptr;
}

ParseResult ArrayItem::Feed(char c)
{
    switch (status_)
    {
    case AI_STATUS::PARSING_LENGTH:
        if (c != '\r')
        {
            if (std::isdigit(c) || (c == '-' && count_.size() == 0))
            {
                count_.push_back(c);
            }
            else
            {
                return ParseResult::PR_ERROR;
            }
        }
        else
        {
            item_count_ = std::stoi(count_);
            status_ = AI_STATUS::EXPECT_LF;
        }
        return ParseResult::PR_CONTINUE;
        break;
    case AI_STATUS::EXPECT_LF:
        if (c != '\n')
        {
            return ParseResult::PR_ERROR;
        }
        status_ = AI_STATUS::PARSING_SUB_ITEM_HEADER;

        if (item_count_ <= 0)
        {
            return ParseResult::PR_FINISHED;
        }
        return ParseResult::PR_CONTINUE;
        break;
    case AI_STATUS::PARSING_SUB_ITEM_HEADER:
        current_item_ = AbstractReplyItem::CreateItem(c);
        if (!current_item_)
        {
            return ParseResult::PR_ERROR;
        }
        items_.push_back(current_item_);
        status_ = AI_STATUS::PARSEING_SUB_ITEM_CONTENT;
        return ParseResult::PR_CONTINUE;
        break;
    case AI_STATUS::PARSEING_SUB_ITEM_CONTENT:
    {
        ParseResult pr = current_item_->Feed(c);
        if (pr == ParseResult::PR_ERROR)
        {
            return ParseResult::PR_ERROR;
        }
        if (pr == ParseResult::PR_FINISHED)
        {
            if (static_cast<int>(items_.size()) >= item_count_)
                return ParseResult::PR_FINISHED;
            status_ = AI_STATUS::PARSING_SUB_ITEM_HEADER;
        }
        return ParseResult::PR_CONTINUE;
        break;
    }
    default:
        return ParseResult::PR_ERROR;
        break;
    }
}

ParseResult OneLineString::Feed(char c)
{
    switch (status_)
    {
    case OLS_STATUS::PARSING_STRING:
        if (c == '\r')
        {
            status_ = OLS_STATUS::EXPECT_LF;
        }
        else
        {
            content_.push_back(c);
        }
        return ParseResult::PR_CONTINUE;
        break;
    case OLS_STATUS::EXPECT_LF:
        if (c == '\n')
        {
            return ParseResult::PR_FINISHED;
        }
        else
        {
            return ParseResult::PR_ERROR;
        }
        break;
    }

    return ParseResult::PR_ERROR; // impossible
}

ParseResult BulkString::Feed(char c)
{
    switch (status_)
    {
    case BULK_STATUS::BS_EXPECT_LENGTH:
        if (c == '-' or std::isdigit(c))
        {
            length_line_.push_back(c);
        }
        else if (c == '\r')
        {
            status_ = BULK_STATUS::BS_EXPECT_LENGTH_LF;
        }
        else
        {
            // error
        }
        return ParseResult::PR_CONTINUE;
        break;
    case BULK_STATUS::BS_EXPECT_LENGTH_LF:
        if (c != '\n')
        {
            // err
        }
        length_ = std::stoi(length_line_);
        if (length_ == 0 || length_ == -1)
        {
            return ParseResult::PR_FINISHED;
        }
        else if (length_ < 0)
        {
            // err
        }
        status_ = BULK_STATUS::BS_EXPECT_CONTENT;
        return ParseResult::PR_CONTINUE;
        break;
    case BULK_STATUS::BS_EXPECT_CONTENT:
        if (c == '\r')
        {
            status_ = BULK_STATUS::BS_EXPECT_CONTENT_LF;
        }
        else
        {
            content_.push_back(c);
        }
        return ParseResult::PR_CONTINUE;
        break;
    case BULK_STATUS::BS_EXPECT_CONTENT_LF:
        if (c != '\n')
        {
            //err
        }
        return ParseResult::PR_FINISHED;
        break;
    }
    return ParseResult::PR_ERROR;
}

ParseResult NumberItem::Feed(char c)
{
    ParseResult pr = OneLineString::Feed(c);
    if (pr == ParseResult::PR_FINISHED)
    {
        number_ = std::stoi(content_);
    }
    return pr;
}
