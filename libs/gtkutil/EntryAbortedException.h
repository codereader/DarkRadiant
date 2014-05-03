#pragma once

#include <stdexcept>
#include <string>

namespace wxutil
{

/* Exception class raised when a text entry dialog is cancelled or destroyed.
 */

class EntryAbortedException: public std::runtime_error
{
public:
    EntryAbortedException(const std::string& what):
        std::runtime_error(what) {
    }
};

}

namespace gtkutil
{

/* Exception class raised when a text entry dialog is cancelled or destroyed.
 */

class EntryAbortedException: public std::runtime_error
{
public:
    EntryAbortedException(const std::string& what):
        std::runtime_error(what) {
    }
};

}
