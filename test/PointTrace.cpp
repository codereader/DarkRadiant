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
    EXPECT_EQ(trace.points().size(), 0);
}

TEST_F(PointTraceTest, ConstructPointTraceWithData)
{
    // Construct a stream to read the data
    std::istringstream iss(LIN_DATA);

    // Construct the PointTrace to read the stream and confirm the expected
    // number of points are parsed
    map::PointTrace trace(iss);
    auto ps = trace.points();
    ASSERT_EQ(ps.size(), 5);
    EXPECT_EQ(ps[0], Vector3(544, 64, 112));
    EXPECT_EQ(ps[1], Vector3(544, 64, 240));
    EXPECT_EQ(ps[2], Vector3(512, 64, 240));
    EXPECT_EQ(ps[3], Vector3(512, 64, 112));
    EXPECT_EQ(ps[4], Vector3(544, 64, 112));
}

namespace
{

using Paths = std::vector<fs::path>;

// Get pointfile names in a list
Paths pointfiles()
{
    Paths result;
    GlobalMapModule().forEachPointfile([&](const fs::path& pf)
                                       { result.push_back(pf); });
    return result;
}

}

// Test ensures that point file name case is not relevant 
// ALTAr.lin should be found when looking for files matching altar.map
TEST_F(PointTraceTest, IdentifyMapPointfiles)
{
    GlobalCommandSystem().executeCommand("OpenMap", std::string("altar.map"));

    // Check the pointfiles for this map
    auto pfs = pointfiles();
    ASSERT_EQ(pfs.size(), 2);
    EXPECT_EQ(pfs[0].filename(), "ALTAr.lin");
    EXPECT_EQ(pfs[1].filename(), "altar_portalL_544_64_112.lin");
}

TEST_F(PointTraceTest, PointFilesAssociatedWithCorrectMap)
{
    std::string modRelativePath = "maps/altar_in_pk4.map";
    GlobalCommandSystem().executeCommand("OpenMap", modRelativePath);

    // No pointfiles should be associated with this map, even though it also
    // starts with "altar_"
    EXPECT_EQ(pointfiles().size(), 0);
}

}