#ifndef _MATH_INTERFACE_H_
#define _MATH_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

namespace script {

// ========== Math objects ==========

class MathInterface :
	public IScriptInterface
{
public:
	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<MathInterface> MathInterfacePtr;

} // namespace script

#endif /* _MATH_INTERFACE_H_ */
