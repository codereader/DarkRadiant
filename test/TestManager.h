#pragma once

#include <string>
#include <map>
#include <iostream>
#include "Test.h"

class TestManager
{
private:
	typedef std::map<std::string, TestPtr> TestMap;
	TestMap _tests;

	TestManager()
	{}

public:
	void registerTest(const TestPtr& test)
	{
		_tests.insert(TestMap::value_type(test->getName(), test));
	}

	void runAll()
	{
		for (TestMap::const_iterator i = _tests.begin(); i != _tests.end(); ++i)
		{
			i->second->prepare();

			try
			{
				std::cout << "Running test: " << i->first << "...";

				i->second->run();

				std::cout << "done." << std::endl;
			}
			catch (Test::TestFailedException& ex)
			{
				std::cout << i->first << " failed with message: " << ex.what() << std::endl;
			}

			i->second->cleanup();
		}
	}

	// Contains the singleton instance
	static TestManager& Instance()
	{
		static TestManager _testManager;
		return _testManager;
	}
};
