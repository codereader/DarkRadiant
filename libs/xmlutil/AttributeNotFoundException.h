#ifndef ATTRIBUTENOTFOUNDEXCEPTION_H_
#define ATTRIBUTENOTFOUNDEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace xml
{

// Exception to indicate an invalid Node was passed to a function.

class AttributeNotFoundException:
    public std::runtime_error
{
public:

    // Constructor. Must initialise the parent.
    AttributeNotFoundException(const std::string& what):
        std::runtime_error(what) {}
        
};

}

#endif /*ATTRIBUTENOTFOUNDEXCEPTION_H_*/
