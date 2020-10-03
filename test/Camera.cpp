#include "RadiantTest.h"

#include "icameraview.h"
#include "icommandsystem.h"
#include "render/View.h"

namespace test
{

TEST_F(RadiantTest, SetCameraPositionAndAngles)
{
	// Assume that we don't have any camera at first
	bool exceptionThrown = false;

	try
	{
		GlobalCameraManager().getActiveView();
	}
	catch (const std::runtime_error&)
	{
		exceptionThrown = true;
	}

	ASSERT_TRUE(exceptionThrown);

	render::View view;
	auto allocatedCam = GlobalCameraManager().createCamera(view, [](bool) {});

	// We should be able to get an active camera now
	exceptionThrown = false;
	try
	{
		auto& camera = GlobalCameraManager().getActiveView();
		auto oldPosition = camera.getCameraOrigin();
		auto oldAngles = camera.getCameraAngles();

		// Run the command, and compare the camera's angles
		GlobalCommandSystem().executeCommand("SetActiveCameraPosition", cmd::Argument(oldPosition + Vector3(50,50,50)));
		
		auto newPosition = camera.getCameraOrigin();
		ASSERT_TRUE(oldPosition != newPosition); // position changed
		ASSERT_TRUE(camera.getCameraAngles() == oldAngles); // angles unchanged

		GlobalCommandSystem().executeCommand("SetActiveCameraAngles", cmd::Argument(oldAngles + Vector3(10,10,10)));

		ASSERT_TRUE(camera.getCameraOrigin() == newPosition); // position unchanged
		ASSERT_TRUE(camera.getCameraAngles() != oldAngles); // angles changed
	}
	catch (const std::runtime_error&)
	{
		exceptionThrown = true;
	}

	ASSERT_TRUE(exceptionThrown == false);

	GlobalCameraManager().destroyCamera(allocatedCam);
}

}
