#ifndef MISSINGXMLNODEEXCEPTION_H_
#define MISSINGXMLNODEEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace shaders
{

/** Exception thrown by the Shaders plugin if the values it needs from the game
 * descriptor cannot be found.
 */

class MissingXMLNodeException
: public std::runtime_error
{
public:

	// Constructor
	MissingXMLNodeException(const std::string& what)
	: std::runtime_error(what) {}
	
};

}

#endif /*MISSINGXMLNODEEXCEPTION_H_*/
