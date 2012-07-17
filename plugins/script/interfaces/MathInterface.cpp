#include "MathInterface.h"

#include "math/FloatTools.h"
#include "math/AABB.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "render/Vertex3f.h"

namespace script {

// IScriptInterface implementation
void MathInterface::registerInterface(boost::python::object& nspace) {
	// Add the Vector3 class
	nspace["Vector3"] = boost::python::class_<Vector3>("Vector3", boost::python::init<double, double, double>())
		.def(boost::python::init<const Vector3&>())
		// greebo: Pick the correct overload - this is hard to read, but it is necessary
		.def("x", static_cast<double& (Vector3::*)()>(&Vector3::x),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("y", static_cast<double& (Vector3::*)()>(&Vector3::y),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("z", static_cast<double& (Vector3::*)()>(&Vector3::z),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
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

	// Register Vertex3f, which extends Vector3
	nspace["Vertex3f"] = boost::python::class_<Vertex3f,
		boost::python::bases<Vector3> >("Vertex3f", boost::python::init<>() )
		.def(boost::python::init<const Vector3&>())
		.def(boost::python::init<double, double, double>())
	;

	// Add the Vector2 class
	nspace["Vector2"] = boost::python::class_<Vector2>("Vector2", boost::python::init<double, double>())
		.def(boost::python::init<const Vector2&>())
		// greebo: Pick the correct overload - this is hard to read, but it is necessary
		.def("x", static_cast<double& (Vector2::*)()>(&Vector2::x),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("y", static_cast<double& (Vector2::*)()>(&Vector2::y),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
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

	// Add the Vector4 class
	boost::python::class_<Vector4> vector4Decl("Vector4", boost::python::init<double, double, double, double>());

	vector4Decl.def(boost::python::init<const Vector4&>())
		// greebo: Pick the correct overload - this is hard to read, but it is necessary
		.def("x", static_cast<double& (Vector4::*)()>(&Vector4::x),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("y", static_cast<double& (Vector4::*)()>(&Vector4::y),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("z", static_cast<double& (Vector4::*)()>(&Vector4::z),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("w", static_cast<double& (Vector4::*)()>(&Vector4::w),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("getVector3", static_cast<Vector3& (Vector4::*)()>(&Vector4::getVector3),
			boost::python::return_value_policy<boost::python::copy_non_const_reference>())
		.def("getProjected", &Vector4::getProjected)
		.def("dot", &Vector4::dot<double>)
		// Most important operators
		.def(boost::python::self + boost::python::self)		// __add__
		.def(boost::python::self - boost::python::self)		// __sub__
		.def(boost::python::self += boost::python::self)
		.def(boost::python::self -= boost::python::self)
	;

	// Add the Vector4 and Quaternion with the same interface
	nspace["Vector4"] = nspace["Quaternion"] = vector4Decl;

	// Declare AABB to python
	nspace["AABB"] = boost::python::class_<AABB>("AABB", boost::python::init<>())
		.def(boost::python::init<const Vector3&, const Vector3&>())
		.def_readwrite("origin", &AABB::origin)
		.def_readwrite("extents", &AABB::extents)
		.def("isValid", &AABB::isValid)
		.def("getRadius", &AABB::getRadius)
		.def("includePoint", &AABB::includePoint)
		.def("includeAABB", &AABB::includeAABB)
	;
}

} // namespace script
