/** 
 * greebo: Precompiled header module. This is included by the respective precompiled.h
 * files throughout the project. Many of those include boost headers into the
 * pre-compilation, and they do so by #include'ing this file.
 */
#pragma once

#include <memory>
#include <functional>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "util/Noncopyable.h"
