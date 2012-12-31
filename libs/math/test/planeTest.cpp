#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE vectorTest
#include <boost/test/unit_test.hpp>

#include <math/Plane3.h>
#include <math/Matrix4.h>

#include <boost/math/constants/constants.hpp>

using namespace boost::math;

namespace
{
    const double EPSILON = 0.0000001;
    const double ONE_OVER_ROOT_TWO = 1.0 / constants::root_two<double>();
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

BOOST_AUTO_TEST_CASE(translatePlaneDirectly)
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
    Plane3 p3_matrix = plane3.transformed(transMat);

    Plane3 p3_trans = plane3;
    p3_trans.translate(transVec);

    BOOST_CHECK_CLOSE(p3_matrix.normal().x(), p3_trans.normal().x(), EPSILON);
    BOOST_CHECK_CLOSE(p3_matrix.normal().y(), p3_trans.normal().y(), EPSILON);
    BOOST_CHECK_CLOSE(p3_matrix.normal().z(), p3_trans.normal().z(), EPSILON);
}

BOOST_AUTO_TEST_CASE(translatePlaneWithMatrix)
{
    // Plane for y = 5
    Plane3 plane(0, 1, 0, 5);
    BOOST_CHECK_EQUAL(plane.normal(), Vector3(0, 1, 0));

    // Translate the plane by 3 in the Y direction
    Matrix4 trans = Matrix4::getTranslation(Vector3(0, 3, 0));
    Plane3 newPlane = plane.transformed(trans);

    // The normal should not have changed but the distance should, although for
    // some reason the translation happens backwards (i.e. negative Y)
    BOOST_CHECK_EQUAL(newPlane.normal(), Vector3(0, 1, 0));
    BOOST_CHECK_EQUAL(newPlane.dist(), 2);

    // Inclined plane
    Plane3 inclined(1, -1, 0, 0);
    BOOST_CHECK_EQUAL(inclined.dist(), 0);

    // Again move 3 in the Y direction
    Plane3 newInclined = inclined.transformed(trans);

    // Again there should be no normal change, but a distance change
    BOOST_CHECK_EQUAL(newInclined.normal(), Vector3(1, -1, 0));
    BOOST_CHECK_EQUAL(newInclined.dist(), 3);

    // If moved along Z the distance should not change
    Plane3 movedZ = inclined.transformed(
        Matrix4::getTranslation(Vector3(0, 0, 2.3))
    );
    BOOST_CHECK_EQUAL(movedZ.normal(), Vector3(1, -1, 0));
    BOOST_CHECK_EQUAL(movedZ.dist(), 0);
}

BOOST_AUTO_TEST_CASE(rotatePlane)
{
    // A plane at 5 in the Y direction
    Plane3 plane(0, 1, 0, 5);

    // Rotate 45 degrees around the Z axis
    Matrix4 rot = Matrix4::getRotation(
        Vector3(0, 0, 1), constants::pi<double>() / 4
    );
    Plane3 rotated = plane.transformed(rot);

    BOOST_CHECK_CLOSE(rotated.normal().x(), ONE_OVER_ROOT_TWO, EPSILON);
    BOOST_CHECK_CLOSE(rotated.normal().y(), ONE_OVER_ROOT_TWO, EPSILON);
    BOOST_CHECK_EQUAL(rotated.normal().z(), 0);
}

BOOST_AUTO_TEST_CASE(scalePlane)
{
    Plane3 plane(1, -1, 0, 3.5);

    // Scale the plane by a factor of 2
    Matrix4 times2 = Matrix4::getScale(Vector3(2, 2, 2));
    Plane3 scaled = plane.transformed(times2);

    BOOST_CHECK_EQUAL(scaled.normal().x(), 2);
    BOOST_CHECK_EQUAL(scaled.normal().y(), -2);
    BOOST_CHECK_EQUAL(scaled.normal().z(), 0);

    BOOST_CHECK_EQUAL(scaled.dist(), 28);
}

BOOST_AUTO_TEST_CASE(transformPlane)
{
    // Check transform with some randomly-generated values with no particular
    // geometric meaning
    Plane3 plane(-2.643101, -47.856364, 17.5173264, -35.589485);
    Plane3 copy = plane;

    Matrix4 arbMatrix = Matrix4::byRows(
        -30.1065587, -28.048640, -10.003604, 18.986724,
        18.94792014, -16.186764, -8.7790217, -32.59777,
        12.85452125, -8.8305872, -36.502315, 32.345895,
        0,           0,          0,          1
    );
    Plane3 transformed = plane.transformed(arbMatrix);
    BOOST_CHECK_EQUAL(plane, copy); // must not be changed by transformed()

    // TODO: Improve the maths so that this works more accurately
    static const double ROUGH_EPSILON = 0.001;
    BOOST_CHECK_CLOSE(transformed.normal().x(), 1246.64431, ROUGH_EPSILON);
    BOOST_CHECK_CLOSE(transformed.normal().y(), 570.775895, ROUGH_EPSILON);
    BOOST_CHECK_CLOSE(transformed.normal().z(), -250.79896, ROUGH_EPSILON);

    BOOST_CHECK_CLOSE(transformed.dist(), -69140351.87, EPSILON);
}
