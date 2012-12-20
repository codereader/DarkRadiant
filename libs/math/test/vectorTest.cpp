#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE vectorTest
#include <boost/test/unit_test.hpp>

#include <math/Vector3.h>

BOOST_AUTO_TEST_CASE(constructVector3)
{
    Vector3 vec(1.0, 2.0, 3.5);

    BOOST_CHECK_EQUAL(vec.x(), 1.0);
    BOOST_CHECK_EQUAL(vec.y(), 2.0);
    BOOST_CHECK_EQUAL(vec.z(), 3.5);
}
