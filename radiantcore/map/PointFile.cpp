#include "PointFile.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "itextstream.h"
#include "icameraview.h"
#include "imap.h"
#include "iorthoview.h"
#include <fstream>
#include <iostream>

#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "map/Map.h"
#include "scene/PointTrace.h"
#include <fmt/format.h>

#include "module/StaticModule.h"
#include "command/ExecutionFailure.h"

namespace map
{

namespace
{
    const Colour4b RED(255, 0, 0, 1);
}

// Constructor
PointFile::PointFile() :
	_curPos(0),
    _renderable(_points)
{
    GlobalCommandSystem().addCommand(
        "NextLeakSpot", sigc::mem_fun(*this, &PointFile::nextLeakSpot)
    );
    GlobalCommandSystem().addCommand(
        "PrevLeakSpot", sigc::mem_fun(*this, &PointFile::prevLeakSpot)
    );
}

PointFile::~PointFile()
{
}

void PointFile::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloading || ev == IMap::MapSaved)
	{
        show({});
	}
}

bool PointFile::isVisible() const
{
	return !_points.empty();
}

void PointFile::show(const fs::path& pointfile)
{
	// Update the status if required
	if (!pointfile.empty())
	{
		// Parse the pointfile from disk
		parse(pointfile);

        // Construct shader if needed, and activate rendering
        auto renderSystem = GlobalMapModule().getRoot()->getRenderSystem();

        if (renderSystem)
        {
            _renderable.update(renderSystem->capture(BuiltInShaderType::PointTraceLines));
        }
	}
	else if (isVisible())
    {
        _points.clear();
        _renderable.clear();
	}

	// Regardless whether hide or show, we reset the current position
	_curPos = 0;

	// Redraw the scene
	SceneChangeNotify();
}

void PointFile::parse(const fs::path& pointfile)
{
    // Open the first pointfile and get its input stream if possible
	std::ifstream inFile(pointfile);
	if (!inFile)
	{
        throw cmd::ExecutionFailure(
            fmt::format(_("Could not open pointfile: {0}"), pointfile.string())
        );
    }

    // Construct vertices from parsed point data
    PointTrace trace(inFile);
    for (auto pos: trace.points())
    {
        _points.emplace_back(pos, RED);
    }
}

// advance camera to previous point
void PointFile::advance(bool forward)
{
	if (!isVisible())
	{
		return;
	}

	if (forward)
	{
		if (_curPos + 2 >= _points.size())
		{
			rMessage() << "End of pointfile" << std::endl;
			return;
		}

		_curPos++;
	}
	else // Backward movement
	{
		if (_curPos == 0)
		{
			rMessage() << "Start of pointfile" << std::endl;
			return;
		}

		_curPos--;
	}

	try
	{
		auto& cam = GlobalCameraManager().getActiveView();

		cam.setCameraOrigin(_points[_curPos].vertex);

		if (module::GlobalModuleRegistry().moduleExists(MODULE_ORTHOVIEWMANAGER))
		{
			GlobalXYWndManager().setOrigin(_points[_curPos].vertex);
		}

		{
			Vector3 dir((_points[_curPos + 1].vertex - cam.getCameraOrigin()).getNormalised());
			Vector3 angles(cam.getCameraAngles());

			angles[camera::CAMERA_YAW] = radians_to_degrees(atan2(dir[1], dir[0]));
			angles[camera::CAMERA_PITCH] = radians_to_degrees(asin(dir[2]));

			cam.setCameraAngles(angles);
		}

		// Redraw the scene
		SceneChangeNotify();
	}
	catch (const std::runtime_error& ex)
	{
		rError() << "Cannot set camera view position: " << ex.what() << std::endl;
	}
}

void PointFile::nextLeakSpot(const cmd::ArgumentList& args)
{
	advance(true);
}

void PointFile::prevLeakSpot(const cmd::ArgumentList& args)
{
	advance(false);
}

} // namespace map
