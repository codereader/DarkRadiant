#pragma once

#include "gtest/gtest.h"

namespace test
{

// EXPECT that two matrices are close to each other (using GTest's EXPECT_NEAR)
inline void expectNear(const Matrix4& m1, const Matrix4& m2)
{
    constexpr double TestEpsilon = 0.0001;

    EXPECT_NEAR(m1.xx(), m2.xx(), TestEpsilon);
    EXPECT_NEAR(m1.xy(), m2.xy(), TestEpsilon);
    EXPECT_NEAR(m1.xz(), m2.xz(), TestEpsilon);
    EXPECT_NEAR(m1.xw(), m2.xw(), TestEpsilon);

    EXPECT_NEAR(m1.yx(), m2.yx(), TestEpsilon);
    EXPECT_NEAR(m1.yy(), m2.yy(), TestEpsilon);
    EXPECT_NEAR(m1.yz(), m2.yz(), TestEpsilon);
    EXPECT_NEAR(m1.yw(), m2.yw(), TestEpsilon);

    EXPECT_NEAR(m1.zx(), m2.zx(), TestEpsilon);
    EXPECT_NEAR(m1.zy(), m2.zy(), TestEpsilon);
    EXPECT_NEAR(m1.zz(), m2.zz(), TestEpsilon);
    EXPECT_NEAR(m1.zw(), m2.zw(), TestEpsilon);

    EXPECT_NEAR(m1.tx(), m2.tx(), TestEpsilon);
    EXPECT_NEAR(m1.ty(), m2.ty(), TestEpsilon);
    EXPECT_NEAR(m1.tz(), m2.tz(), TestEpsilon);
    EXPECT_NEAR(m1.tw(), m2.tw(), TestEpsilon);
}

}
