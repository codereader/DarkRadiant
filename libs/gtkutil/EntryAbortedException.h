#ifndef ENTRYABORTEDEXCEPTION_H_
#define ENTRYABORTEDEXCEPTION_H_

#include <stdexcept>
#include <string>

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

#endif /*ENTRYABORTEDEXCEPTION_H_*/
