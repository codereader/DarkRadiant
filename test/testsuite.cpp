#include "TestManager.h"

/**
 * Main entry point for the application.
 */
int main (int argc, char* argv[])
{
	// Run all registered test
	TestManager::Instance().runAll();

	std::cout << "Press enter to close this test." << std::endl;

	std::string dummy;
	std::cin >> dummy;

	return 0;
}

