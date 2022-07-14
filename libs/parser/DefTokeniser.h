#pragma once

#include "ParseException.h"

#include <iterator>
#include <iostream>
#include <ios>
#include <string>
#include "string/tokeniser.h"

namespace parser
{

/* TokenizerFunction which splits tokens on whitespace with additional
 * protection of quoted content.
 */

class DefTokeniserFunc 
{
    // Enumeration of states
    enum {
        SEARCHING,        // haven't found anything yet
        TOKEN_STARTED,    // found the start of a possible multi-char token
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

    // Test if a character is a delimiter
    bool isDelim(char c) {
        const char* curDelim = _delims;
        while (*curDelim != 0) {
            if (*(curDelim++) == c) {
                return true;
            }
        }
        return false;
    }

    // Test if a character is a kept delimiter
    bool isKeptDelim(char c) {
        const char* curDelim = _keptDelims;
        while (*curDelim != 0) {
            if (*(curDelim++) == c) {
                return true;
            }
        }
        return false;
    }


public:

    // Constructor
    DefTokeniserFunc(const char* delims, const char* keptDelims)
    : _state(SEARCHING), _delims(delims), _keptDelims(keptDelims)
    {}

    /* REQUIRED. Operator() is called by the tokeniser. This function
     * must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true.
     */
    template<typename InputIterator>
    bool operator() (InputIterator& next, const InputIterator& end, std::string& tok)
	{
        // Initialise state, no persistence between calls
        _state = SEARCHING;

        // Clear out the token, no guarantee that it is empty
        tok = "";

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

                    // If we have a KEPT delimiter, this is the token to return.
                    if (isKeptDelim(*next)) {
                        tok = *(next++);
                        return true;
                    }

                    // Otherwise fall through into TOKEN_STARTED, saving the state for the
                    // next iteration
                    _state = TOKEN_STARTED;

                case TOKEN_STARTED:

                    // Here a delimiter indicates a successful token match
                    if (isDelim(*next) || isKeptDelim(*next)) {
                        return true;
                    }

                    // Now next is pointing at a non-delimiter. Switch on this
                    // character.
                    switch (*next) {

                        // Found a quote, enter QUOTED state, or return the
                        // current token if we are in the process of building
                        // one.
                        case '\"':
                            if (tok != "") {
                                return true;
                            }
                            else {
                                _state = QUOTED;
                                ++next;
                                continue; // skip the quote
                            }

                        // Found a slash, possibly start of comment
                        case '/':
                            _state = FORWARDSLASH;
                            ++next;
                            continue; // skip slash, will need to add it back if this is not a comment

                        // General case. Token lasts until next delimiter.
                        default:
                            tok += *next;
                            ++next;
                            continue;
                    }

                case QUOTED:

                    // In the quoted state, just advance until the closing
                    // quote. No delimiter splitting is required.
                    if (*next == '\"') {
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
							else if (*next == '"') // Quote
							{
								tok += '"';
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

					if (*next == '\"') {
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

                    switch (*next) {

                        case '*':
                            _state = COMMENT_DELIM;
                            ++next;
                            continue;

                        case '/':
                            _state = COMMENT_EOL;
                            ++next;
                            continue;

                        default: // false alarm, add the slash and carry on
							// greebo: switch to TOKEN_STARTED, the SEARCHING state might 
							// overwrite this if the next token is a kept delimiter
                            _state = TOKEN_STARTED; 
                            tok += "/";
                            // Do not increment next here
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

                    if (*next == '\r' || *next == '\n')
					{
						++next;

						// If we have a token at this point, return it
						if (tok != "") 
						{
							return true;
						}
						else
						{
							_state = SEARCHING;
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
                        ++next;

						// If we have a token at this point, return it
						if (tok != "") 
						{
							return true;
						}
						else
						{
							_state = SEARCHING;
							continue;
						}

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
        // If we ran out of tokens after the closing quote, even an empty string is a valid token
        return tok != "" || _state == AFTER_CLOSING_QUOTE;
    }
};

constexpr const char* const WHITESPACE = " \t\n\v\r";

/**
 * DefTokeniser abstract base class. This class provides a unified interface to
 * DefTokeniser subclasses, so that calling code can retrieve a stream of tokens
 * without needing knowledge of the underlying container type (std::string,
 * std::istream etc).
 *
 * Each subclass MUST implement hasMoreTokens() and nextToken() appropriately,
 * while default implementations of assertNextToken() and skipTokens() are
 * provided that make use of the former two methods.
 */
class DefTokeniser
{
public:
    /**
	 * Destructor
	 */
	virtual ~DefTokeniser() {}

    /**
     * Test if this DefTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    virtual bool hasMoreTokens() const = 0;

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
     virtual std::string nextToken() = 0;

    /**
     * Assert that the next token in the sequence must be equal to the provided
     * value. A ParseException is thrown if the assert fails.
     *
     * @param val
     * The expected value of the token.
     */
    virtual void assertNextToken(const std::string& val)
	{
        const std::string tok = nextToken();
        if (tok != val)
            throw ParseException("DefTokeniser: Assertion failed: Required \""
            					 + val + "\", found \"" + tok + "\"");
    }

    /**
     * Skip the next n tokens. This method provides a convenient way to dispose
     * of a number of tokens without returning them.
     *
     * @param n
     * The number of tokens to consume.
     */
    virtual void skipTokens(unsigned int n) 
	{
        for (unsigned int i = 0; i < n; i++) 
		{
        	nextToken();
        }
    }

	/**
	 * Returns the next token without incrementing the internal
	 * iterator. Use this if you want to take a look at what is coming
	 * next without actually changing the tokeniser's state.
	 */
	virtual std::string peek() const = 0;
};

/**
 * Tokenise a DEF file.
 *
 * This class provides a similar interface to Java's StringTokenizer class. It accepts
 * and std::string and a list of separators, and provides a simple interface to return
 * the next token in the string. It also protects quoted content and ignores both C and
 * C++ style comments.
 */
template<typename ContainerT>
class BasicDefTokeniser : 
	public DefTokeniser
{
    // Internal tokenizer and its iterator
    typedef string::Tokeniser<DefTokeniserFunc> CharTokeniser;
    CharTokeniser _tok;
    CharTokeniser::Iterator _tokIter;

public:

    /**
     * Construct a DefTokeniser with the given input type, and optionally
     * a list of separators.
     *
     * @param str
     * The container to tokenise.
     *
     * @param delims
     * The list of characters to use as delimiters.
     *
     * @param keptDelims
     * String of characters to treat as delimiters but return as tokens in their
     * own right.
     */
    BasicDefTokeniser(const ContainerT& str,
                      const char* delims = WHITESPACE,
                      const char* keptDelims = "{}()")
    : _tok(str, DefTokeniserFunc(delims, keptDelims)),
      _tokIter(_tok.getIterator())
    { }

    /** Test if this StringTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    bool hasMoreTokens() const override
	{
        return !_tokIter.isExhausted();
    }

    /** Return the next token in the sequence. This function consumes
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

        throw ParseException("DefTokeniser: no more tokens");
    }

	/**
	 * Returns the next token without incrementing the internal
	 * iterator. Use this if you want to take a look at what is coming
	 * next without actually changing the tokeniser's state.
	 */
	std::string peek() const override
	{
		if (hasMoreTokens())
		{
            return *_tokIter;
		}
        
		throw ParseException("DefTokeniser: no more tokens");
	}
};

/**
 * Specialisation of DefTokeniser to work with std::istream objects. This is
 * needed because an std::istream does not provide begin() and end() methods
 * to get an iterator, but needs a separate istream_iterator<> to be constructed
 * for it.
 */
template<>
class BasicDefTokeniser<std::istream> : 
	public DefTokeniser
{
private:
    // Istream iterator type
    typedef std::istream_iterator<char> CharStreamIterator;

    // Internal tokenizer and its iterator
    typedef string::Tokeniser<DefTokeniserFunc, CharStreamIterator> CharTokeniser;
    CharTokeniser _tok;
    CharTokeniser::Iterator _tokIter;

	// Helper function to set noskipws on the input stream.
	static std::istream& setNoskipws(std::istream& is)
	{
		is >> std::noskipws;
		return is;
	}

public:

    /**
     * Construct a DefTokeniser with the given input stream, and optionally
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
     */
    BasicDefTokeniser(std::istream& str,
                      const char* delims = WHITESPACE,
                      const char* keptDelims = "{}(),")
    : _tok(CharStreamIterator(setNoskipws(str)), // start iterator
           CharStreamIterator(), // end (null) iterator
           DefTokeniserFunc(delims, keptDelims)),
      _tokIter(_tok.getIterator())
    { }

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
        
		throw ParseException("DefTokeniser: no more tokens");
    }

	/**
	 * Returns the next token without incrementing the internal
	 * iterator. Use this if you want to take a look at what is coming
	 * next without actually changing the tokeniser's state.
	 */
	std::string peek() const override
	{
		if (hasMoreTokens())
		{
            return *_tokIter;
		}
        
		throw ParseException("DefTokeniser: no more tokens");
	}
};

} // namespace parser
