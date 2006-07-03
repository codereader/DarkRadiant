#ifndef STRINGTOKENISER_H_
#define STRINGTOKENISER_H_

#include <string>
#include <iostream>

#include <boost/tokenizer.hpp>

namespace util {

/* Model of boost::TokenizerFunction which splits tokens on whitespace with additional
 * protection of quoted content.
 */

class QuotedTokeniserFunc {
    
    // List of delimiters to skip
    const char* _delims;
    
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
    
public:

    // Constructor
    QuotedTokeniserFunc(const char* delims = " \t\n")
    : _delims(delims)
    {}

    /* REQUIRED. Operator() is called by the boost::tokenizer. This function
     * must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true.
     */
    template<typename InputIterator, typename Token>
    bool operator() (InputIterator& next, InputIterator end, Token& tok) {
        // Clear out the token, no guarantee that it is empty
        tok = "";
        // Skip any preceding delimiters
        while (next != end && isDelim(*next))
            next++;
        // Return false if we didn't find anything
        if (next == end)
            return false;
        // Now next is pointing at a non-delimiter. Switch based on whether this
        // is a quote or not.
        if (*next == '\"') {
            while (next++ != end && *next != '\"')
                tok += *next;
            // Advance past the final quote, unless we hit the end rather than
            // a close quote.
            if (next != end)
                next++;
            return true;
        }
        else { // no quote
            while (next != end && !isDelim(*next)) {
                tok += *(next++);
            }
            return true;
        }
    }
    
    // REQUIRED. Reset function to clear internal state
    void reset() {}
    
};


/** Split a string into tokens.
 * 
 * This class provides a similar interface to Java's StringTokenizer class. It accepts
 * and std::string and a list of separators, and provides a simple interface to return
 * the next token in the string.
 */

class StringTokeniser
{

    // Internal Boost tokenizer and its iterator
    typedef boost::tokenizer<QuotedTokeniserFunc> CharTokeniser;
    CharTokeniser _tok;
    CharTokeniser::iterator _tokIter;

public:

    /** Construct a StringTokeniser with the given input string, and optionally
     * a list of separators.
     * 
     * @param str
     * The string to tokenise.
     * 
     * @param seps
     * The list of characters to use as delimiters.
     */

    StringTokeniser(const std::string& str, const char* seps = " \t\n")
    : _tok(str, QuotedTokeniserFunc()),
      _tokIter(_tok.begin()) 
    {
    }
        
        
    /** Test if this StringTokeniser has more tokens to return.
     * 
     * @returns
     * true if there are further tokens, false otherwise
     */
     
    bool hasMoreTokens() {
        return _tokIter != _tok.end();
    }


    /** Return the next token in the sequence.
     * 
     * @returns
     * std::string containing the next token in the sequence.
     * 
     * @pre
     * hasMoreTokens() must be true, otherwise the result is undefined
     */
     
    const std::string nextToken() {
        return *(_tokIter++);
    }   
};

} // namespace util

#endif /*STRINGTOKENISER_H_*/
