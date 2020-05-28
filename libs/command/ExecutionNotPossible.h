#pragma once

#include <stdexcept>
#include "ExecutionFailure.h"

namespace cmd
{

/**
 * Exception type thrown when an operation cannot be executed.
 * Any function called by the CommandSystem can throw this type,
 * it will be caught by the CommandSystem and possibly end up
 * displayed in an error popup window.
 */
class ExecutionNotPossible :
    public ExecutionFailure
{
public:
    ExecutionNotPossible(const std::string& msg) :
        ExecutionFailure(msg)
    {}
};

}
