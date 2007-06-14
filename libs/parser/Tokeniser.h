#ifndef TOKENISER_H_
#define TOKENISER_H_

#include <string>
#include <boost/tokenizer.hpp>
#include "ParseException.h"

namespace parser {

/** Base class of a tokeniser wrapping around a boost::tokeniser
 * 
 *  Standard delimiters are initialised to whitespace: " \t\n\v\r"
 */
class StringTokeniser
{
    // Internal Boost tokenizer and its iterator
    typedef boost::char_separator<char> CharSeparator;
    typedef boost::tokenizer<CharSeparator> CharTokeniser;
    
    CharSeparator _separator;
    CharTokeniser _tok;
    CharTokeniser::iterator _tokIter;

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
    StringTokeniser(const std::string& str, 
					const char* delimiters = " \t\n\v\r") :
    	_separator(delimiters),
		_tok(str, _separator),
		_tokIter(_tok.begin()) 
    {}
        
    /** Test if this StringTokeniser has more tokens to return.
     * 
     * @returns
     * true if there are further tokens, false otherwise
     */
    bool hasMoreTokens() {
        return _tokIter != _tok.end();
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
    std::string nextToken() {
        if (hasMoreTokens())
            return *(_tokIter++);
        else
            throw ParseException("Tokeniser: no more tokens");
    }
    
    /** Assert that the next token in the sequence must be equal to the provided
     * value. A ParseException is thrown if the assert fails.
     * 
     * @param val
     * The expected value of the token.
     */
	void assertNextToken(const std::string& val) {
        const std::string tok = nextToken();
        if (tok != val)
            throw ParseException("Tokeniser: Assertion failed: Required \"" 
            					 + val + "\", found \"" + tok + "\"");
    }   
    
    /** Skip the next n tokens. This method provides a convenient way to dispose
     * of a number of tokens without returning them.
     * 
     * @param n
     * The number of tokens to consume.
     */
    void skipTokens(unsigned int n) {
        for (unsigned int i = 0; i < n; i++) {
            if (hasMoreTokens()) {
                _tokIter++;
            }
            else {
                throw ParseException("Tokeniser: no more tokens");
            }
        }
    }
}; // class StringTokeniser
	
} // namespace parser

#endif /*TOKENISER_H_*/
