#pragma once

#include <stdexcept>
#include <boost/shared_ptr.hpp>

class Test;
typedef boost::shared_ptr<Test> TestPtr;

// Abstract definition of a unit test
class Test
{
public:
	class TestFailedException :
		public std::runtime_error
	{
	public:
		TestFailedException(const std::string& msg) :
			std::runtime_error(msg)
		{}
	};

	/**
	 * Each test should have a name.
	 */
	virtual std::string getName() = 0;

	/** 
	 * Prepares this test, should be called at least once before Run().
	 */
	virtual void prepare() {}

	/**
	 * Runs the test. If any test fails, a TestFailedException is thrown.
	 */
	virtual void run() = 0;

	/** 
	 * Cleans up any data allocated by this test, should be called after Run()
	 */
	virtual void cleanup() {}

	class Registrar
	{
	public:
		Registrar(const TestPtr& test);
	};
};
typedef boost::shared_ptr<Test> TestPtr;

// Macro to run an expression and throw a FailureException if it fails

#define REQUIRE_TRUE(x, msg) if (!(x)) throw Test::TestFailedException((msg));
#define REQUIRE_FALSE(x, msg) REQUIRE_TRUE(!(x))