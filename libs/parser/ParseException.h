#ifndef PARSEEXCEPTION_H_
#define PARSEEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace parser
{

/** Exception raised by parser classes when an unrecoverable error occurs.
 */

class ParseException
: public std::runtime_error 
{
public:
    ParseException(const std::string& what)
    : std::runtime_error(what) 
    {}
};

}

#endif /*PARSEEXCEPTION_H_*/
