#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE facePlaneTest
#include <boost/test/unit_test.hpp>

#include "radiant/brush/FacePlane.h"
#include "math/Matrix4.h"

namespace
{
    const float EPSILON = 0.001;

    void checkNormalised(const FacePlane& fp)
    {
        BOOST_CHECK_CLOSE(fp.getPlane().normal().getLength(), 1, EPSILON);
    }
}

BOOST_AUTO_TEST_CASE(constructAndSetPlane)
{
    Plane3 p3(1, 0, 0, 15);

    // Construct the FacePlane and give it the Plane3
    FacePlane fp;
    fp.setPlane(p3);

    BOOST_CHECK_EQUAL(fp.getPlane(), p3);
}

BOOST_AUTO_TEST_CASE(copy)
{
    Plane3 p3(1, 0, 0, -15.9);
    FacePlane first;
    first.setPlane(p3);

    FacePlane copy(first);
    BOOST_CHECK_EQUAL(first.getPlane(), copy.getPlane());

    FacePlane assign;
    assign = first;
    BOOST_CHECK_EQUAL(first.getPlane(), assign.getPlane());
}

BOOST_AUTO_TEST_CASE(exportMemento)
{
    // Create a FacePlane and export its memento
    FacePlane orig;
    orig.setPlane(Plane3(1, 2, 3, 4));
    FacePlane::SavedState memento(orig);

    // Pass the memento state to another plane
    FacePlane another;
    another.setPlane(Plane3(-1, 4, 5, 12));
    BOOST_CHECK_NE(another.getPlane(), orig.getPlane());

    memento.exportState(another);
    BOOST_CHECK_EQUAL(another.getPlane(), orig.getPlane());
}

BOOST_AUTO_TEST_CASE(rotateWithMatrix)
{
    FacePlane fp;
    const static Plane3 ORIG(1, 0, 0, -5);
    fp.setPlane(ORIG);

    // Transform the plane with a rotation matrix
    Matrix4 rot = Matrix4::getRotation(Vector3(0, 1, 0), 2);
    fp.transform(rot);

    BOOST_CHECK_NE(fp.getPlane(), ORIG);
    BOOST_CHECK_EQUAL(fp.getPlane(), ORIG.transformed(rot));
    checkNormalised(fp);
}

BOOST_AUTO_TEST_CASE(translateWithMatrix)
{
    FacePlane fp;
    const static Plane3 ORIG(0, 1, 0, -5);
    fp.setPlane(ORIG);

    // Get a matrix to translate in the Y direction
    Matrix4 trans = Matrix4::getTranslation(Vector3(0, 3, 0));
    fp.transform(trans);

    // The FacePlane transformation negates the distance, giving us the value we
    // might expect rather than the negated value that comes out of Plane3.
    BOOST_CHECK_EQUAL(fp.getPlane(), Plane3(0, 1, 0, -2));
    checkNormalised(fp);
}

BOOST_AUTO_TEST_CASE(translateWithVector)
{
    FacePlane fp;
    const static Plane3 ORIG(0, 1, 0, -5);
    fp.setPlane(ORIG);

    // Check that the translate() method gives the same geometric result as
    // using a translation matrix
    static const Vector3 TRANSLATION(0, 3, 0);
    Matrix4 transMat = Matrix4::getTranslation(TRANSLATION);

    FacePlane translated = fp;
    translated.translate(TRANSLATION);

    FacePlane transformed = fp;
    transformed.transform(transMat);

    BOOST_CHECK_EQUAL(translated.getPlane(), transformed.getPlane());
}
