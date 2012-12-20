#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE quaternionTest
#include <boost/test/unit_test.hpp>

#include <math/Quaternion.h>

BOOST_AUTO_TEST_CASE(testQuaternions)
{
    Quaternion q1(3, 5, 7, 11);
    Quaternion q2(13, 17, 19, 23);

    Quaternion product = q1.getMultipliedBy(q2);

    BOOST_CHECK(product.x() == 188);
    BOOST_CHECK(product.y() == 336);
    BOOST_CHECK(product.z() == 356);
    BOOST_CHECK(product.w() == -4);

    Quaternion q1multiplied = q1;
    q1multiplied.multiplyBy(q2);

    BOOST_CHECK(q1multiplied.x() == 188);
    BOOST_CHECK(q1multiplied.y() == 336);
    BOOST_CHECK(q1multiplied.z() == 356);
    BOOST_CHECK(q1multiplied.w() == -4);

    Quaternion q1inverted = q1.getInverse();

    BOOST_CHECK(q1inverted.x() == -3);
    BOOST_CHECK(q1inverted.y() == -5);
    BOOST_CHECK(q1inverted.z() == -7);
    BOOST_CHECK(q1inverted.w() == 11);

    Quaternion normalised = q1.getNormalised();

    const double EPSILON = 0.0001f;
    BOOST_CHECK_CLOSE(normalised.x(), 0.2100420126042014f, EPSILON);
    BOOST_CHECK_CLOSE(normalised.y(), 0.3500700210070024f, EPSILON);
    BOOST_CHECK_CLOSE(normalised.z(), 0.4900980294098034f, EPSILON);
    BOOST_CHECK_CLOSE(normalised.w(), 0.7701540462154054f, EPSILON);

    Vector3 point(13, 17, 19);

    Vector3 transformed = q1.transformPoint(point);

    BOOST_CHECK(transformed.x() == q1.w()*q1.w()*point.x() + 2*q1.y()*q1.w()*point.z() - 2*q1.z()*q1.w()*point.y() + q1.x()*q1.x()*point.x() + 2*q1.y()*q1.x()*point.y() + 2*q1.z()*q1.x()*point.z() - q1.z()*q1.z()*point.x() - q1.y()*q1.y()*point.x());
    BOOST_CHECK(transformed.y() == 2*q1.x()*q1.y()*point.x() + q1.y()*q1.y()*point.y() + 2*q1.z()*q1.y()*point.z() + 2*q1.w()*q1.z()*point.x() - q1.z()*q1.z()*point.y() + q1.w()*q1.w()*point.y() - 2*q1.x()*q1.w()*point.z() - q1.x()*q1.x()*point.y());
    BOOST_CHECK(transformed.z() == 2*q1.x()*q1.z()*point.x() + 2*q1.y()*q1.z()*point.y() + q1.z()*q1.z()*point.z() - 2*q1.w()*q1.y()*point.x() - q1.y()*q1.y()*point.z() + 2*q1.w()*q1.x()*point.y() - q1.x()*q1.x()*point.z() + q1.w()*q1.w()*point.z());
}
