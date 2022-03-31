#include "gtest/gtest.h"

#include "settings/MajorMinorVersion.h"

namespace test
{

TEST(MajorMinorVersionTest, ParseFromString)
{
    settings::MajorMinorVersion v1("2.13.0");
    EXPECT_EQ(v1.getMajorVersion(), 2);
    EXPECT_EQ(v1.getMinorVersion(), 13);

    settings::MajorMinorVersion v2("2.14.0pre4");
    EXPECT_EQ(v2.getMajorVersion(), 2);
    EXPECT_EQ(v2.getMinorVersion(), 14);

    settings::MajorMinorVersion v3("2.15.0pre1_rev563abfa");
    EXPECT_EQ(v3.getMajorVersion(), 2);
    EXPECT_EQ(v3.getMinorVersion(), 15);

    settings::MajorMinorVersion v3a("2.15.4_rev563abfa");
    EXPECT_EQ(v3a.getMajorVersion(), 2);
    EXPECT_EQ(v3a.getMinorVersion(), 15);

    settings::MajorMinorVersion v4("3.14.5");
    EXPECT_EQ(v4.getMajorVersion(), 3);
    EXPECT_EQ(v4.getMinorVersion(), 14);

    settings::MajorMinorVersion v5("11.0.0");
    EXPECT_EQ(v5.getMajorVersion(), 11);
    EXPECT_EQ(v5.getMinorVersion(), 0);

    settings::MajorMinorVersion v6("11.0.1");
    EXPECT_EQ(v6.getMajorVersion(), 11);
    EXPECT_EQ(v6.getMinorVersion(), 0);

    settings::MajorMinorVersion v7("11.100.1");
    EXPECT_EQ(v7.getMajorVersion(), 11);
    EXPECT_EQ(v7.getMinorVersion(), 100);

    // Invalid expressions
    EXPECT_THROW(settings::MajorMinorVersion("11.a100.1"), std::runtime_error);
    EXPECT_THROW(settings::MajorMinorVersion("11.1"), std::runtime_error);
    EXPECT_THROW(settings::MajorMinorVersion("x.y"), std::runtime_error);
    EXPECT_THROW(settings::MajorMinorVersion("2_3_7"), std::runtime_error);
    EXPECT_THROW(settings::MajorMinorVersion("10.8.9-"), std::runtime_error);
}

TEST(MajorMinorVersionTest, LessThanOperator)
{
    settings::MajorMinorVersion v1("2.13.0");
    settings::MajorMinorVersion v2("2.14.0pre4");
    EXPECT_TRUE(v1 < v2) << "2.13.0 should be smaller than 2.14.0pre4";
    EXPECT_FALSE(v2 < v1) << "2.14.0pre4 should not be smaller than 2.13.0";

    settings::MajorMinorVersion v3("3.14.5");
    EXPECT_TRUE(v2 < v3) << "2.14.0pre4 should be smaller than 3.14.5";
    EXPECT_TRUE(v1 < v3) << "2.13.0 should be smaller than 3.14.5";
    EXPECT_FALSE(v3 < v1) << "3.14.5 should not be smaller than 2.13.0";
    EXPECT_FALSE(v3 < v2) << "3.14.5 should not be smaller than 2.14.0pre4";

    settings::MajorMinorVersion v5("11.0.0");
    EXPECT_TRUE(v1 < v5) << "2.13.0 should be smaller than 11.0.0";
    EXPECT_TRUE(v2 < v5) << "2.14.0pre4 should be smaller than 11.0.0";
    EXPECT_TRUE(v3 < v5) << "3.14.5 should be smaller than 11.0.0";
    EXPECT_FALSE(v5 < v1) << "11.0.0 should not be smaller than 2.13.0";
    EXPECT_FALSE(v5 < v2) << "11.0.0 should not be smaller than 2.14.0pre4";
    EXPECT_FALSE(v5 < v3) << "11.0.0 should not be smaller than 3.14.5";

    settings::MajorMinorVersion v6("11.0.1");
    EXPECT_FALSE(v5 < v6) << "11.0.0 should not be smaller than 11.0.1 (micro version is ignored)";
    EXPECT_FALSE(v6 < v5) << "11.0.1 should not be smaller than 11.0.0 (micro version is ignored)";

    settings::MajorMinorVersion v7("11.100.1");
    EXPECT_TRUE(v5 < v7) << "11.0.0 should be smaller than 11.100.1";
    EXPECT_TRUE(v6 < v7) << "11.0.1 should be smaller than 11.100.1";
    EXPECT_FALSE(v7 < v5) << "11.100.1 should not be smaller than 11.0.0";
    EXPECT_FALSE(v7 < v6) << "11.100.1 should not be smaller than 11.0.1";

    settings::MajorMinorVersion v8("11.10.3");
    settings::MajorMinorVersion v9("11.1.3");
    EXPECT_TRUE(v9 < v8) << "11.1.3 should be smaller than 11.10.3";
    EXPECT_FALSE(v8 < v9) << "11.10.3 should not be smaller than 11.1.3";
}

TEST(MajorMinorVersionTest, ToString)
{
    EXPECT_EQ(settings::MajorMinorVersion("2.13.0").toString(), "2.13");
    EXPECT_EQ(settings::MajorMinorVersion("2.14.0pre4").toString(), "2.14");
    EXPECT_EQ(settings::MajorMinorVersion("3.14.5").toString(), "3.14");
    EXPECT_EQ(settings::MajorMinorVersion("11.0.0").toString(), "11.0");
    EXPECT_EQ(settings::MajorMinorVersion("11.0.1").toString(), "11.0");
    EXPECT_EQ(settings::MajorMinorVersion("11.100.1").toString(), "11.100");
    EXPECT_EQ(settings::MajorMinorVersion("9.10.3").toString(), "9.10");
    EXPECT_EQ(settings::MajorMinorVersion("11.1.300").toString(), "11.1");
}

}
