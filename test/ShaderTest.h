#pragma once

#include "Test.h"

class ShaderTest : 
	public Test
{
private:
	static Registrar _registrar;

public:
	std::string getName()
	{
		return "Shader Library";
	}

	void run();

private:
	void testShaderExpressions();
};
