#ifndef MISSINGXMLNODEEXCEPTION_H_
#define MISSINGXMLNODEEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace xml
{

/**
 * Exception thrown by modules if the values it needs
 * from the registry cannot be found.
 */
class MissingXMLNodeException :
	public std::runtime_error
{
public:
	// Constructor
	MissingXMLNodeException(const std::string& what) :
		std::runtime_error(what)
	{}
};

} // namespace xml

#endif
