#ifndef _COMMAND_TOKENISER_H_
#define _COMMAND_TOKENISER_H_

#include "parser/Tokeniser.h"

namespace cmd {

class CommandTokeniserFunc
{
    // Enumeration of states
    enum States {
        SEARCHING,        // haven't found anything yet
        TOKEN_STARTED,    // found the start of a possible multi-char token
        DOUBLEQUOTE,      // inside double-quoted text, no tokenising
		SINGLEQUOTE,	  // Inside single-quoted text, no tokenising
    } _state;

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
    CommandTokeniserFunc(const char* delims) :
		_state(SEARCHING),
		_delims(delims)
    {}

    /* REQUIRED. Operator() is called by the boost::tokenizer. This function
     * must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true.
     */
    template<typename InputIterator, typename Token>
    bool operator() (InputIterator& next, InputIterator end, Token& tok) {

        // Initialise state, no persistence between calls
        _state = SEARCHING;

        // Clear out the token, no guarantee that it is empty
        tok = "";

        while (next != end) {

            switch (_state) {

                case SEARCHING:

                    // If we have a delimiter, just advance to the next character
                    if (isDelim(*next)) {
                        ++next;
                        continue;
                    }

					// If we have a KEPT delimiter, this is the token to return.
                    if (*next == ';') {
                        tok = *(next++);
                        return true;
                    }

                    // Otherwise fall through into TOKEN_STARTED, saving the state for the
                    // next iteration
                    _state = TOKEN_STARTED;

                case TOKEN_STARTED:

                    // Here a delimiter indicates a successful token match
                    if (isDelim(*next) || *next == ';') {
                        return true;
                    }

                    // Now next is pointing at a non-delimiter. Switch on this
                    // character.
                    switch (*next) {

                        // Found a quote, enter DOUBLEQUOTE state, or return the
                        // current token if we are in the process of building
                        // one.
                        case '\"':
                            if (tok != "") {
                                return true;
                            }
                            else {
                                _state = DOUBLEQUOTE;
                                ++next;
                                continue; // skip the quote
                            }

                        // Found a quote, enter SINGLEQUOTE state, or return the
                        // current token if we are in the process of building
                        // one.
                        case '\'':
							if (tok != "") {
                                return true;
                            }
                            else {
                                _state = SINGLEQUOTE;
                                ++next;
                                continue; // skip the quote
                            }

                        // General case. Token lasts until next delimiter.
                        default:
                            tok += *next;
                            ++next;
                            continue;
                    }

                case DOUBLEQUOTE:
					// In the quoted state, just advance until the closing
                    // quote. No delimiter splitting is required.
                    if (*next == '\"') {
                        ++next;
						return true;
                    }
                    else {
                        tok += *next;
                        ++next;
                        continue;
                    }

				case SINGLEQUOTE:
					// In the quoted state, just advance until the closing
                    // quote. No delimiter splitting is required.
                    if (*next == '\'') {
                        ++next;
						return true;
                    }
                    else {
                        tok += *next;
                        ++next;
                        continue;
                    }
            } // end of state switch
        } // end of for loop

        // Return true if we have added anything to the token
        return (tok != "");
    }

    // REQUIRED. Reset function to clear internal state
    void reset() {
        _state = SEARCHING;
    }

};

/**
 * greebo: A Command Tokeniser splits the given input strings into
 * pieces, delimited by whitespace characters. The tokeniser is respecting
 * quoted content, which will be treated as one string token (excluding the actual quotes).
 */
class CommandTokeniser :
	public parser::StringTokeniser
{
	typedef boost::tokenizer<CommandTokeniserFunc> Tokeniser;

    Tokeniser _tok;
    Tokeniser::iterator _tokIter;

public:
	CommandTokeniser(const std::string& str) :
		_tok(str, CommandTokeniserFunc(" \n\t\v\r")),
		_tokIter(_tok.begin())
	{}

	// Documentation: see base class
	bool hasMoreTokens() {
        return _tokIter != _tok.end();
    }

	// Documentation: see base class
	std::string nextToken() {
		if (hasMoreTokens()) {
            return *(_tokIter++);
		}
		else {
            throw parser::ParseException("CommandTokeniser: no more tokens");
		}
    }

	void assertNextToken(const std::string& val) {
        const std::string tok = nextToken();
        if (tok != val)
            throw parser::ParseException("CommandTokeniser: Assertion failed: Required \""
            					 + val + "\", found \"" + tok + "\"");
    }

	void skipTokens(unsigned int n) {
        for (unsigned int i = 0; i < n; i++) {
            if (hasMoreTokens()) {
                _tokIter++;
            }
            else {
				throw parser::ParseException("CommandTokeniser: no more tokens");
            }
        }
    }
};

} // namespace cmd

#endif /* _COMMAND_TOKENISER_H_ */
