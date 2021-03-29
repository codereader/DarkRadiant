#pragma once

#include "gtest/gtest.h"

namespace test
{

// EXPECT that two matrices are close to each other (using GTest's own
// EXPECT_DOUBLE_EQ)
inline void expectNear(const Matrix4& m1, const Matrix4& m2)
{
    EXPECT_DOUBLE_EQ(m1.xx(), m2.xx());
    EXPECT_DOUBLE_EQ(m1.xy(), m2.xy());
    EXPECT_DOUBLE_EQ(m1.xz(), m2.xz());
    EXPECT_DOUBLE_EQ(m1.xw(), m2.xw());

    EXPECT_DOUBLE_EQ(m1.yx(), m2.yx());
    EXPECT_DOUBLE_EQ(m1.yy(), m2.yy());
    EXPECT_DOUBLE_EQ(m1.yz(), m2.yz());
    EXPECT_DOUBLE_EQ(m1.yw(), m2.yw());

    EXPECT_DOUBLE_EQ(m1.zx(), m2.zx());
    EXPECT_DOUBLE_EQ(m1.zy(), m2.zy());
    EXPECT_DOUBLE_EQ(m1.zz(), m2.zz());
    EXPECT_DOUBLE_EQ(m1.zw(), m2.zw());

    EXPECT_DOUBLE_EQ(m1.tx(), m2.tx());
    EXPECT_DOUBLE_EQ(m1.ty(), m2.ty());
    EXPECT_DOUBLE_EQ(m1.tz(), m2.tz());
    EXPECT_DOUBLE_EQ(m1.tw(), m2.tw());
}

}
