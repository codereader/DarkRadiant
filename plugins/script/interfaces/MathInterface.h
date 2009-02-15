#ifndef _MATH_INTERFACE_H_
#define _MATH_INTERFACE_H_

#include "iscript.h"
#include "math/FloatTools.h"
#include "math/Vector2.h"
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
			// Most important operators
			.def(boost::python::self + boost::python::self)		// __add__
			.def(boost::python::self - boost::python::self)		// __sub__
			.def(boost::python::self += boost::python::self)
			.def(boost::python::self -= boost::python::self)
			.def(boost::python::self < boost::python::self);	// __lt__
		;

		// Add the Vector2 class
		nspace["Vector2"] = boost::python::class_<Vector2>("Vector2", boost::python::init<double, double>())
			.def(boost::python::init<const Vector2&>())
			// greebo: Pick the correct overload - this is hard to read, but it is necessary
			.def("x", static_cast<double& (Vector2::*)()>(&Vector2::x), boost::python::return_value_policy<boost::python::copy_non_const_reference>())
			.def("y", static_cast<double& (Vector2::*)()>(&Vector2::y), boost::python::return_value_policy<boost::python::copy_non_const_reference>())
			.def("getLength", &Vector2::getLength)
			.def("getLengthSquared", &Vector2::getLengthSquared)
			.def("dot", &Vector2::dot<double>)
			.def("crossProduct", &Vector2::crossProduct<double>)
			// Most important operators
			.def(boost::python::self + boost::python::self)		// __add__
			.def(boost::python::self - boost::python::self)		// __sub__
			.def(boost::python::self += boost::python::self)
			.def(boost::python::self -= boost::python::self)
			.def(boost::python::self < boost::python::self);	// __lt__
		;
	}
};
typedef boost::shared_ptr<MathInterface> MathInterfacePtr;

} // namespace script

#endif /* _MATH_INTERFACE_H_ */
