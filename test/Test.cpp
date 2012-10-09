#include "Test.h"

#include "TestManager.h"

Test::Registrar::Registrar(const TestPtr& test)
{
	TestManager::Instance().registerTest(test);
}
