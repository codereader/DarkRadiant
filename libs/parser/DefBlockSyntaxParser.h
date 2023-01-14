#pragma once

#include <memory>
#include <vector>
#include <algorithm>

#include "string/tokeniser.h"
#include "string/join.h"
#include "string/trim.h"

namespace parser
{

// A snippet of the source text
// Can be strings, whitespace or comments
struct DefSyntaxToken
{
    enum class Type
    {
        Nothing,
        Whitespace,
        BracedBlock, // starting with { and *maybe* ending with }
        Token,
        EolComment,
        BlockComment,
    };

    // Token type
    Type type;

    // The raw string as parsed from the source text
    std::string value;

    DefSyntaxToken() :
        DefSyntaxToken(Type::Nothing, "")
    {}

    DefSyntaxToken(Type type_, const std::string& value_) :
        type(type_),
        value(value_)
    {}

    void clear()
    {
        type = Type::Nothing;
        value.clear();
    }
};

// Represents an element of a parsed syntax tree.
// Each node can have 0 or more child nodes, grouping them
// into a meaningful structure.
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
        DeclBlock,
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

    void removeChildNode(const Ptr& node)
    {
        auto existing = std::find(_children.begin(), _children.end(), node);

        if (existing != _children.end())
        {
            _children.erase(existing);
        }
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
    using Ptr = std::shared_ptr<DefWhitespaceSyntax>;

    DefWhitespaceSyntax(const DefSyntaxToken& token) :
        DefSyntaxNode(Type::Whitespace),
        _token(token)
    {
        assert(token.type == DefSyntaxToken::Type::Whitespace);
    }

    // Returns the number of line breaks this whitespace contains
    std::size_t getNumberOfLineBreaks()
    {
        return std::count_if(_token.value.begin(), _token.value.end(), [](char c) { return c == '\n'; });
    }

    std::string getString() const override
    {
        return _token.value;
    }

    static Ptr Create(const std::string& whitespace)
    {
        return std::make_shared<DefWhitespaceSyntax>(
            DefSyntaxToken(DefSyntaxToken::Type::Whitespace, whitespace)
        );
    }
};

class DefCommentSyntax :
    public DefSyntaxNode
{
private:
    DefSyntaxToken _token;
public:
    DefCommentSyntax(const DefSyntaxToken& token) :
        DefSyntaxNode(Type::Comment),
        _token(token)
    {
        assert(token.type == DefSyntaxToken::Type::BlockComment || token.type == DefSyntaxToken::Type::EolComment);
    }

    std::string getString() const override
    {
        return _token.value;
    }
};

class DefTypeSyntax :
    public DefSyntaxNode
{
private:
    DefSyntaxToken _token;
public:
    using Ptr = std::shared_ptr<DefTypeSyntax>;

    DefTypeSyntax(const DefSyntaxToken& token) :
        DefSyntaxNode(Type::DeclType),
        _token(token)
    {
        assert(token.type == DefSyntaxToken::Type::Token);
    }

    const DefSyntaxToken& getToken() const
    {
        return _token;
    }

    std::string getString() const override
    {
        return _token.value;
    }

    static Ptr Create(const std::string& typeName)
    {
        return std::make_shared<DefTypeSyntax>(
            DefSyntaxToken(DefSyntaxToken::Type::Token, typeName)
        );
    }
};

class DefNameSyntax :
    public DefSyntaxNode
{
private:
    DefSyntaxToken _token;
public:
    using Ptr = std::shared_ptr<DefNameSyntax>;

    DefNameSyntax(const DefSyntaxToken& token) :
        DefSyntaxNode(Type::DeclName),
        _token(token)
    {
        assert(token.type == DefSyntaxToken::Type::Token);
    }

    const DefSyntaxToken& getToken() const
    {
        return _token;
    }

    void setName(const std::string& name)
    {
        _token = DefSyntaxToken(DefSyntaxToken::Type::Token, name);
    }

    std::string getString() const override
    {
        return _token.value;
    }

    static Ptr Create(const std::string& name)
    {
        return std::make_shared<DefNameSyntax>(
            DefSyntaxToken(DefSyntaxToken::Type::Token, name)
        );
    }
};

class DefBlockSyntax :
    public DefSyntaxNode
{
private:
    DefSyntaxToken _blockToken;

    std::vector<DefSyntaxNode::Ptr> _headerNodes;

    DefTypeSyntax::Ptr _type;
    DefNameSyntax::Ptr _name;
public:
    using Ptr = std::shared_ptr<DefBlockSyntax>;

    DefBlockSyntax(const DefSyntaxToken& blockToken, std::vector<DefSyntaxNode::Ptr>&& headerNodes, 
                   int nameIndex = -1, int typeIndex = -1) :
        DefSyntaxNode(Type::DeclBlock),
        _blockToken(blockToken),
        _headerNodes(headerNodes)
    {
        assert(_blockToken.type == DefSyntaxToken::Type::BracedBlock);

        if (nameIndex != -1)
        {
            _name = std::static_pointer_cast<DefNameSyntax>(_headerNodes.at(nameIndex));
        }

        if (typeIndex != -1)
        {
            _type = std::static_pointer_cast<DefTypeSyntax>(_headerNodes.at(typeIndex));
        }
    }

    // Returns the type identifier of this block (can be empty)
    const DefTypeSyntax::Ptr& getType() const
    {
        return _type;
    }

    // Returns the name identifier of this block (can be empty)
    const DefNameSyntax::Ptr& getName() const
    {
        return _name;
    }

    // Returns the raw block contents without the opening and closing braces
    std::string getBlockContents() const
    {
        return string::trim_copy(_blockToken.value, "{}");
    }

    // Sets the raw block contents without the opening and closing braces
    void setBlockContents(const std::string& contents)
    {
        _blockToken.value = "{" + contents + "}";
    }

    std::string getString() const override
    {
        std::string output;
        output.reserve(_blockToken.value.size() + _headerNodes.size() * 50);

        for (const auto& headerNode : _headerNodes)
        {
            if (!headerNode) continue;

            output.append(headerNode->getString());
        }

        output.append(_blockToken.value);

        return output;
    }

    static Ptr CreateTypedBlock(const std::string& type, const std::string& name)
    {
        std::vector<DefSyntaxNode::Ptr> headerNodes;

        int typeIndex = -1;

        if (!type.empty())
        {
            typeIndex = static_cast<int>(headerNodes.size());
            headerNodes.emplace_back(DefTypeSyntax::Create(type));
            headerNodes.emplace_back(DefWhitespaceSyntax::Create(" "));
        }

        int nameIndex = static_cast<int>(headerNodes.size());
        headerNodes.emplace_back(DefNameSyntax::Create(name));
        headerNodes.emplace_back(DefWhitespaceSyntax::Create("\n"));

        DefSyntaxToken blockToken(DefSyntaxToken::Type::BracedBlock, "{}");
        return std::make_shared<DefBlockSyntax>(blockToken, std::move(headerNodes), nameIndex, typeIndex);
    }
};

class DefSyntaxTree
{
private:
    DefSyntaxNode::Ptr _root;

public:
    using Ptr = std::shared_ptr<DefSyntaxTree>;

    DefSyntaxTree() :
        _root(std::make_shared<DefSyntaxNode>(DefSyntaxNode::Type::Root))
    {}

    const DefSyntaxNode::Ptr& getRoot()
    {
        return _root;
    }

    void foreachBlock(const std::function<void(const DefBlockSyntax::Ptr&)>& functor)
    {
        for (const auto& node : getRoot()->getChildren())
        {
            if (!node || node->getType() != DefSyntaxNode::Type::DeclBlock) continue;

            auto blockNode = std::static_pointer_cast<DefBlockSyntax>(node);

            functor(blockNode);
        }
    }

    // Return the first block syntax node matching the given name (case-sensitive)
    parser::DefBlockSyntax::Ptr findFirstNamedBlock(const std::string& name)
    {
        return findFirstBlock([&](const parser::DefBlockSyntax::Ptr& block)
        {
            return block->getName() && block->getName()->getString() == name;
        });
    }

    // Find the first block matching the given predicate
    parser::DefBlockSyntax::Ptr findFirstBlock(const std::function<bool(const parser::DefBlockSyntax::Ptr&)>& predicate)
    {
        parser::DefBlockSyntax::Ptr result;

        foreachBlock([&](const parser::DefBlockSyntax::Ptr& block)
        {
            if (!result && predicate(block))
            {
                result = block;
            }
        });

        return result;
    }

    std::string getString() const
    {
        return _root->getString();
    }
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
        Searching,                // haven't found anything yet
        Whitespace,               // on whitespace
        Token,                    // non-whitespace, non-control character
        BracedBlock,              // within a braced block
        BlockComment,             // within a /* block comment */
        EolComment,               // on an EOL comment starting with //
        QuotedStringWithinBlock,  // within a quoted string within a block
        BlockCommentWithinBlock,  // within a /* block comment */ within a block
        EolCommentWithinBlock,    // an EOL within a block
    };

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
        auto state = State::Searching;

        // Clear out the token, no guarantee that it is empty
        tok.clear();

        std::size_t openedBlocks = 0;

        while (next != end)
        {
            char ch = *next;

            switch (state)
            {
            case State::Searching:
                if (IsWhitespace(ch))
                {
                    state = State::Whitespace;
                    tok.type = DefSyntaxToken::Type::Whitespace;
                    tok.value += ch;
                    ++next;
                    continue;
                }

                if (ch == OpeningBrace)
                {
                    state = State::BracedBlock;
                    tok.type = DefSyntaxToken::Type::BracedBlock;
                    tok.value += ch;
                    openedBlocks = 1;
                    ++next;
                    continue;
                }
                
                if (ch == '/')
                {
                    // Could be a comment
                    tok.value += ch;
                    ++next;

                    // Check the next character, it determines what we have
                    if (next != end)
                    {
                        if (*next == '*')
                        {
                            state = State::BlockComment;
                            tok.type = DefSyntaxToken::Type::BlockComment;
                            tok.value += '*';
                            ++next;
                            continue;
                        }

                        if (*next == '/')
                        {
                            state = State::EolComment;
                            tok.type = DefSyntaxToken::Type::EolComment;
                            tok.value += '/';
                            ++next;
                            continue;
                        }
                    }
                }
                 
                tok.type = DefSyntaxToken::Type::Token;
                state = State::Token;
                tok.value += ch;
                ++next;
                continue;

            case State::BracedBlock:

                // Add the character and advance in any case
                tok.value += ch;
                ++next;

                // Check for another opening brace
                if (ch == OpeningBrace)
                {
                    // another block within this block, ignore this
                    ++openedBlocks;
                }
                else if (ch == ClosingBrace && --openedBlocks == 0)
                {
                    // End of block content, we're done here
                    return true;
                }
                else if (ch == '"')
                {
                    // An opening quote within the braced block, switch to a special block
                    // ignoring any control characters within that string
                    state = State::QuotedStringWithinBlock;
                }
                else if (ch == '/')
                {
                    // Could be a comment, check the next character, it determines what we have
                    if (next != end)
                    {
                        if (*next == '*')
                        {
                            state = State::BlockCommentWithinBlock;
                            tok.value += '*';
                            ++next;
                            continue;
                        }

                        if (*next == '/')
                        {
                            state = State::EolCommentWithinBlock;
                            tok.value += '/';
                            ++next;
                            continue;
                        }
                    }
                }
                continue;

            case State::QuotedStringWithinBlock:
                // Add the character and advance over anything
                tok.value += ch;
                ++next;

                if (ch == '"')
                {
                    state = State::BracedBlock;
                }
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
            case State::BlockCommentWithinBlock:
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

                    if (state == State::BlockComment)
                    {
                        return true;
                    }

                    // switch back to block mode
                    state = State::BracedBlock;
                }

                continue; // carry on

            case State::EolComment:
            case State::EolCommentWithinBlock:
                // This comment lasts until the end of the line.
                if (ch == '\r' || ch == '\n')
                {
                    // Stop here, leave next where it is
                    if (state == State::EolComment)
                    {
                        return true;
                    }
                    
                    // switch back to block mode
                    state = State::BracedBlock;
                    continue;
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
                    auto nextChar = next != end ? next.peek() : '\0';

                    // Check for a block comment
                    if (nextChar == '*' || nextChar == '/')
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

namespace detail
{

// Traits class to retrieve the correct iterators for a given container type
template<typename ContainerType> struct SyntaxParserTraits
{};

// Specialisation on const std::string inputs
template<>
struct SyntaxParserTraits<const std::string>
{
    struct StringIteratorAdapter
    {
        const std::string& container;
        std::string::const_iterator wrapped;

        StringIteratorAdapter(const std::string& container_, const std::string::const_iterator& iter) :
            container(container_),
            wrapped(iter)
        {}

        StringIteratorAdapter& operator++()
        {
            ++wrapped;
            return *this;
        }

        StringIteratorAdapter operator++(int) noexcept
        {
            StringIteratorAdapter temp = *this;
            ++(*this);
            return temp;
        }
        
        char operator*() const
        {
            return *wrapped;
        }

        char peek() const
        {
            if (wrapped == container.end()) return '\0';

            std::string::const_iterator copy = wrapped;
            ++copy;

            return copy != container.end() ? *copy : '\0';
        }

        bool operator==(const StringIteratorAdapter& other) const
        {
            return wrapped == other.wrapped;
        }

        bool operator!=(const StringIteratorAdapter& other) const
        {
            return !(operator==(other));
        }
    };

    typedef StringIteratorAdapter Iterator;

    static Iterator GetStartIterator(const std::string& str)
    {
        return StringIteratorAdapter(str, str.begin());
    }

    static Iterator GetEndIterator(const std::string& str)
    {
        return StringIteratorAdapter(str, str.end());
    }
};

// Specialisation on std::istream inputs
template<>
struct SyntaxParserTraits<std::istream>
{
    struct StreamIteratorAdapter
    {
        std::istream& stream;
        std::istream_iterator<char> wrapped;

        StreamIteratorAdapter(std::istream& stream_, std::istream_iterator<char> iter) :
            stream(stream_),
            wrapped(iter)
        {}

        StreamIteratorAdapter(const StreamIteratorAdapter& other) :
            stream(other.stream),
            wrapped(other.wrapped)
        {}

        StreamIteratorAdapter& operator++()
        {
            ++wrapped;
            return *this;
        }

        StreamIteratorAdapter operator++(int) noexcept
        {
            StreamIteratorAdapter temp(*this);
            ++(*this);
            return temp;
        }

        char operator*() const
        {
            return *wrapped;
        }

        char peek() const
        {
            return static_cast<char>(stream.peek());
        }

        bool operator==(const StreamIteratorAdapter& other) const
        {
            return wrapped == other.wrapped;
        }

        bool operator!=(const StreamIteratorAdapter& other) const
        {
            return !(operator==(other));
        }
    };

    typedef StreamIteratorAdapter Iterator;

    static Iterator GetStartIterator(std::istream& str)
    {
        str >> std::noskipws;
        return StreamIteratorAdapter(str, std::istream_iterator<char>(str));
    }

    static Iterator GetEndIterator(std::istream& str)
    {
        return StreamIteratorAdapter(str, std::istream_iterator<char>());
    }
};

}

/**
 * Parses and cuts decl file contents into a syntax tree.
 * Every syntax tree has a root node with 0..N children.
 */
template<typename ContainerType>
class DefBlockSyntaxParser
{
public:
    static_assert(!std::is_same_v<ContainerType, std::string>, 
        "Non-const std::string is not supported, change ContainerType to 'const std::string'");

    // Internal tokeniser and its iterator
    using Tokeniser = string::Tokeniser<DefBlockSyntaxTokeniserFunc,
        typename detail::SyntaxParserTraits<ContainerType>::Iterator, DefSyntaxToken>;

private:
    Tokeniser _tok;
    typename Tokeniser::Iterator _tokIter;

public:
    DefBlockSyntaxParser(ContainerType& str) :
        _tok(detail::SyntaxParserTraits<ContainerType>::GetStartIterator(str),
             detail::SyntaxParserTraits<ContainerType>::GetEndIterator(str), 
             DefBlockSyntaxTokeniserFunc()),
        _tokIter(_tok.getIterator())
    {}

    // Parse the text stored in the container into a def syntax tree
    // The returned syntax tree reference is never null
    DefSyntaxTree::Ptr parse()
    {
        auto syntaxTree = std::make_shared<DefSyntaxTree>();

        while (!_tokIter.isExhausted())
        {
            auto token = *_tokIter;

            switch (token.type)
            {
            case DefSyntaxToken::Type::BlockComment:
            case DefSyntaxToken::Type::EolComment:
                syntaxTree->getRoot()->appendChildNode(std::make_shared<DefCommentSyntax>(token));
                ++_tokIter;
                break;
            case DefSyntaxToken::Type::Whitespace:
                syntaxTree->getRoot()->appendChildNode(std::make_shared<DefWhitespaceSyntax>(token));
                ++_tokIter;
                break;
            case DefSyntaxToken::Type::BracedBlock:
                rWarning() << "Unnamed block encountered: " << token.value << std::endl;
                syntaxTree->getRoot()->appendChildNode(std::make_shared<DefBlockSyntax>(token, std::vector<DefSyntaxNode::Ptr>()));
                ++_tokIter;
                break;
            case DefSyntaxToken::Type::Token:
                auto blockNodes = parseBlock();

                for (auto&& node : blockNodes)
                {
                    syntaxTree->getRoot()->appendChildNode(std::move(node));
                }
                break;
            }
        }

        return syntaxTree;
    }

private:
    std::vector<DefSyntaxNode::Ptr> parseBlock()
    {
        std::vector<DefSyntaxNode::Ptr> headerNodes;

        int nameIndex = -1;
        int typeIndex = -1;

        // Check the next token
        while (!_tokIter.isExhausted())
        {
            auto token = *_tokIter++;

            switch (token.type)
            {
            case DefSyntaxToken::Type::BlockComment:
            case DefSyntaxToken::Type::EolComment:
                headerNodes.push_back(std::make_shared<DefCommentSyntax>(token));
                break;
            case DefSyntaxToken::Type::Whitespace:
                headerNodes.push_back(std::make_shared<DefWhitespaceSyntax>(token));
                break;
            case DefSyntaxToken::Type::BracedBlock:
                // The braced block token concludes this decl block
                return { std::make_shared<DefBlockSyntax>(token, std::move(headerNodes), nameIndex, typeIndex) };
            case DefSyntaxToken::Type::Token:
                if (nameIndex == -1)
                {
                    // No name yet, assume this is the name
                    nameIndex = static_cast<int>(headerNodes.size());
                    headerNodes.emplace_back(std::make_shared<DefNameSyntax>(token));
                    continue;
                }

                // We already got a first string token, check if this is the second
                if (typeIndex == -1)
                {
                    // The first one is the type, change the node type
                    typeIndex = nameIndex;
                    auto oldName = std::static_pointer_cast<DefNameSyntax>(headerNodes.at(typeIndex));
                    headerNodes.at(typeIndex) = std::make_shared<DefTypeSyntax>(oldName->getToken());

                    // Append the name to the end of the vector
                    nameIndex = static_cast<int>(headerNodes.size());
                    headerNodes.emplace_back(std::make_shared<DefNameSyntax>(token));
                    continue;
                }
                
                rWarning() << "Invalid number of decl block headers, already got a name and type: " 
                    << headerNodes.at(typeIndex)->getString() << " " 
                    << headerNodes.at(nameIndex)->getString() << std::endl;
                break;
            }
        }

        // We didn't find a well-formed block syntax node here, but we'll
        // at least return all the tokens we found on the way
        return headerNodes;
    }
};

}
