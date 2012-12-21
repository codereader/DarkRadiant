AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs 
AM_CXXFLAGS = -fPIC

pkglib_LTLIBRARIES = libmath.la
libmath_la_LDFLAGS = -release @PACKAGE_VERSION@
libmath_la_SOURCES = Matrix4.cpp \
                     Frustum.cpp \
					 Plane3.cpp \
                     AABB.cpp \
                     Quaternion.cpp

TESTS = vectorTest matrixTest quaternionTest planeTest
check_PROGRAMS = vectorTest matrixTest quaternionTest planeTest

vectorTest_SOURCES = test/vectorTest.cpp
vectorTest_LDADD = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

matrixTest_SOURCES = test/matrixTest.cpp
matrixTest_LDADD = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS) libmath.la

quaternionTest_SOURCES = test/quaternionTest.cpp
quaternionTest_LDADD = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

planeTest_SOURCES = test/planeTest.cpp
planeTest_LDADD = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS) libmath.la
