#pragma once

#include <memory>

#include "ParseException.h"
#include "string/tokeniser.h"

namespace parser
{

// A structural element of the parsed text
// Can be of type text, whitespace or comment
struct DefSyntaxToken
{
    
};

struct DefSyntaxNode
{
    using Ptr = std::shared_ptr<DefSyntaxNode>;


};

struct DefSyntaxTree
{
    using Ptr = std::shared_ptr<DefSyntaxTree>;

    DefSyntaxNode::Ptr root;
};

namespace detail
{

/**
 * Stateful tokeniser function cutting the incoming character range into
 * qualified DefSyntaxTokens.
 */
class DefSyntaxTokeniserFunc
{
    // Enumeration of states
    enum class State
    {
        None,	  // haven't found anything yet
    } _state;

    constexpr static const char* const Delims = " \t\n\v\r";

	constexpr static char BlockStartChar = '{';
	constexpr static char BlockEndChar = '}';

    static bool IsWhitespace(char c)
	{
        const char* curDelim = Delims;
        while (*curDelim != 0) {
            if (*(curDelim++) == c) {
                return true;
            }
        }
        return false;
    }

public:
    DefSyntaxTokeniserFunc() :
		_state(State::None)
    {}

    /**
     * REQUIRED by the string::Tokeniser. The operator() will be invoked by the Tokeniser.
     * This function must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true. The function will return false,
     * meaning it didn't find anything before reaching the end iterator.
     */
    template<typename InputIterator>
    bool operator() (InputIterator& next, const InputIterator& end, DefSyntaxToken& tok)
	{
        // Return true if we have found a named block
        return false;
    }
}; 

}

/**
 * Parses and cuts decl file contents into syntax blocks.
 * Each block structure has a name, an optional typename,
 * the raw block contents. Every block can have leading and trailing
 * non-decl syntax elements like whitespace and/or comments.
 */
template<typename ContainerType>
class DefBlockSyntaxParser
{
private:
    // Internal tokeniser and its iterator
    using Tokeniser = string::Tokeniser<detail::DefSyntaxTokeniserFunc,
        std::string::const_iterator, DefSyntaxToken>;

    Tokeniser _tok;
    Tokeniser::Iterator _tokIter;

public:
    DefBlockSyntaxParser(const ContainerType& str) :
        _tok(str, detail::DefSyntaxTokeniserFunc()),
        _tokIter(_tok.getIterator())
    {}

    // Parse the text stored in the container into a def syntax tree
    DefSyntaxTree::Ptr parse()
    {
        return {};
    }
};

}
