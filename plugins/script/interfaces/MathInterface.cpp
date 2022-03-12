#include "MathInterface.h"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

#include "math/FloatTools.h"
#include "math/AABB.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "render/Vertex3f.h"
#include "string/convert.h"

namespace script {

// IScriptInterface implementation
void MathInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the Vector3 class
	py::class_<Vector3> vec3(scope, "Vector3");
	vec3.def(py::init<double, double, double>());
	vec3.def(py::init<const Vector3&>());

	// greebo: Pick the correct overload - this is hard to read, but it is necessary
	vec3.def("x", static_cast<double& (Vector3::*)()>(&Vector3::x), py::return_value_policy::reference);
	vec3.def("y", static_cast<double& (Vector3::*)()>(&Vector3::y), py::return_value_policy::reference);
	vec3.def("z", static_cast<double& (Vector3::*)()>(&Vector3::z), py::return_value_policy::reference);
	vec3.def("getLength", &Vector3::getLength);
	vec3.def("getLengthSquared", &Vector3::getLengthSquared);
	vec3.def("getNormalised", &Vector3::getNormalised);
	vec3.def("normalise", &Vector3::normalise);
	vec3.def("dot", &Vector3::dot);
	vec3.def("angle", &Vector3::angle);
	vec3.def("crossProduct", &Vector3::cross);
	// Most important operators
	vec3.def(py::self + py::self);		// __add__
	vec3.def(py::self - py::self);		// __sub__
	vec3.def(py::self += py::self);		// __iadd__
	vec3.def(py::self -= py::self);		// __isub__
	vec3.def(py::self < py::self);	// __lt__
	vec3.def("__repr__", [](const Vector3& vec)
	{
		return "(" + string::to_string(vec.x()) + " " + string::to_string(vec.y()) +
			" " + string::to_string(vec.z()) + ")";
	});

	// Register Vertex3, which is an alias of Vector3
    scope.add_object("Vertex3", vec3);

	// Add the Vector2 class
	py::class_<Vector2> vec2(scope, "Vector2");
	vec2.def(py::init<double, double>());
	vec2.def(py::init<const Vector2&>());

	// greebo: Pick the correct overload - this is hard to read, but it is necessary
	vec2.def("x", static_cast<double& (Vector2::*)()>(&Vector2::x), py::return_value_policy::reference);
	vec2.def("y", static_cast<double& (Vector2::*)()>(&Vector2::y), py::return_value_policy::reference);
	vec2.def("getLength", &Vector2::getLength);
	vec2.def("getLengthSquared", &Vector2::getLengthSquared);
	vec2.def("dot", &Vector2::dot<double>);
	vec2.def("crossProduct", &Vector2::crossProduct<double>);
	// Most important operators
	vec2.def(py::self + py::self);		// __add__
	vec2.def(py::self - py::self);		// __sub__
	vec2.def(py::self += py::self);
	vec2.def(py::self -= py::self);
	vec2.def(py::self < py::self);	// __lt__
	vec2.def("__repr__", [](const Vector2& vec)
	{
		return "(" + string::to_string(vec.x()) + " " + string::to_string(vec.y()) + ")";
	});

	// Add the Vector4 class
	py::class_<Vector4> vec4(scope, "Vector4");
	vec4.def(py::init<double, double, double, double>());
	vec4.def(py::init<const Vector4&>());

	// greebo: Pick the correct overload - this is hard to read, but it is necessary
	vec4.def("x", static_cast<double& (Vector4::*)()>(&Vector4::x), py::return_value_policy::reference);
	vec4.def("y", static_cast<double& (Vector4::*)()>(&Vector4::y), py::return_value_policy::reference);
	vec4.def("z", static_cast<double& (Vector4::*)()>(&Vector4::z), py::return_value_policy::reference);
	vec4.def("w", static_cast<double& (Vector4::*)()>(&Vector4::w), py::return_value_policy::reference);
	vec4.def("getVector3", &Vector4::getVector3);
	vec4.def("getProjected", &Vector4::getProjected);
	vec4.def("dot", &Vector4::dot);
	// Most important operators
	vec4.def(py::self + py::self);		// __add__
	vec4.def(py::self - py::self);		// __sub__
	vec4.def(py::self += py::self);
	vec4.def(py::self -= py::self);
	vec4.def("__repr__", [](const Vector4& vec)
	{
		return "(" + string::to_string(vec.x()) + " " + string::to_string(vec.y()) + " " +
			string::to_string(vec.z()) + " " + string::to_string(vec.w()) + ")";
	});

	// Add the Vector4 and Quaternion with the same interface
	scope.add_object("Quaternion", vec4);

	// Declare AABB to python
	py::class_<AABB> aabb(scope, "AABB");
	aabb.def(py::init<>());
	aabb.def(py::init<const Vector3&, const Vector3&>());
	aabb.def_readwrite("origin", &AABB::origin);
	aabb.def_readwrite("extents", &AABB::extents);
	aabb.def("isValid", &AABB::isValid);
	aabb.def("getRadius", &AABB::getRadius);
	aabb.def("includePoint", &AABB::includePoint);
	aabb.def("includeAABB", &AABB::includeAABB);
}

} // namespace script
