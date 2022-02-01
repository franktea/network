/*
 * protocol_parser_test.cpp
 *
 *  Created on: Jul 26, 2018
 *      Author: frank
 */
#include "response_parser.h"

#include <string>

#include <boost/ut.hpp>

using std::string;
using namespace boost::ut;

class StringParser
{
public:
    void ParseString(const string &s)
    {
        for (size_t i = 0;
             i < s.size() && pr_ != ParseResult::PR_FINISHED && pr_ != ParseResult::PR_ERROR; ++i)
        {
            if (!item_)
            {
                item_ = AbstractReplyItem::CreateItem(s[i]);
                continue;
            }

            pr_ = item_->Feed(s[i]);
        }
    }

    std::shared_ptr<AbstractReplyItem> item_;
    ParseResult pr_ = ParseResult::PR_INIT;
};

int main()
{
    // test simple string
    "test simple string"_test = []
    {
        string s = "+OK\r\n";
        StringParser sp;
        sp.ParseString(s);
        expect(sp.pr_ == ParseResult::PR_FINISHED);
        expect(sp.item_->ToString() == "OK");
    };

    // test resp error
    "test resp error"_test = []
    {
        string s = "-Error message\r\n";
        StringParser sp;
        sp.ParseString(s);
        expect(sp.pr_ == ParseResult::PR_FINISHED);
        expect(sp.item_->ToString() == "(error) Error message");
    };

    // test resp integer
    "test resp integer"_test = []
    {
        string s = ":-1000\r\n";
        StringParser sp;
        sp.ParseString(s);
        expect(sp.pr_ == ParseResult::PR_FINISHED);
        expect(sp.item_->ToString() == "(integer) -1000");
    };

    // resp bulk strings
    "test bulk string"_test = []
    {
        {
            string s = "$6\r\nfoobar\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(sp.item_->ToString() == "\"foobar\"");
        }

        {
            string s = "$-1\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(sp.item_->ToString() == "(nil)");
        }
    };

    // test resp array
    "test resp array"_test = []
    {
        { // empty array
            string s = "*0\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(sp.item_->ToString() == "[]");
        }

        { // complicate array
            string s = "*5\r\n"
                       ":1\r\n"
                       ":2\r\n"
                       ":3\r\n"
                       ":4\r\n"
                       "$6\r\n"
                       "foobar\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(
                sp.item_->ToString() == "[(integer) 1, (integer) 2, (integer) 3, (integer) 4, \"foobar\"]");
        }

        { // arrays of arrays
            string s = "*2\r\n"
                       "*3\r\n"
                       ":1\r\n"
                       ":2\r\n"
                       ":3\r\n"
                       "*2\r\n"
                       "+Foo\r\n"
                       "-Bar\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(
                sp.item_->ToString() == "[[(integer) 1, (integer) 2, (integer) 3], [Foo, (error) Bar]]");
        }

        { // null element in arrays
            string s = "*3\r\n"
                       "$3\r\n"
                       "foo\r\n"
                       "$-1\r\n"
                       "$3\r\n"
                       "bar\r\n";
            StringParser sp;
            sp.ParseString(s);
            expect(sp.pr_ == ParseResult::PR_FINISHED);
            expect(sp.item_->ToString() == "[\"foo\", (nil), \"bar\"]");
        }
    };
}