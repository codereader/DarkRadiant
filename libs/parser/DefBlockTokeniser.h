#ifndef DEF_BLOCK_TOKENISER_H_
#define DEF_BLOCK_TOKENISER_H_

#include "ParseException.h"

#include <string>
#include <ctype.h>
#include <boost/tokenizer.hpp>

namespace parser {

/**
 * Abstract base class BlockTokeniser. This class inspects a given input block
 * or stream and returns definition blocks (including name). 
 * 
 * C and C++-style comments are properly ignored.
 */
class BlockTokeniser {
public:

	struct Block {
		// The name of this block
		std::string name;

		// The block contents (excluding braces)
		std::string contents;

		void clear() {
			name.clear();
			contents.clear();
		}
	};
	
    /** 
     * Test if this DefTokeniser has more blocks to return.
     * 
     * @returns
     * true if there are further blocks, false otherwise
     */
    virtual bool hasMoreBlocks() = 0;

    /** 
     * Return the next block in the sequence. This function consumes
     * the returned block and advances the internal state to the following
     * block.
     * 
     * @returns
     * A named Block structure.
     * 
     * @pre
     * hasMoreBlocks() must be true, otherwise an exception will be thrown.
     */
     virtual Block nextBlock() = 0;
};

/* Model of boost::TokenizerFunction which splits tokens on whitespace with additional
 * protection of quoted content.
 */

class DefBlockTokeniserFunc {
    
    // Enumeration of states
    enum State {
        SEARCHING_NAME,	  // haven't found anything yet
		TOKEN_STARTED,	  // first non-delimiter character found
        BLOCK_NAME,       // found the start of a possible multi-char token
		SEARCHING_BLOCK,  // searching for block opening char
		BLOCK_CONTENT,	  // within a block
        FORWARDSLASH,     // forward slash found, possible comment coming
        COMMENT_EOL,      // double-forwardslash comment
        COMMENT_DELIM,    // inside delimited comment (/*)
        STAR              // asterisk, possibly indicates end of comment (*/)
    } _state;
       
	const char* _delims;			// whitespace

	const char _blockStartChar;	// "{"
	const char _blockEndChar;		// "}"

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
    DefBlockTokeniserFunc(const char* delims, char blockStartChar, char blockEndChar) : 
		_state(SEARCHING_NAME),
		_delims(delims),
		_blockStartChar(blockStartChar),
		_blockEndChar(blockEndChar)
    {}

    /* REQUIRED. Operator() is called by the boost::tokenizer. This function
     * must search for a token between the two iterators next and end, and if
     * a token is found, set tok to the token, set next to position to start
     * parsing on the next call, and return true.
     */
    template<typename InputIterator, typename Token>
    bool operator() (InputIterator& next, InputIterator end, Token& tok) {
        // Initialise state, no persistence between calls
        _state = SEARCHING_NAME;

        // Clear out the token, no guarantee that it is empty
		tok.clear();

		char ch = '\0';
		std::size_t blockLevel = 0;

		while (next != end) {

			ch = *next;

			switch (_state) {

                case SEARCHING_NAME:
					// Ignore delimiters
					if (isDelim(ch)) {
						++next;
						continue;
					}

                    // We have a non-delimiter, examine it
					_state = TOKEN_STARTED;
					// Fall through

				case TOKEN_STARTED:
					// Here a delimiter indicates a successful token match
                    if (isDelim(ch)) {
                        _state = SEARCHING_BLOCK;
						continue;
                    }

                    // The character is pointing at a non-delimiter. Switch on it.
                    switch (ch) {
                        // Found a slash, possibly start of comment
                        case '/':
                            _state = FORWARDSLASH;
                            ++next;
                            continue; // skip slash, will need to add it back if this is not a comment
            
                        // General case. Token lasts until next delimiter.
                        default:
                            tok.name += ch;
                            ++next;
                            continue;
                    }
					break;

				case SEARCHING_BLOCK:
					if (isDelim(ch)) {
						++next; // keep on searching
						continue;
					}
					else if (ch == _blockStartChar) {
						// Found an opening brace
						_state = BLOCK_CONTENT;
						blockLevel++;
						++next;
						continue;
					}
					else if (ch == '/') {
						// Forward slash, possible comment start
						_state = FORWARDSLASH;
						++next;
						continue;
					}
					else {
						// Not a delimiter, not an opening brace, must be 
						// an "extension" for the name
						tok.name += ' ';
						tok.name += ch;

						// Switch back to name
						_state = TOKEN_STARTED;
						++next;
						continue;
					}

				case BLOCK_CONTENT:
					// Check for another opening brace
					if (ch == _blockEndChar) {
						blockLevel--;

						if (blockLevel == 0) {
							// End of block content, we're done here,
							// don't add this last character either
							++next;
							return true;
						}
						else {
							// Still within a block, add to contents
							tok.contents += ch;
							++next;
							continue;
						}
					}
					else if (ch == _blockStartChar) {
						// another block within this block, ignore this
						blockLevel++;
						tok.contents += ch;
						++next;
						continue;
					}
					else {
						tok.contents += ch;
						++next;
						continue;
					}

				case FORWARDSLASH:
                
                    // If we have a forward slash we may be entering a comment. The forward slash
                    // will NOT YET have been added to the token, so we must add it manually if
                    // this proves not to be a comment.
                    
                    switch (ch) {
                        case '*':
                            _state = COMMENT_DELIM;
                            ++next;
                            continue;
                            
                        case '/':
                            _state = COMMENT_EOL;
                            ++next;
                            continue;
                            
                        default: // false alarm, add the slash and carry on
                            _state = TOKEN_STARTED;
                            tok.name += '/';
                            // Do not increment next here
                            continue;
                    }
                    
                case COMMENT_DELIM:
                    // Inside a delimited comment, we add nothing to the token but check for
                    // the "*/" sequence.
                    if (ch == '*') {
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
                    if (ch == '\r' || ch == '\n') {
						// An EOL comment with non-empty name means searching for block
						_state = (tok.name.empty()) ? SEARCHING_NAME : SEARCHING_BLOCK;
                        ++next;
                        continue;
                    }
                    else {
                        ++next;
                        continue; // do nothing
                    }
                    
                case STAR:
                    // The star may indicate the end of a delimited comment. 
                    // This state will only be entered if we are inside a 
                    // delimited comment.
                    if (ch == '/') {
                    	// End of comment
                        _state = (tok.name.empty()) ? SEARCHING_NAME : SEARCHING_BLOCK;
                        ++next;
                        continue;
                    }
                    else if (ch == '*') {
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
				}
			}

        // Return true if we have found a named block
        if (!tok.name.empty())
            return true;
        else
            return false;
    }
    
    // REQUIRED. Reset function to clear internal state
    void reset() {
        _state = SEARCHING_NAME;
    }
};

/** 
 * Tokenise a DEF file.
 * 
 * This class provides a similar interface to Java's StringTokenizer class. It accepts
 * an input stream and provides a simple interface to return the next block in the stream. 
 * It also protects quoted content and ignores both C and C++ style comments.
 */
template<typename ContainerT>
class BasicDefBlockTokeniser : 
	public BlockTokeniser
{
    // Internal Boost tokenizer and its iterator
	typedef boost::tokenizer<DefBlockTokeniserFunc, 
							  std::string::const_iterator, 
							  BlockTokeniser::Block> Tokeniser;

    Tokeniser _tok;
    Tokeniser::iterator _tokIter;

public:

    /** 
     * Construct a BasicDefBlockTokeniser with the given input type.
     * 
     * @param str
     * The container to tokenise.
     */
    BasicDefBlockTokeniser(const ContainerT& str,
						   const char* delims = " \t\n\v\r",
						   const char blockStartChar = '{',
						   const char blockEndChar = '}') : 
		_tok(str, DefBlockTokeniserFunc(delims, blockStartChar, blockEndChar)),
		_tokIter(_tok.begin())
    {}
        
    /** Test if this StringTokeniser has more blocks to return.
     * 
     * @returns
     * true if there are further blocks, false otherwise
     */
    bool hasMoreBlocks() {
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
    Block nextBlock() {
        if (hasMoreBlocks())
            return *(_tokIter++);
        else
            throw ParseException("BlockTokeniser: no more blocks");
    }
};

/**
 * Specialisation of DefTokeniser to work with std::istream objects. This is
 * needed because an std::istream does not provide begin() and end() methods
 * to get an iterator, but needs a separate istream_iterator<> to be constructed
 * for it.
 */
template<>
class BasicDefBlockTokeniser<std::istream> : 
	public BlockTokeniser
{
    // Istream iterator type
    typedef std::istream_iterator<char> CharStreamIterator;
    
    // Internal Boost tokenizer and its iterator
    typedef boost::tokenizer<DefBlockTokeniserFunc,
                             CharStreamIterator,
							 BlockTokeniser::Block> Tokeniser;

    Tokeniser _tok;
    Tokeniser::iterator _tokIter;

private:
	
	// Helper function to set noskipws on the input stream.
	static std::istream& setNoskipws(std::istream& is) {
		is >> std::noskipws;
		return is;
	}
    
public:

    /** 
     * Construct a BasicDefBlockTokeniser with the given input stream.
     * 
     * @param str
     * The std::istream to tokenise. This is a non-const parameter, since tokens
     * will be extracted from the stream.
     */
	BasicDefBlockTokeniser(std::istream& str, 
						   const char* delims = " \t\n\v\r",
						   const char blockStartChar = '{',
						   const char blockEndChar = '}') :
		_tok(CharStreamIterator(setNoskipws(str)), // start iterator
			 CharStreamIterator(), // end (null) iterator
			 DefBlockTokeniserFunc(delims, blockStartChar, blockEndChar)),
		_tokIter(_tok.begin())
	{}
        
    /** 
     * Test if this BlockTokeniser has more blocks to return.
     * 
     * @returns
     * true if there are further blocks, false otherwise
     */
    bool hasMoreBlocks() {
        return _tokIter != _tok.end();
    }

    /** 
     * Return the next block in the sequence. This function consumes
     * the returned block and advances the internal state to the following
     * block.
     * 
     * @returns
     * The named block.
     * 
     * @pre
     * hasMoreBlocks() must be true, otherwise an exception will be thrown.
     */
    Block nextBlock() {
        if (hasMoreBlocks())
            return *(_tokIter++);
        else
            throw ParseException("BlockTokeniser: no more tokens");
    }
};

} // namespace parser

#endif /* DEF_BLOCK_TOKENISER_H_ */
