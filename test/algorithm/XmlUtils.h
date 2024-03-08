#pragma once

#include "gtest/gtest.h"
#include <fstream>
#include <sstream>

#include "FileUtils.h"
#include "string/predicate.h"
#include "string/split.h"

namespace test
{

namespace algorithm
{

inline void assertStringIsMapxFile(const std::string& content)
{
    const auto tagContents = string::splitToVec(content, "<>");
    ASSERT_GE(tagContents.size(), 1);
    ASSERT_EQ(tagContents[0], "?xml version=\"1.0\" encoding=\"utf-8\"?");

    ASSERT_TRUE(content.find("<map") != std::string::npos);
}

// Very rough check to see if path points to a file that looks like an XML document
inline void assertFileIsMapxFile(const std::string& path)
{
    assertStringIsMapxFile(loadFileToString(path));
}

}

}
