#include <stdexcept>

namespace objectives {

/**
 * Exception thrown when a critical error occurs in the dialog.
 */
class ObjectivesException :
	public std::runtime_error
{
public:
	ObjectivesException(const std::string& what) :
		std::runtime_error(what)
	{}
};

} // namespace objectives
