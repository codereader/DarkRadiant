#ifndef _DEBUGGING_FUNCTIONS_H_
#define _DEBUGGING_FUNCTIONS_H_

/// \brief Debugging macros for fatal error/assert messages.

#if defined(_DEBUG)
#define DEBUG_ASSERTS
#endif

#include "imodule.h" // for the ErrorHandlingFunction typedef

// This method holds a function pointer which can do some error display (like popups)
inline ErrorHandlingFunction& GlobalErrorHandler()
{
	static ErrorHandlingFunction _func;
	return _func;
}

#if defined(DEBUG_ASSERTS)

// Define the breakpoint function, to fire up the debugger
#if defined(_MSC_VER) && defined(_M_IX86)
#define DEBUGGER_BREAKPOINT() __asm { int 3 }
#elif defined(_MSC_VER) && defined(_WIN64)
#define DEBUGGER_BREAKPOINT() __debugbreak()
#elif defined (__i386__) && defined (__GNUC__) && __GNUC__ >= 2
#define DEBUGGER_BREAKPOINT() __asm__ __volatile__ ("int $03")
#else
#include <signal.h>

#define DEBUGGER_BREAKPOINT() raise(SIGTRAP);
#endif

#define STR(x)	#x
#define STR2(x)	STR(x)
#define FILE_LINE __FILE__ ":" STR2(__LINE__)

/// \brief Sends a \p message to the current debug-message-handler text-output-stream if \p condition evaluates to false.
#define ASSERT_MESSAGE(condition, message)\
	if(!(condition)) { GlobalErrorHandler()("DarkRadiant - Assertion Failure", std::string(FILE_LINE) + "\nAssertion failure: " + message + "\nBreak into the debugger?"); }

/// \brief Sends a \p message to the current debug-message-handler text-output-stream.
#define ERROR_MESSAGE(message)\
{ GlobalErrorHandler()("DarkRadiant - Runtime Error", std::string(FILE_LINE) + "\nRuntime Error: " + message + "\nBreak into the debugger?"); }

#define ASSERT_NOTNULL(ptr) ASSERT_MESSAGE(ptr != 0, "pointer \"" #ptr "\" is null")

#else // Release Builds

#define ASSERT_MESSAGE(condition, message)
#define ERROR_MESSAGE(message)
#define ASSERT_NOTNULL(ptr)
#define DEBUGGER_BREAKPOINT()

#endif

#endif /* _DEBUGGING_FUNCTIONS_H_ */
