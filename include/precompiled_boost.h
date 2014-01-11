/** 
 * greebo: Precompiled header module. This is included by the respective precompiled.h
 * files throughout the project. Many of those include boost headers into the
 * pre-compilation, and they do so by #include'ing this file.
 */
#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/noncopyable.hpp>
