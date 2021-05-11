#include "RadiantTest.h"
#include "scene/PointTrace.h"

#include <sstream>

namespace test
{

using PointTraceTest = RadiantTest;

const std::string LIN_DATA = "544.000000 64.000000 112.000000\n"
                             "544.000000 64.000000 240.000000\n"
                             "512.000000 64.000000 240.000000\n"
                             "512.000000 64.000000 112.000000\n"
                             "544.000000 64.000000 112.000000\n";

TEST_F(PointTraceTest, ConstructPointTraceEmpty)
{
    std::string s("");
    std::istringstream ss(s);

    // Constructing with empty data should not crash, or add any undefined or
    // [0, 0, 0] points.
    map::PointTrace trace(ss);
    EXPECT_EQ(trace.size(), 0);
}

TEST_F(PointTraceTest, ConstructPointTraceWithData)
{
    // Construct a stream to read the data
    std::istringstream iss(LIN_DATA);

    // Construct the PointTrace to read the stream and confirm the expected
    // number of points are parsed
    map::PointTrace trace(iss);
    EXPECT_EQ(trace.size(), 5);
}

TEST_F(PointTraceTest, IdentifyMapPointfiles)
{
    GlobalCommandSystem().executeCommand("OpenMap", std::string("altar.map"));

    // Check the number of pointfiles for this map
    int pointfiles = 0;
    GlobalMapModule().forEachPointfile([&](const std::string&)
                                       { ++pointfiles; });
    EXPECT_EQ(pointfiles, 0);
}

}