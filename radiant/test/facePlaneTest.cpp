#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE matrixTest
#include <boost/test/unit_test.hpp>

#include "radiant/brush/FacePlane.h"

BOOST_AUTO_TEST_CASE(constructAndSetPlane)
{
    Plane3 p3(1, 0, 0, 15);

    // Construct the FacePlane and give it the Plane3
    FacePlane fp;
    fp.setPlane(p3);

    BOOST_CHECK_EQUAL(fp.getPlane(), p3);
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
