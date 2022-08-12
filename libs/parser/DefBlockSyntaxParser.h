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
        OpeningBrace,
        ClosingBrace,
        Token,
        EolComment,
        BlockComment,
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
        Comment,
        DeclType,
        DeclName,
        BlockStart,
        BlockContent,
        BlockEnd,
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

/**
 * Stateless tokeniser function cutting the incoming character range into
 * qualified DefSyntaxTokens, returning one token at a time.
 */
class DefBlockSyntaxTokeniserFunc
{
    // Enumeration of states
    enum class State
    {
        Searching,      // haven't found anything yet
        Whitespace,     // on whitespace
        Token,          // non-whitespace, non-control character
        BlockComment,   // within a /* block comment */
        EolComment,     // on an EOL comment starting with //
    } _state;

    constexpr static const char* const Delims = " \t\n\v\r";

	constexpr static char OpeningBrace = '{';
	constexpr static char ClosingBrace = '}';

    static bool IsWhitespace(char c)
	{
        for (const char* curDelim = Delims; *curDelim != 0; ++curDelim)
        {
            if (*curDelim == c) return true;
        }
        return false;
    }

public:
    DefBlockSyntaxTokeniserFunc() :
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

        std::size_t blockLevel = 0;

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

                if (ch == OpeningBrace)
                {
                    tok.type = DefSyntaxToken::Type::OpeningBrace;
                    tok.value += ch;
                    ++next;
                    return true;
                }
                
                if (ch == ClosingBrace)
                {
                    tok.type = DefSyntaxToken::Type::ClosingBrace;
                    tok.value += ch;
                    ++next;
                    return true;
                }
                
                if (ch == '/')
                {
                    // Token type not yet determined switch to forward slash mode
                    tok.value += ch;
                    ++next;

                    // Check the next character, it determines what we have
                    if (next != end)
                    {
                        if (*next == '*')
                        {
                            _state = State::BlockComment;
                            tok.type = DefSyntaxToken::Type::BlockComment;
                            tok.value += '*';
                            ++next;
                            continue;
                        }

                        if (*next == '/')
                        {
                            _state = State::EolComment;
                            tok.type = DefSyntaxToken::Type::EolComment;
                            tok.value += '/';
                            ++next;
                            continue;
                        }
                    }
                }
                 
                tok.type = DefSyntaxToken::Type::Token;
                _state = State::Token;
                tok.value += ch;
                ++next;
                continue;

            case State::Whitespace:
                if (IsWhitespace(ch))
                {
                    tok.value += ch;
                    ++next;
                    continue;
                }

                // Ran out of whitespace, return token
                return true;

            case State::BlockComment:
                // Inside a delimited comment, we add everything to the token 
                // and check for the "*/" sequence.
                tok.value += ch;
                ++next;

                // If we just added an asterisk, check if we hit the end
                if (ch == '*' && next != end && *next == '/')
                {
                    // Add the slash and close this block comment
                    tok.value += '/';
                    ++next;
                    return true;
                }

                continue; // carry on

            case State::EolComment:
                // This comment lasts until the end of the line.
                if (ch == '\r' || ch == '\n')
                {
                    // Stop here, leave next where it is
                    return true;
                }
                
                // Add to comment and continue
                tok.value += ch;
                ++next;
                continue;

            case State::Token:
                assert(!tok.value.empty());

                if (ch == OpeningBrace || ch == ClosingBrace)
                {
                    // Finalise the token and leave next where it is
                    return true;
                }
                else if (ch == '/')
                {
                    // Check for a block comment
                    auto afterNext = next;
                    ++afterNext;

                    if (afterNext != end && (*afterNext == '*' || *afterNext == '/'))
                    {
                        // This ends our token
                        return true;
                    }

                    // Not starting a comment, continue
                    tok.value += ch;
                    ++next;
                    continue;
                }
                else if (IsWhitespace(ch))
                {
                    // Whitespace terminates our token
                    return true;
                }
                
                tok.value += ch;
                ++next;
                continue;
            }
        }

        // Return true if we have found a non-empty token
        return !tok.value.empty();
    }
}; 

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
    using Tokeniser = string::Tokeniser<DefBlockSyntaxTokeniserFunc,
        std::string::const_iterator, DefSyntaxToken>;

    Tokeniser _tok;
    Tokeniser::Iterator _tokIter;

public:
    DefBlockSyntaxParser(const ContainerType& str) :
        _tok(str, DefBlockSyntaxTokeniserFunc()),
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
