#include "RadiantTest.h"

// Issue #5336: Crash when using CSG Merge on brushes that are entities
TEST_F(RadiantTest, CSGMergeWithFuncStatic)
{
    loadMap("csg_merge_with_func_static.map");


}
