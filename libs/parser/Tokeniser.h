#pragma once

#include <string>
#include <iterator>
#include "string/tokeniser.h"
#include "ParseException.h"

namespace parser 
{

/**
 * greebo: Abstract type of a StringTokeniser, which splits a given
 * input string into pieces. It must provide a basic set of methods
 * to retrieve the tokens one by one.
 */
class StringTokeniser
{
public:
    /**
	 * Destructor
	 */
	virtual ~StringTokeniser() {}

	/** Test if this StringTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    virtual bool hasMoreTokens() = 0;

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
    virtual std::string nextToken() = 0;

    /** Assert that the next token in the sequence must be equal to the provided
     * value. A ParseException is thrown if the assert fails.
     *
     * @param val
     * The expected value of the token.
     */
	virtual void assertNextToken(const std::string& val) = 0;

    /** Skip the next n tokens. This method provides a convenient way to dispose
     * of a number of tokens without returning them.
     *
     * @param n
     * The number of tokens to consume.
     */
    virtual void skipTokens(unsigned int n) = 0;
};

/** Base class of a tokeniser wrapping around a string::tokeniser
 *
 *  Standard delimiters are initialised to whitespace: " \t\n\v\r"
 */
template<typename ContainerT>
class BasicStringTokeniser :
	public StringTokeniser
{
private:
    // Internal tokenizer helper
    typedef string::CharTokeniserFunc CharSeparator;
    typedef string::Tokeniser<CharSeparator> CharTokeniser;

	CharSeparator _separator;
    CharTokeniser _tok;
    CharTokeniser::Iterator _tokIter;

public:
    /** Construct a Tokeniser with the given input string, and optionally
     * a list of separators.
     *
     * @param str
     * The string to tokenise.
     *
     * @param delims
     * The list of characters to use as delimiters.
     *
     * @param keptDelims
     * The list of delimiters to keep
     */
    BasicStringTokeniser(const ContainerT& str,
						 const char* delimiters = " \t\n\v\r") :
    	_separator(delimiters),
		_tok(str, _separator),
		_tokIter(_tok.getIterator())
    {}

    /** Test if this StringTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    bool hasMoreTokens() override
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

        throw ParseException("Tokeniser: no more tokens");
    }

    /** Assert that the next token in the sequence must be equal to the provided
     * value. A ParseException is thrown if the assert fails.
     *
     * @param val
     * The expected value of the token.
     */
	void assertNextToken(const std::string& val) override
	{
        const std::string tok = nextToken();

        if (tok != val) throw ParseException("Tokeniser: Assertion failed: Required \"" + 
			val + "\", found \"" + tok + "\"");
    }

    /** Skip the next n tokens. This method provides a convenient way to dispose
     * of a number of tokens without returning them.
     *
     * @param n
     * The number of tokens to consume.
     */
    void skipTokens(unsigned int n) override
	{
        for (unsigned int i = 0; i < n; i++)
		{
            if (hasMoreTokens())
			{
                _tokIter++;
				continue;
            }
            
			throw ParseException("Tokeniser: no more tokens");
        }
    }
}; // class BasicStringTokeniser

/**
 * Specialisation of BasicStringTokeniser to work with std::istream objects. This is
 * needed because an std::istream does not provide begin() and end() methods
 * to get an iterator, but needs a separate istream_iterator<> to be constructed
 * for it.
 */
template<>
class BasicStringTokeniser<std::istream> :
    public StringTokeniser
{
private:
    // Istream iterator type
    typedef std::istream_iterator<char> CharStreamIterator;

    // Internal tokenizer helper
    typedef string::CharTokeniserFunc CharSeparator;
    typedef string::Tokeniser<CharSeparator, CharStreamIterator> CharTokeniser;

    CharSeparator _separator;
    CharTokeniser _tok;
    CharTokeniser::Iterator _tokIter;

    // Helper function to set noskipws on the input stream.
    static std::istream& setNoskipws(std::istream& is)
    {
        is >> std::noskipws;
        return is;
    }

public:
    /** Construct a Tokeniser with the given input string, and optionally
     * a list of separators.
     *
     * @param str
     * The string to tokenise.
     *
     * @param delims
     * The list of characters to use as delimiters.
     *
     * @param keptDelims
     * The list of delimiters to keep
     */
    BasicStringTokeniser(std::istream& str,
        const char* delimiters = " \t\n\v\r") :
        _separator(delimiters),
        _tok(CharStreamIterator(setNoskipws(str)), CharStreamIterator(), _separator),
        _tokIter(_tok.getIterator())
    {}

    /** Test if this StringTokeniser has more tokens to return.
     *
     * @returns
     * true if there are further tokens, false otherwise
     */
    bool hasMoreTokens() override
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

        throw ParseException("Tokeniser: no more tokens");
    }

    /** Assert that the next token in the sequence must be equal to the provided
     * value. A ParseException is thrown if the assert fails.
     *
     * @param val
     * The expected value of the token.
     */
    void assertNextToken(const std::string& val) override
    {
        const std::string tok = nextToken();

        if (tok != val) throw ParseException("Tokeniser: Assertion failed: Required \"" +
            val + "\", found \"" + tok + "\"");
    }

    /** Skip the next n tokens. This method provides a convenient way to dispose
     * of a number of tokens without returning them.
     *
     * @param n
     * The number of tokens to consume.
     */
    void skipTokens(unsigned int n) override
    {
        for (unsigned int i = 0; i < n; i++)
        {
            if (hasMoreTokens())
            {
                _tokIter++;
                continue;
            }

            throw ParseException("Tokeniser: no more tokens");
        }
    }
}; // class BasicStringTokeniser

} // namespace parser
