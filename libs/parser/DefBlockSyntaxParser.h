#pragma once

#include <memory>
#include <vector>

#include "string/tokeniser.h"
#include "string/join.h"

namespace parser
{

// A structural element of the parsed text
// Can be of type text, whitespace or comment
struct DefSyntaxToken
{
    enum class Type
    {
        Nothing,
        Whitespace,
    };

    // Token type
    Type type;

    // The raw string as parsed from the source text
    std::string value;

    void clear()
    {
        type = Type::Nothing;
        value.clear();
    }
};

class DefSyntaxNode
{
public:
    using Ptr = std::shared_ptr<DefSyntaxNode>;

    enum class Type
    {
        Root,
        Whitespace,
    };

private:
    // All child nodes of this node
    std::vector<Ptr> _children;

    Type _type;

public:
    DefSyntaxNode(Type type) :
        _type(type)
    {}

    Type getType() const
    {
        return _type;
    }

    const std::vector<Ptr>& getChildren() const
    {
        return _children;
    }

    void appendChildNode(Ptr&& node)
    {
        _children.emplace_back(node);
    }

    virtual std::string getString() const
    {
        std::string value;
        value.reserve(getChildren().size() * 10);

        for (const auto& child : getChildren())
        {
            value += child->getString();
        }

        return value;
    }
};

class DefWhitespaceSyntax :
    public DefSyntaxNode
{
private:
    DefSyntaxToken _token;
public:
    DefWhitespaceSyntax(const DefSyntaxToken& token) :
        DefSyntaxNode(Type::Whitespace),
        _token(token)
    {
        assert(token.type == DefSyntaxToken::Type::Whitespace);
    }

    std::string getString() const override
    {
        return _token.value;
    }
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
        Searching,      // haven't found anything yet
        Whitespace,     // on whitespace
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
		_state(State::Searching)
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
        // Initialise state, no persistence between calls
        _state = State::Searching;

        // Clear out the token, no guarantee that it is empty
        tok.clear();

        while (next != end)
        {
            char ch = *next;

            switch (_state)
            {
            case State::Searching:
                if (IsWhitespace(ch))
                {
                    _state = State::Whitespace;
                    tok.type = DefSyntaxToken::Type::Whitespace;
                    tok.value += ch;
                    ++next;
                    continue;
                }
                break;
            case State::Whitespace:
                if (IsWhitespace(ch))
                {
                    tok.value += ch;
                    ++next;
                    continue;
                }

                // Ran out of whitespace, return token
                return true;
                break;
            }
        }

        // Return true if we have found a non-empty token
        return !tok.value.empty();
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
    // The returned syntax tree reference is never null
    DefSyntaxTree::Ptr parse()
    {
        auto syntaxTree = std::make_shared<DefSyntaxTree>();

        syntaxTree->root = std::make_shared<DefSyntaxNode>(DefSyntaxNode::Type::Root);

        while (!_tokIter.isExhausted())
        {
            auto token = *_tokIter++;

            switch (token.type)
            {
            case DefSyntaxToken::Type::Whitespace:
                syntaxTree->root->appendChildNode(std::make_shared<DefWhitespaceSyntax>(token));
                break;
            }
        }

        return syntaxTree;
    }
};

}
