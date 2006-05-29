#ifndef INVALIDNODEEXCEPTION_H_
#define INVALIDNODEEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace xml
{

// Exception to indicate an invalid Node was passed to a function.

class InvalidNodeException:
    public std::runtime_error
{
public:

    // Constructor. Must initialise the parent.
    InvalidNodeException(const std::string& what):
        std::runtime_error(what) {}
        
};

}

#endif /*INVALIDNODEEXCEPTION_H_*/
