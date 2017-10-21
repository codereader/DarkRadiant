#pragma once

#include <string>
#include <cassert>

namespace string
{

/**
* Tokeniser class similar to the one shipping with boost. It needs a TokeniserFunc type
* as template argument which it uses to split the target string into pieces.
* Use the getIterator() method to retrieve an Iterator object which can be used to
* retrieve the resulting tokens and checking for input exhaustion.
*
* The TokeniserFunc is a function object matching the following signature:
* 
* template<typename InputIterator, typename TokenType>
* bool (InputIterator& next, const InputIterator& end, TokenType& tok);
*
* TokeniserFunc returns true if a non-empty token could be stored into "tok".
* "next" is non-const is advanced by the TokeniserFunc as it parses the characters.
* "end" indicates the end of the input sequence.
* The InputIterator type usually refers to a std::string::const_iterator or similar,
* dereferencing it will retrieve a char value.
* 
* See CharTokeniserFunc in string/tokeniser.h for an example.
*/
template<typename TokeniserFunc, 
		 typename InputIterator = std::string::const_iterator,
		 typename TokenType = std::string>
class Tokeniser
{
protected:
	// The iterators defining the range to tokenise
	InputIterator _begin;
	InputIterator _end;
	TokeniserFunc _func;

public:
	// Public type for use by client code, to retrieve tokens
	class Iterator
	{
	private:
		TokeniserFunc _tokeniserFunc;
		InputIterator _cur;
		InputIterator _end;

		TokenType _token;
		bool _hasValidToken; // true if we're not exhausted yet

	public:
		Iterator(const TokeniserFunc& tokeniserFunc, const InputIterator& begin, const InputIterator& end) :
			_tokeniserFunc(tokeniserFunc),
			_cur(begin),
			_end(end),
			_hasValidToken(false)
		{
			// Initialise our local token by the first call to the tokenisation function
			advance();
		}

		// Call this to check whether the iterator can deliver at least one more token.
		// Returns true if no more tokens are available.
		bool isExhausted() const
		{
			return !_hasValidToken;
		}

		// Retrieves the current token - only call this if isExhausted == false
		const TokenType& operator*() const
		{
			assert(!isExhausted());
			return _token;
		}

		// Advances to the next token. Only call this when not exhausted
		Iterator& operator++()
		{
			assert(!isExhausted());
			advance();
			return *this;
		}

		// Advances to the next token. Only call this when not exhausted
		Iterator operator++(int) // postfix-increment
		{
			assert(!isExhausted());

			Iterator prev(*this);
			advance();
			return prev;
		}

	private:
		void advance()
		{
			// Call the tokenisation function and let it store the characters
			// into our local _token member. Store the func result bool
			_hasValidToken = _tokeniserFunc(_cur, _end, _token);
		}
	};

	// Construct this tokeniser on top of the given iterator range [begin..end), 
	// using the given TokeniserFunc to split it into tokens.
	Tokeniser(const InputIterator& begin, const InputIterator& end, const TokeniserFunc& func = TokeniserFunc()) :
		_begin(begin),
		_end(end),
		_func(func)
	{}

	// Construct this tokeniser on top of the given string, using the given TokeniserFunc
	// to split it into tokens.
	Tokeniser(const std::string& string, const TokeniserFunc& func = TokeniserFunc()) :
		Tokeniser(string.begin(), string.end(), func)
	{}

	// Returns an Iterator for use by client code, pointing
	// to the first token extacted from the target range
	Iterator getIterator()
	{
		return Iterator(_func, _begin, _end);
	}
};

class CharTokeniserFunc 
{
	// List of delimiters
	const char* _delims;

	// Test if a character is a delimiter
	bool isDelim(char c)
	{
		const char* curDelim = _delims;
		while (*curDelim != 0) {
			if (*(curDelim++) == c) {
				return true;
			}
		}
		return false;
	}

public:
	// Constructor
	CharTokeniserFunc(const char* delims) : 
		_delims(delims)
	{}

	/* REQUIRED. Operator() is called by the string::tokeniser. This function
	* must search for a token between the two iterators next and end, and if
	* a token is found, set tok to the token, set next to position to start
	* parsing on the next call, and return true.
	*/
	template<typename InputIterator>
	bool operator() (InputIterator& next, const InputIterator& end, std::string& tok)
	{
		// Clear out the token, no guarantee that it is empty
		tok.clear();

		while (next != end)
		{
			if (isDelim(*next))
			{
				// Are we still searching for a non-delimiter?
				if (tok.empty())
				{
					++next;
					continue;
				}

				// hit a delim, token filled => success
				return true; 
			}

			// Next is pointing at a non-delimiter, add to token
			tok += *next++;
		}

		// Return true if we have added anything to the token
		return !tok.empty();
	}
};

}
