#ifndef STREAMTOKENISER_H_
#define STREAMTOKENISER_H_

#include "itextstream.h"

#include <string>
#include <sstream>


/** Split an input stream into tokens.
 * 
 * This class provides functionality to split an input stream into tokens, separated
 * by whitespace. It provides a simple interface to check if there is another token, 
 * and to retrieve the next token and update the internal state.
 */

class StreamTokeniser
{
private:

    // The string to tokenise
    std::string _tokStr;

public:

    /** Construct a StreamTokenizer to retrieve tokens from the provided input stream.
     * 
     * @param stream
     * The existing input stream to tokenise.
     */

	StreamTokeniser(TextInputStream& stream)
    {
        // Read the entire TextInputStream into a buffer and store the
        // resulting string.
        std::stringstream temp;
        char buf[1024];

        std::size_t numRead;
        while ((numRead = stream.read(buf, 1024)) > 0)
            temp.write(buf, numRead);
            
        _tokStr = temp.str();
    }    


    /** Check whether the StreamTokeniser has any more tokens to return.
     * 
     * @return
     * true if there are more tokens to return, false otherwise.
     */
     
    bool hasMoreTokens();
    

    /** Return the next token in the stream.
     * 
     * @return
     * An std::string containing the next token in the stream
     */
     
    std::string getToken();

};

#endif /*STREAMTOKENISER_H_*/
