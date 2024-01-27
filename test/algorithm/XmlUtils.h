#pragma once

#include "gtest/gtest.h"
#include <fstream>
#include <sstream>

#include "FileUtils.h"
#include "string/predicate.h"

namespace test
{

namespace algorithm
{

inline void assertStringIsMapxFile(const std::string& content)
{
    ASSERT_TRUE(string::starts_with(content, "<?xml version=\"1.0\" encoding=\"utf-8\"?>"));
    ASSERT_TRUE(content.find("<map") != std::string::npos);
}

// Very rough check to see if path points to a file that looks like an XML document
inline void assertFileIsMapxFile(const std::string& path)
{
    assertStringIsMapxFile(loadFileToString(path));
}

}

}
