#pragma once

#include "gtest/gtest.h"
#include <iostream>

class RadiantTest :
	public ::testing::Test
{
protected:
	// The RadiantApp owns the ApplicationContext which is then passed to the
	// ModuleRegistry as a reference.
	radiant::ApplicationContextBase _context;

	std::unique_ptr<module::CoreModule> _coreModule;

protected:
	RadiantTest()
	{
		std::cout << "Test ctor";
	}

	~RadiantTest()
	{
		std::cout << "Test dtor";
	}
};