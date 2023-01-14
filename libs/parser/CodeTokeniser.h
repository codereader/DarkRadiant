#pragma once

#include "iarchive.h"
#include "ifilesystem.h"
#include "DefTokeniser.h"

#include <list>
#include <map>
#include <algorithm>
#include <fmt/format.h>

#include "string/trim.h"
#include "string/predicate.h"
#include "Tokeniser.h"

namespace parser
{

// Code tokeniser function, with special treatment for #define statements
class CodeTokeniserFunc
{
    // Enumeration of states
    enum
	{
        SEARCHING,        // haven't found anything yet
        TOKEN_STARTED,    // found the start of a possible multi-char token
        OPERATOR,         // found the start of an operator
		AFTER_DEFINE,	  // after parsing a #define command
		AFTER_DEFINE_BACKSLASH,		// after a #define token, encountering a backslash
		AFTER_DEFINE_SEARCHING_FOR_EOL,	// after a #define token, after encountering a backslash
		AFTER_DEFINE_FORWARDSLASH,	// after parsing when encountering a forward slash
        QUOTED,         // inside quoted text, no tokenising
		AFTER_CLOSING_QUOTE, // right after a quoted text, checking for backslash
		SEARCHING_FOR_QUOTE, // searching for continuation of quoted string (after a backslash was found)
        FORWARDSLASH,   // forward slash found, possible comment coming
        COMMENT_EOL,    // double-forwardslash comment
        COMMENT_DELIM,  // inside delimited comment (/*)
        STAR            // asterisk, possibly indicates end of comment (*/)
    } _state;

    // List of delimiters to skip
    const char* _delims;

    // List of delimiters to keep
    const char* _keptDelims;

    // List of recognised operator tokens
    const std::vector<std::string>& _operators;

    // Test if a character is a delimiter
    bool isDelim(char c) const
    {
        const char* curDelim = _delims;
        while (*curDelim != 0) {
            if (*(curDelim++) == c) {
                return true;
            }
        }
        return false;
    }

    // Test if a character is a kept delimiter
    bool isKeptDelim(char c) const
    {
        const char* curDelim = _keptDelims;
        while (*curDelim != 0) {
            if (*(curDelim++) == c) {
                return true;
            }
        }
        return false;
    }

    bool isMatchingOperatorByFirstCharacter(char c) const
    {
        for (const auto& op : _operators)
        {
            if (op.at(0) == c)
            {
                return true;
            }
        }

        return false;
    }


public:

    CodeTokeniserFunc(const char* delims, const char* keptDelims, const std::vector<std::string>& operators) :
		_state(SEARCHING),
		_delims(delims),
		_keptDelims(keptDelims),
        _operators(operators)
    {}

    /* REQUIRED. Operator() is called by the Tokeniser. This function
     * must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true.
     */
    template<typename InputIterator, typename Token>
    bool operator() (InputIterator& next, InputIterator end, Token& tok)
    {
        // Initialise state, no persistence between calls
        _state = SEARCHING;

        // Clear out the token, no guarantee that it is empty
        tok.clear();

		enum class QuoteType
		{
			Single = 0,
			Double = 1,
		};

		auto quoteType = QuoteType::Single;

        while (next != end)
		{
            switch (_state)
			{
                case SEARCHING:

                    // If we have a delimiter, just advance to the next character
                    if (isDelim(*next)) {
                        ++next;
                        continue;
                    }

                    if (*next == '/')
                    {
                        _state = FORWARDSLASH;
                        ++next;
                        continue; // skip slash, will need to add it back if this is not a comment)
                    }

                    // If we have a KEPT delimiter, this is the token to return.
                    if (isKeptDelim(*next)) {
                        tok = *next++;
                        return true;
                    }

                    if (isMatchingOperatorByFirstCharacter(*next))
                    {
                        // Found the first character of one of our supported operators
                        tok = *next++;
                        _state = OPERATOR;
                        continue;
                    }

                    // Otherwise fall through into TOKEN_STARTED, saving the state for the
                    // next iteration
                    _state = TOKEN_STARTED;

                case TOKEN_STARTED:

                    // Here a delimiter indicates a successful token match
                    if (isDelim(*next) || isKeptDelim(*next))
					{
						// Check the token for possible preprocessor #define
						if (tok == "#define")
						{
							_state = AFTER_DEFINE;
							continue;
						}

                        return true;
                    }

                    // Check for operators, they basically act as delimiters
                    if (isMatchingOperatorByFirstCharacter(*next))
                    {
                        // Return what we have so far, we'll deal with the operator next round
                        return true;
                    }

                    // Now next is pointing at a non-delimiter. Switch on this
                    // character.
                    switch (*next) 
					{
                        // Found a quote, enter QUOTED state, or return the
                        // current token if we are in the process of building
                        // one.
                        case '\"':
                            if (!tok.empty()) return true;
                            
                            quoteType = QuoteType::Double;
                            _state = QUOTED;
                            ++next;
                            continue; // skip the quote

						case '\'':
                            if (!tok.empty()) return true;

							quoteType = QuoteType::Single;
							_state = QUOTED;
							++next;
							continue; // skip the quote

                        // Found a slash, possibly start of comment
                        case '/':
                            _state = FORWARDSLASH;
                            ++next;
                            continue; // skip slash, will need to add it back if this is not a comment

                        // General case. Token lasts until next delimiter.
                        default:
                            tok += *next++;
                            continue;
                    }

                    case OPERATOR:
                    // We already matched the first character of one of our operators
                    // Check if we can match a second character
                    {
                        const std::string* singleCharacterOperator = nullptr;
                        auto lastChar = tok.at(tok.length() - 1);

                        for (const auto& op : _operators)
                        {
                            if (op.length() == 2 && op.at(0) == lastChar && op.at(1) == *next)
                            {
                                // Exact two-char operator match, exhaust character and return
                                tok += *next++;
                                return true;
                            }

                            // Check if a single-character operator is matching
                            // We will resort to this one if nothing else matches
                            if (op.length() == 1 && op.at(0) == lastChar)
                            {
                                singleCharacterOperator = &op;
                            }
                        }

                        // No two-character operator matched, did we find a single-character operator?
                        if (singleCharacterOperator != nullptr)
                        {
                            // Don't exhaust the second character, return the operator as token
                            return true;
                        }

                        // No operator found, switch back to token mode and re-check (don't exhaust)
                        _state = TOKEN_STARTED;
                        continue;
                    }

				case AFTER_DEFINE:
					// Collect token until EOL is found
					if (*next == '\r' || *next == '\n')
					{
                        _state = SEARCHING;
                        ++next;
                        return true;
                    }
					else if (*next == '\\')
					{
						// Found a backslash, this can be used to connect lines
						_state = AFTER_DEFINE_BACKSLASH;
						++next;
						continue;
					}
					else if (*next == '/')
					{
						// This could be a (line) comment starting here
						_state = AFTER_DEFINE_FORWARDSLASH;
						++next;
						continue;
					}
                    else {
						tok += *next;
                        ++next;
                        continue; // do nothing
                    }
				case AFTER_DEFINE_BACKSLASH:
					// Skip delimiters
					if (!isDelim(*next))
					{
						// False alarm, not a delimiter after the backslash
						tok += '\\';
						tok += *next;

						_state = AFTER_DEFINE;
						++next;
						continue;
					}

					// Skip delimiters until next line break
					_state = AFTER_DEFINE_SEARCHING_FOR_EOL;

					// FALL THROUGH to AFTER_DEFINE_SEARCHING_FOR_EOL

				case AFTER_DEFINE_SEARCHING_FOR_EOL:
					// Search for EOL
					if (*next == '\r' || *next == '\n')
					{
						tok += '\n'; // add the line break to the token
						_state = AFTER_DEFINE;
					}

					++next;
					continue;

				case AFTER_DEFINE_FORWARDSLASH:
					if (*next == '/')
					{
						// This is the second forward slash, we're in line comment mode now
						_state = COMMENT_EOL;
						++next;
						continue;
					}
					else if (*next == '*')
					{
						// This is a star, we're in block comment mode now
						_state = COMMENT_DELIM;
						++next;
						continue;
					}
					else
					{
						// False alarm, add the first slash and this character
						tok += '/';

						// Switch back to DEFINE mode
						_state = AFTER_DEFINE;
						continue;
					}

                case QUOTED:

                    // In the quoted state, just advance until the closing
                    // quote. No delimiter splitting is required.
                    if ((*next == '"' && quoteType == QuoteType::Double) ||
						(*next == '\'' && quoteType == QuoteType::Single))
					{
                        ++next;

						// greebo: We've found a closing quote, but there might be a backslash indicating
						// a multi-line string constant "" \ "", so switch to AFTER_CLOSING_QUOTE mode
						_state = AFTER_CLOSING_QUOTE;
                        continue;
                    }
                    else if (*next == '\\')
					{
						// Escape found, check next character
						++next;

						if (next != end)
						{
							if (*next == 'n') // Linebreak
							{
								tok += '\n';
							}
							else if (*next == 't') // Tab
							{
								tok += '\t';
							}
							else if (*next == '"' && quoteType == QuoteType::Double) // Escape Double Quote
							{
								tok += '"';
							}
							else if (*next == '\'' && quoteType == QuoteType::Single) // Escaped Single Quote
							{
								tok += '\'';
							}
							else
							{
								// No special escape sequence, add the backslash
								tok += '\\';
								// Plus the character itself
								tok += *next;
							}

							++next;
						}

						continue;
					}
					else
					{
                        tok += *next;
                        ++next;
                        continue;
                    }

				case AFTER_CLOSING_QUOTE:
					// We already have a valid string token in our hands, but it might be continued
					// if one of the next tokens is a backslash

					if (*next == '\\') {
						// We've found a backslash right after a closing quote, this indicates we could
						// proceed with parsing quoted content
						++next;
						_state = SEARCHING_FOR_QUOTE;
						continue;
					}

					// Ignore delimiters
					if (isDelim(*next)) {
                        ++next;
                        continue;
                    }

					// Everything except delimiters and backslashes indicates that
					// the quoted content is not continued, so break the loop.
					// This returns the token and parsing continues.
					// Return TRUE in any case, even if the parsed token is empty ("").
					return true;

				case SEARCHING_FOR_QUOTE:
					// We have found a backslash after a closing quote, search for an opening quote

					// Step over delimiters
					if (isDelim(*next)) {
                        ++next;
                        continue;
                    }

					if ((*next == '\"' && quoteType == QuoteType::Double) ||
						(*next == '\'' && quoteType == QuoteType::Single))
					{
						// Found the desired opening quote, switch to QUOTED
						++next;
						_state = QUOTED;
						continue;
					}

					// Everything except delimiters or opening quotes indicates an error
					throw ParseException("Could not find opening double quote after backslash.");

                case FORWARDSLASH:

                    // If we have a forward slash we may be entering a comment. The forward slash
                    // will NOT YET have been added to the token, so we must add it manually if
                    // this proves not to be a comment.
                    switch (*next)
                    {
                    case '*':
                        _state = COMMENT_DELIM;
                        ++next;
                        continue;

                    case '/':
                        _state = COMMENT_EOL;
                        ++next;
                        continue;

                    default:
                        // Not a comment, add to token nonetheless
                        // Do not increment next, we're already past the slash
                        tok += "/";

                        // Check if the slash is a starting character of any operator,
                        // if yes we switch to OPERATOR mode
                        _state = isMatchingOperatorByFirstCharacter('/') ? OPERATOR : SEARCHING;
                        continue;
                    }

                case COMMENT_DELIM:

                    // Inside a delimited comment, we add nothing to the token but check for
                    // the "*/" sequence.

                    if (*next == '*') {
                        _state = STAR;
                        ++next;
                        continue;
                    }
                    else {
                        ++next;
                        continue; // ignore and carry on
                    }

                case COMMENT_EOL:

                    // This comment lasts until the end of the line.

                    if (*next == '\r' || *next == '\n') {
                        _state = SEARCHING;
                        ++next;

						// If we have a token after a line comment, return it
						if (tok != "")
						{
							return true;
						}
						else
						{
							continue;
						}
                    }
                    else {
                        ++next;
                        continue; // do nothing
                    }

                case STAR:

                    // The star may indicate the end of a delimited comment.
                    // This state will only be entered if we are inside a
                    // delimited comment.

                    if (*next == '/') {
                    	// End of comment
                        _state = SEARCHING;
                        ++next;
                        continue;
                    }
                    else if (*next == '*') {
                    	// Another star, remain in the STAR state in case we
                    	// have a "**/" end of comment.
                    	_state = STAR;
                    	++next;
                    	continue;
                    }
                    else {
                    	// No end of comment
                    	_state = COMMENT_DELIM;
                    	++next;
                        continue;
                    }

            } // end of state switch
        } // end of for loop

        // Return true if we have added anything to the token
        return !tok.empty();
    }
};

// Represents a #DEFINE'd macro in a code file, which may have 0 to n arguments
// #define MYDEF( y ) set "cmd" y;
class Macro
{
public:
	Macro()
	{}

	Macro(const std::string& name_) :
		name(name_)
	{}

	// Name of the #DEFINE'd macro
	std::string name;

	// The arguments of this macro
	std::list<std::string> arguments;

	// The macro body
	std::list<std::string> tokens;
};

class SingleCodeFileTokeniser :
	public DefTokeniser
{
private:
    // Istream iterator type
    typedef std::istream_iterator<char> CharStreamIterator;

    // Internal tokeniser and its iterator
    typedef string::Tokeniser<CodeTokeniserFunc, CharStreamIterator> CharTokeniser;
    CharTokeniser _tok;
    CharTokeniser::Iterator _tokIter;

private:

	// Helper function to set noskipws on the input stream.
	static std::istream& setNoskipws(std::istream& is)
	{
		is >> std::noskipws;
		return is;
	}

public:

    /**
     * Construct a SingleCodeFileTokeniser with the given input stream, and optionally
     * a list of separators.
     *
     * @param str
     * The std::istream to tokenise. This is a non-const parameter, since tokens
     * will be extracted from the stream.
     *
     * @param delims
     * The list of characters to use as delimiters.
     *
     * @param keptDelims
     * String of characters to treat as delimiters but return as tokens in their
     * own right.
     *
     * @param operators
     * List of recognised operator tokens, like "+=", "/" and "?"
     */
    SingleCodeFileTokeniser(std::istream& str,
                      const char* delims,
                      const char* keptDelims,
                      const std::vector<std::string>& operators)
    : _tok(CharStreamIterator(setNoskipws(str)), // start iterator
           CharStreamIterator(), // end (null) iterator
           CodeTokeniserFunc(delims, keptDelims, operators)),
      _tokIter(_tok.getIterator())
    {}

    /**
     * Test if this StringTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    bool hasMoreTokens() const override
	{
		return !_tokIter.isExhausted();
    }

    /**
     * Return the next token in the sequence. This function consumes
     * the returned token and advances the internal state to the following
     * token.
     *
     * @returns
     * std::string containing the next token in the sequence.
     *
     * @pre
     * hasMoreTokens() must be true, otherwise an exception will be thrown.
     */
    std::string nextToken() override
	{
		if (hasMoreTokens())
		{
			return *(_tokIter++);
		}
        
		throw ParseException("SingleCodeFileTokeniser: no more tokens");
    }

	std::string peek() const override
	{
		if (hasMoreTokens())
		{
            return *_tokIter;
		}
        
		throw ParseException("SingleCodeFileTokeniser: no more tokens");
	}
};

/**
 * High-level tokeniser taking a specific VFS file as input.
 * It is able to handle preprocessor statements like #include
 * by maintaining several child tokenisers. This can be used
 * to parse code-like files as Doom 3 Scripts or GUIs.
 *
 * Note: Don't expect this tokeniser to be particularly fast.
 */
class CodeTokeniser :
	public DefTokeniser
{
private:

	struct ParseNode
	{
        using Ptr = std::shared_ptr<ParseNode>;

		ArchiveTextFilePtr archive;
		std::istream inputStream;
		SingleCodeFileTokeniser tokeniser;

		ParseNode(const ArchiveTextFilePtr& archive_,
				const char* delims, const char* keptDelims, const std::vector<std::string>& operators) :
			archive(archive_),
			inputStream(&archive->getInputStream()),
			tokeniser(inputStream, delims, keptDelims, operators)
		{}
	};

	// The stack of child tokenisers
    using NodeList = std::list<ParseNode::Ptr>;
	NodeList _nodes;

	NodeList::iterator _curNode;

	// A set of visited files to catch infinite include loops
    std::list<std::string> _fileStack;

	using StringList = std::list<std::string>;

	// A map associating names to #define'd macros
    std::map<std::string, Macro> _macros;

	// A small local buffer which is needed to properly resolve #define statements
	// which could consist of several tokens themselves
	StringList _tokenBuffer;

	const char* _delims;
	const char* _keptDelims;

    // The list of operators supported by this tokeniser
    std::vector<std::string> _operators;

public:
    constexpr static const char* KEPT_DELIMS = "{}(),;";

    /**
     * Construct a CodeTokeniser with the given text file from the VFS.
     */
    CodeTokeniser(const ArchiveTextFilePtr& file,
                  const char* delims,
                  const char* keptDelims,
                  const std::vector<const char*>& operators) :
        _delims(delims),
        _keptDelims(keptDelims),
        _operators(operators.begin(), operators.end())
    {
		_nodes.emplace_back(std::make_shared<ParseNode>(file, _delims, _keptDelims, _operators));
		_curNode = _nodes.begin();

		_fileStack.push_back(file->getName());

		fillTokenBuffer();
	}

    bool hasMoreTokens() const override
	{
		return !_tokenBuffer.empty();
    }

    std::string nextToken() override
	{
		if (_tokenBuffer.empty())
		{
			throw ParseException("No more tokens.");
		}

		std::string temp = _tokenBuffer.front();
		_tokenBuffer.pop_front();

		// Keep the buffer filled
		if (_tokenBuffer.empty())
		{
			fillTokenBuffer();
		}

		return temp;
    }

	std::string peek() const override
	{
		if (_tokenBuffer.empty())
		{
			throw ParseException("No more tokens.");
		}

		return _tokenBuffer.front();
    }

private:
	void fillTokenBuffer()
	{
		while (_curNode != _nodes.end())
		{
			if (!(*_curNode)->tokeniser.hasMoreTokens())
			{
				_fileStack.pop_back();
				++_curNode;
				continue;
			}

			std::string token = (*_curNode)->tokeniser.nextToken();

			// Don't treat #strNNNN as preprocessor tokens
			if (!token.empty() &&
				token[0] == '#' &&
				!string::starts_with(token, "#str"))
			{
				// A pre-processor token is ahead
				handlePreprocessorToken(token);
				continue;
			}

			_tokenBuffer.push_front(token);

			// Found a non-preprocessor token,
			// check if this is matching a preprocessor definition
			auto found = _macros.find(_tokenBuffer.front());

			if (found != _macros.end())
			{
				// Expand this macro, new tokens are acquired from the currently active tokeniser
				auto expanded = expandMacro(found->second, [this]() { return (*_curNode)->tokeniser.nextToken(); });

				if (!expanded.empty())
				{
					// Replace the token in the buffer with the expanded string
					_tokenBuffer.pop_front();
					_tokenBuffer.insert(_tokenBuffer.begin(), expanded.begin(), expanded.end());
				}
			}

			return; // got a token
		}
	}

	// Expands the given macro
	StringList expandMacro(const Macro& macro, const std::function<std::string()>& nextTokenFunc)
	{
        // A list of tokens that are fill in for each macro argument
        std::vector<StringList> argumentValues;
        argumentValues.resize(macro.arguments.size());

		// Acquire the macro argument values if applicable
		if (!macro.arguments.empty())
		{
			// Assert an opening parenthesis
			if (nextTokenFunc() != "(")
			{
				throw ParseException(fmt::format("Error expanding macro {0}, expected '('", macro.name));
			}

            auto currentArgumentValues = argumentValues.begin();
            for (auto arg : macro.arguments)
            {
                auto argumentToken = nextTokenFunc();

                // Accumulate the macro arguments, comma-separated, until we hit the closing parenthesis
                while (argumentToken != "," && argumentToken != ")")
                {
                    currentArgumentValues->emplace_back(std::move(argumentToken));
                    argumentToken = nextTokenFunc();
                }

                ++currentArgumentValues;
            }
		}

		// Allocate a new list for the expanded tokens
        auto macroTokens = macro.tokens;

		// Process the macro tokens expanding sub-macros while iterating
		for (auto t = macroTokens.begin(); t != macroTokens.end();)
		{
			// Replace any macro identifier with the set of values
			auto tokens = getMacroTokens(*t, macro, argumentValues);

            // Insert all replaced tokens at this point in the list
            t = macroTokens.erase(t);
            t = macroTokens.insert(t, tokens.begin(), tokens.end());

            auto found = _macros.find(*t);

            if (found == _macros.end())
            {
                ++t;
                continue; // leave this token unchanged
            }

            // Remove the macro identifier, get its expansion
            t = macroTokens.erase(t);

            // Enter recursion to expand this sub-macro, new tokens are acquired from the current iterator t
            auto subMacro = expandMacro(found->second, [&]()
            {
                if (t == macroTokens.end())
                {
                    throw ParseException(fmt::format("Running out of tokens expanding sub-macro {0}", *t));
                }

                // Extract a new piece from the macroTokens and deliver this
                auto subTokens = getMacroTokens(*t, macro, argumentValues);;

                // Before returning the token, expand any placeholders
                t = macroTokens.erase(t);
                t = macroTokens.insert(t, subTokens.begin(), subTokens.end());

                // Take and remove the token from the list and deliver it
                auto token = *t;
                t = macroTokens.erase(t);

                return token;
            });

            if (!subMacro.empty())
            {
                // Insert the expanded macro contents
                t = macroTokens.insert(t, subMacro.begin(), subMacro.end());
            }
            else
            {
                rWarning() << "Macro expansion yields empty token list: " << *t <<
                    " in " << (*_curNode)->archive->getName() << std::endl;
            }
		}

		return macroTokens;
	}

    // Check if the current token is referring to a macro argument and replace it with its value tokens
    static StringList getMacroTokens(std::string token, const Macro& macro, const std::vector<StringList>& argumentValues)
    {
        auto values = argumentValues.begin();

		for (auto arg = macro.arguments.begin();
			arg != macro.arguments.end() && values != argumentValues.end(); ++arg, ++values)
		{
			if (token == *arg)
			{
				return *values;
			}
		}

        return { token }; // leave token unchanged
    }

	void handlePreprocessorToken(const std::string& token)
	{
		if (token == "#include")
		{
			auto includeFile = (*_curNode)->tokeniser.nextToken();
			auto file = GlobalFileSystem().openTextFile(includeFile);

			if (file)
			{
				// Catch infinite recursions
				auto found = std::find(_fileStack.begin(), _fileStack.end(), file->getName());

				if (found == _fileStack.end())
				{
					// Push a new parse node and switch
					_fileStack.push_back(file->getName());

					_curNode = _nodes.insert(
						_curNode,
						std::make_shared<ParseNode>(file, _delims, _keptDelims, _operators)
					);
				}
				else
				{
					rError() << "Caught infinite loop on parsing #include token: "
						<< includeFile << " in " << (*_curNode)->archive->getName() << std::endl;
				}
			}
			else
			{
				rWarning() << "Couldn't find include file: "
					<< includeFile << " in " << (*_curNode)->archive->getName() << std::endl;
			}
		}
		else if (string::starts_with(token, "#define"))
		{
			parseMacro(token);
		}
		else if (token == "#undef")
		{
			auto key = (*_curNode)->tokeniser.nextToken();
			_macros.erase(key);
		}
		else if (token == "#ifdef")
		{
            auto key = (*_curNode)->tokeniser.nextToken();
            auto found = _macros.find(key);

			if (found == _macros.end())
			{
				skipInactivePreprocessorBlock();
			}
		}
		else if (token == "#ifndef")
		{
            auto found = _macros.find((*_curNode)->tokeniser.nextToken());

			if (found != _macros.end())
			{
				skipInactivePreprocessorBlock();
			}
		}
		else if (token == "#else")
		{
			// We have an #else during active parsing, an inactive preprocessor block is ahead
			skipInactivePreprocessorBlock();
		}
		else if (token == "#if")
		{
			(*_curNode)->tokeniser.skipTokens(1);
		}
	}

	void parseMacro(const std::string& token)
	{
		std::string defineToken = token;

		if (defineToken.length() <= 7)
		{
			rWarning() << "Invalid #define statement in " << (*_curNode)->archive->getName() << std::endl;
			return;
		}

		// Replace tabs with spaces
		std::replace(defineToken.begin(), defineToken.end(), '\t', ' ');

		// Cut off the "#define " (including space)
		defineToken = defineToken.substr(8);

		// Parse the entire macro
		std::istringstream macroStream(defineToken);
		SingleCodeFileTokeniser macroParser(macroStream, _delims, _keptDelims, _operators);

		auto name = macroParser.nextToken();

		bool paramsStarted = false;

		if (string::ends_with(name, "("))
		{
			string::trim_right(name, "(");
			paramsStarted = true;
		}

		auto result = _macros.emplace(name, Macro(name));

		if (!result.second)
		{
			rWarning() << "Redefinition of " << name << " in " << (*_curNode)->archive->getName() << std::endl;
			result.first->second = Macro(name);
		}

		auto& macro = result.first->second;

		while (macroParser.hasMoreTokens())
		{
			auto macroToken = macroParser.nextToken();

			// An opening parenthesis might be an argument list, but
			// only if we're still at the beginning of the macro
			if (macroToken == "(" && !paramsStarted && macro.tokens.empty())
			{	
				paramsStarted = true;
			}
			else if (macroToken == ")" && paramsStarted)
			{
				paramsStarted = false;
			}
			else if (macroToken == ",")
			{
				if (paramsStarted)
				{
					continue;
				}

				// Treat the comma as part of the macro value
				macro.tokens.push_back(macroToken);
			}
			else 
			{
				if (paramsStarted)
				{
					// Token is an argument
					macro.arguments.push_back(macroToken);
				}
				else
				{
					// Ordinary macro value
					macro.tokens.push_back(macroToken);
				}
			}
		}
	}

	void skipInactivePreprocessorBlock()
	{
		// Not defined, skip everything until matching #endif
		for (std::size_t level = 1; level > 0;)
		{
			if (!(*_curNode)->tokeniser.hasMoreTokens())
			{
				rWarning() << "No matching #endif for #if(n)def in "
					<< (*_curNode)->archive->getName() << std::endl;
			}

			auto token = (*_curNode)->tokeniser.nextToken();

			if (token == "#endif")
			{
				level--;
			}
			else if (token == "#ifdef" || token == "#ifndef" || token == "#if")
			{
				level++;
			}
			else if (token == "#else" && level == 1)
			{
				// Matching #else, break the loop
				break;
			}
		}
	}
};

} // namespace parser
