#ifndef _MATH_INTERFACE_H_
#define _MATH_INTERFACE_H_

#include "iscript.h"
#include "math/FloatTools.h"
#include "math/Vector3.h"
#include <map>

#include <boost/python.hpp>

namespace script {

// ========== Math objects ==========

class MathInterface :
	public IScriptInterface
{
public:
	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace) {
		// Add the Vector3 class
		double& (Vector3::*fx)() = &Vector3::x;

		nspace["Vector3"] = boost::python::class_<Vector3>("Vector3", boost::python::init<double, double, double>())
			.def(boost::python::init<const Vector3&>())
			// greebo: Pick the correct overload - this is hard to read, but it is necessary
			.def("x", static_cast<double& (Vector3::*)()>(&Vector3::x), boost::python::return_value_policy<boost::python::copy_non_const_reference>())
			.def("y", static_cast<double& (Vector3::*)()>(&Vector3::y), boost::python::return_value_policy<boost::python::copy_non_const_reference>())
			.def("z", static_cast<double& (Vector3::*)()>(&Vector3::z), boost::python::return_value_policy<boost::python::copy_non_const_reference>())
			.def("getLength", &Vector3::getLength)
			.def("getLengthSquared", &Vector3::getLengthSquared)
			.def("getNormalised", &Vector3::getNormalised)
			.def("normalise", &Vector3::normalise)
			.def("getInversed", &Vector3::getInversed)
			.def("dot", &Vector3::dot<double>)
			.def("angle", &Vector3::angle<double>)
			.def("crossProduct", &Vector3::crossProduct<double>)
			.def("max", &Vector3::max)
			.def("min", &Vector3::min)
			.def("isParallel", &Vector3::isParallel<double>)
		;
	}
};
typedef boost::shared_ptr<MathInterface> MathInterfacePtr;

} // namespace script

#endif /* _MATH_INTERFACE_H_ */
