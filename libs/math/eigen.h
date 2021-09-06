#pragma once

// Import the Eigen header files needed by our math classes

#undef Success // get rid of fuckwit X.h macro

// Silence C++17 deprecation warnings from Eigen\src\Core\util\Meta.h(373), will be unsilenced at the bottom of this file
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4127)
#endif

#include <Eigen/Geometry>
