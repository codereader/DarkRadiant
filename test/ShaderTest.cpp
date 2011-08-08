#include "ShaderTest.h"

#include "math/Matrix4.h"
#include "math/Quaternion.h"

void ShaderTest::run()
{
	testShaderExpressions();
}

void ShaderTest::testShaderExpressions()
{
	// TODO
}

// Initialise the static registrar object
Test::Registrar ShaderTest::_registrar(TestPtr(new ShaderTest));
