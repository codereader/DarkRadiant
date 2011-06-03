#pragma once

#include "Test.h"

class MathTest : 
	public Test
{
private:
	static Registrar _registrar;

public:
	std::string getName()
	{
		return "Math Library";
	}

	void run();

private:
	void testIdentity();
	void testMultiplication();
};
