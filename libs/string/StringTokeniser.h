#ifndef STRINGTOKENISER_H_
#define STRINGTOKENISER_H_

#include <string>

#include <boost/tokenizer.hpp>

namespace util {

/** Split a string into tokens.
 * 
 * This class provides a similar interface to Java's StringTokenizer class. It accepts
 * and std::string and a list of separators, and provides a simple interface to return
 * the next token in the string.
 */

class StringTokeniser
{

    // The string to tokenise
    const std::string& _tokStr;
    
    // List of delimiters
    const char* _delims;

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
    : _tokStr(str), _delims(seps) {
    }
        
};

} // namespace util

#endif /*STRINGTOKENISER_H_*/
