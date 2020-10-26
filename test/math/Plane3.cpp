#include "gtest/gtest.h"

#include "math/Plane3.h"
#include "math/Matrix4.h"

namespace test
{

namespace
{
    const double EPSILON = 0.0000001;
    const double ONE_OVER_ROOT_TWO = 1.0 / sqrt(2.0);
}

TEST(Plane3, ConstructPlane)
{
    Plane3 plane(1, 0, 0, 4);
    EXPECT_EQ(plane.normal(), Vector3(1, 0, 0));
    EXPECT_EQ(plane.dist(), 4);

    Plane3 plane2(1.5, 56.4, -325, 19.23);
    EXPECT_EQ(plane2.normal(), Vector3(1.5, 56.4, -325));
    EXPECT_EQ(plane2.dist(), 19.23);

    Plane3 copy = plane2;
    EXPECT_EQ(copy.normal(), Vector3(1.5, 56.4, -325));
    EXPECT_EQ(copy.dist(), 19.23);
}

TEST(Plane3, TranslatePlaneDirectly)
{
    // Basic plane
    Plane3 plane(1, 0, 0, 2.5);
    plane.translate(Vector3(2, 0, 0));

    EXPECT_EQ(plane.normal(), Vector3(1, 0, 0));
    EXPECT_EQ(plane.dist(), 0.5);

    // Inclined plane
    Vector3 normal = Vector3(1.5, 2.6, -0.3).getNormalised();
    Plane3 inclined(normal, 4.5);
    EXPECT_EQ(inclined.normal(), normal);

    inclined.translate(Vector3(1, 1, 0));
    EXPECT_NEAR(inclined.normal().x(), 0.49724515641972766, EPSILON);
    EXPECT_NEAR(inclined.normal().y(), 0.86189160446086122, EPSILON);
    EXPECT_NEAR(inclined.normal().z(), -0.0994490312834552, EPSILON);

    // Check that a plane transformed by a translation matrix matches the
    // plane's own translation function
    Vector3 normal3 = Vector3(0.5, 1.24, 56).getNormalised();
    Plane3 plane3(normal3, -56.1);

    Vector3 transVec(1.5, 45, -2.29);
    Matrix4 transMat = Matrix4::getTranslation(transVec);
    Plane3 p3_matrix = plane3.transformed(transMat);

    Plane3 p3_trans = plane3;
    p3_trans.translate(transVec);

    EXPECT_NEAR(p3_matrix.normal().x(), p3_trans.normal().x(), EPSILON);
    EXPECT_NEAR(p3_matrix.normal().y(), p3_trans.normal().y(), EPSILON);
    EXPECT_NEAR(p3_matrix.normal().z(), p3_trans.normal().z(), EPSILON);
}

TEST(Plane3, TranslatePlaneWithMatrix)
{
    // Plane for y = 5
    Plane3 plane(0, 1, 0, 5);
    EXPECT_EQ(plane.normal(), Vector3(0, 1, 0));

    // Translate the plane by 3 in the Y direction
    Matrix4 trans = Matrix4::getTranslation(Vector3(0, 3, 0));
    Plane3 newPlane = plane.transformed(trans);

    // The normal should not have changed but the distance should, although for
    // some reason the translation happens backwards (i.e. negative Y)
    EXPECT_EQ(newPlane.normal(), Vector3(0, 1, 0));
    EXPECT_EQ(newPlane.dist(), 2);

    // Inclined plane
    Plane3 inclined(1, -1, 0, 0);
    EXPECT_EQ(inclined.dist(), 0);

    // Again move 3 in the Y direction
    Plane3 newInclined = inclined.transformed(trans);

    // Again there should be no normal change, but a distance change
    EXPECT_EQ(newInclined.normal(), Vector3(1, -1, 0));
    EXPECT_EQ(newInclined.dist(), 3);

    // If moved along Z the distance should not change
    Plane3 movedZ = inclined.transformed(
        Matrix4::getTranslation(Vector3(0, 0, 2.3))
    );
    EXPECT_EQ(movedZ.normal(), Vector3(1, -1, 0));
    EXPECT_EQ(movedZ.dist(), 0);
}

TEST(Plane3, RotatePlane)
{
    // A plane at 5 in the Y direction
    Plane3 plane(0, 1, 0, 5);

    // Rotate 45 degrees around the Z axis
    Matrix4 rot = Matrix4::getRotation(
        Vector3(0, 0, 1), c_pi / 4
    );
    Plane3 rotated = plane.transformed(rot);

    EXPECT_NEAR(rotated.normal().x(), ONE_OVER_ROOT_TWO, EPSILON);
    EXPECT_NEAR(rotated.normal().y(), ONE_OVER_ROOT_TWO, EPSILON);
    EXPECT_EQ(rotated.normal().z(), 0);
}

TEST(Plane3, ScalePlane)
{
    Plane3 plane(1, -1, 0, 3.5);

    // Scale the plane by a factor of 2
    Matrix4 times2 = Matrix4::getScale(Vector3(2, 2, 2));
    Plane3 scaled = plane.transformed(times2);

    EXPECT_EQ(scaled.normal().x(), 2);
    EXPECT_EQ(scaled.normal().y(), -2);
    EXPECT_EQ(scaled.normal().z(), 0);

    EXPECT_EQ(scaled.dist(), 28);
}

TEST(Plane3, TransformPlane)
{
    // Check transform with some randomly-generated values with no particular
    // geometric meaning
    Plane3 plane(-2.643101, -47.856364, 17.5173264, -35.589485);
    Plane3 copy = plane;

    Matrix4 arbMatrix = Matrix4::byRows(
        -30.1065587, -28.048640, -10.003604, 18.986724,
        18.94792014, -16.186764, -8.7790217, -32.59777,
        12.85452125, -8.8305872, -36.502315, 32.345895,
        0, 0, 0, 1
    );
    Plane3 transformed = plane.transformed(arbMatrix);
    EXPECT_EQ(plane, copy); // must not be changed by transformed()

    // TODO: Improve the maths so that this works more accurately
    static const double ROUGH_EPSILON = 0.001;
    EXPECT_NEAR(transformed.normal().x(), 1246.64431, ROUGH_EPSILON);
    EXPECT_NEAR(transformed.normal().y(), 570.773414, ROUGH_EPSILON);
    EXPECT_NEAR(transformed.normal().z(), -250.79896, ROUGH_EPSILON);

    EXPECT_NEAR(transformed.dist(), -69140351.87873, ROUGH_EPSILON);
}

}
