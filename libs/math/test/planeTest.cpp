#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE vectorTest
#include <boost/test/unit_test.hpp>

#include <math/Plane3.h>
#include <math/Matrix4.h>

namespace
{
    const double EPSILON = 0.000001;
}

BOOST_AUTO_TEST_CASE(constructPlane)
{
    Plane3 plane(1, 0, 0, 4);
    BOOST_CHECK_EQUAL(plane.normal(), Vector3(1, 0, 0));
    BOOST_CHECK_EQUAL(plane.dist(), 4);

    Plane3 plane2(1.5, 56.4, -325, 19.23);
    BOOST_CHECK_EQUAL(plane2.normal(), Vector3(1.5, 56.4, -325));
    BOOST_CHECK_EQUAL(plane2.dist(), 19.23);

    Plane3 copy = plane2;
    BOOST_CHECK_EQUAL(copy.normal(), Vector3(1.5, 56.4, -325));
    BOOST_CHECK_EQUAL(copy.dist(), 19.23);
}

BOOST_AUTO_TEST_CASE(translatePlane)
{
    // Basic plane
    Plane3 plane(1, 0, 0, 2.5);
    plane.translate(Vector3(2, 0, 0));

    BOOST_CHECK_EQUAL(plane.normal(), Vector3(1, 0, 0));
    BOOST_CHECK_EQUAL(plane.dist(), 0.5);

    // Inclined plane
    Vector3 normal = Vector3(1.5, 2.6, -0.3).getNormalised();
    Plane3 inclined(normal, 4.5);
    BOOST_CHECK_EQUAL(inclined.normal(), normal);

    inclined.translate(Vector3(1, 1, 0));
    BOOST_CHECK_CLOSE(inclined.normal().x(), 0.49724515641972766, EPSILON);
    BOOST_CHECK_CLOSE(inclined.normal().y(), 0.86189160446086122, EPSILON);
    BOOST_CHECK_CLOSE(inclined.normal().z(), -0.0994490312834552, EPSILON);

    // Check that a plane transformed by a translation matrix matches the
    // plane's own translation function
    Vector3 normal3 = Vector3(0.5, 1.24, 56).getNormalised();
    Plane3 plane3(normal3, -56.1);

    Vector3 transVec(1.5, 45, -2.29);
    Matrix4 transMat = Matrix4::getTranslation(transVec);
    Plane3 p3_matrix = transMat.transform(plane3);

    Plane3 p3_trans = plane3;
    p3_trans.translate(transVec);

    BOOST_CHECK_CLOSE(p3_matrix.normal().x(), p3_trans.normal().x(), EPSILON);
    BOOST_CHECK_CLOSE(p3_matrix.normal().y(), p3_trans.normal().y(), EPSILON);
    BOOST_CHECK_CLOSE(p3_matrix.normal().z(), p3_trans.normal().z(), EPSILON);
}
