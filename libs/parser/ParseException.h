#pragma once

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
