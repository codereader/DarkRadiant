#include "PointFile.h"

#include "i18n.h"
#include "igl.h"
#include "iscenegraph.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "itextstream.h"
#include <fstream>
#include <iostream>

#include "wxutil/dialog/MessageBox.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "map/Map.h"
#include "camera/GlobalCamera.h"
#include "camera/CamWnd.h"
#include "xyview/GlobalXYWnd.h"
#include <boost/format.hpp>

#include "modulesystem/StaticModule.h"

#ifdef MessageBox
#undef MessageBox
#endif

namespace map
{

// Constructor
PointFile::PointFile() :
	_curPos(_points.begin()),
	_displayList(0)
{}

void PointFile::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloading)
	{
		clear();
	}
	else if (ev == IMap::MapSaved)
	{
		clear();
	}
}

// Query whether the point path is currently visible
bool PointFile::isVisible() const
{
	return _displayList != 0;
}

/*
 * Toggle the status of the pointfile rendering. If the pointfile must be
 * shown, the file is parsed automatically.
 */
void PointFile::show(bool show) 
{
	// Update the status if required
	if(show && _displayList == 0)
	{
		// Parse the pointfile from disk
		parse();
		if (_points.size() > 0) {
			generateDisplayList();
		}
	}
	else if(!show && _displayList != 0) {
		glDeleteLists (_displayList, 1);
		_displayList = 0;
		_points.clear();
	}

	// Regardless whether hide or show, we reset the current position
	_curPos = _points.begin();

	// Redraw the scene
	SceneChangeNotify();
}

/*
 * OpenGL render function (back-end).
 */
void PointFile::render(const RenderInfo& info) const
{
	glCallList(_displayList);
}

/*
 * Solid renderable submission function (front-end)
 */
void PointFile::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const 
{
	if (isVisible())
	{
		collector.SetState(_renderstate, RenderableCollector::eWireframeOnly);
		collector.SetState(_renderstate, RenderableCollector::eFullMaterials);
		collector.addRenderable(*this, Matrix4::getIdentity());
	}
}

/*
 * Wireframe renderable submission function (front-end).
 */
void PointFile::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	renderSolid(collector, volume);
}

// Parse the current pointfile and read the vectors into the point list
void PointFile::parse()
{
	// Pointfile is the same as the map file but with a .lin extension
	// instead of .map
	std::string mapName = GlobalMap().getMapName();
	std::string pfName = mapName.substr(0, mapName.rfind(".")) + ".lin";

	// Open the pointfile and get its input stream if possible
	std::ifstream inFile(pfName.c_str());

	if (!inFile) 
	{
		wxutil::Messagebox::ShowError(
			(boost::format(_("Could not open pointfile: %s")) % pfName).str());
		return;
	}

	// Pointfile is a list of float vectors, one per line, with components
	// separated by spaces.
	while (inFile.good()) {
		float x, y, z;
		inFile >> x; inFile >> y; inFile >> z;
		_points.push_back(Vector3(x, y, z));
	}
}

// create the display list at the end
void PointFile::generateDisplayList() 
{
	_displayList = glGenLists(1);

	glNewList (_displayList,  GL_COMPILE);

	glBegin(GL_LINE_STRIP);
	for (VectorList::iterator i = _points.begin();
		 i != _points.end();
		 ++i)
	{
		glVertex3dv(*i);
	}
	glEnd();
	glLineWidth (1);

	glEndList();
}

// advance camera to previous point
void PointFile::advance(bool forward) 
{
	if (!isVisible()) {
		return;
	}

	if (forward) {
		if (_curPos+2 == _points.end())	{
			rMessage() << "End of pointfile" << std::endl;
			return;
		}

		_curPos++;
	}
	else {
		// Backward movement
		if (_curPos == _points.begin()) {
			rMessage() << "Start of pointfile" << std::endl;
			return;
		}

		_curPos--;
	}

	ui::CamWndPtr cam = GlobalCamera().getActiveCamWnd();
	if (cam == NULL) return;
	ui::CamWnd& camwnd = *cam;

	camwnd.setCameraOrigin(*_curPos);
	GlobalXYWnd().getActiveXY()->setOrigin(*_curPos);
	{
		Vector3 dir((*(_curPos+1) - camwnd.getCameraOrigin()).getNormalised());
		Vector3 angles(camwnd.getCameraAngles());
		angles[ui::CAMERA_YAW] = static_cast<double>(radians_to_degrees(atan2(dir[1], dir[0])));
        angles[ui::CAMERA_PITCH] = static_cast<double>(radians_to_degrees(asin(dir[2])));
		camwnd.setCameraAngles(angles);
	}

	// Redraw the scene
	SceneChangeNotify();
}

void PointFile::nextLeakSpot(const cmd::ArgumentList& args)
{
	advance(true);
}

void PointFile::prevLeakSpot(const cmd::ArgumentList& args)
{
	advance(false);
}

void PointFile::clear()
{
	show(false);
}

void PointFile::toggle(const cmd::ArgumentList& args)
{
	show(!isVisible());
}

void PointFile::registerCommands()
{
	GlobalCommandSystem().addCommand("TogglePointfile", sigc::mem_fun(*this, &PointFile::toggle));
	GlobalCommandSystem().addCommand("NextLeakSpot", sigc::mem_fun(*this, &PointFile::nextLeakSpot));
	GlobalCommandSystem().addCommand("PrevLeakSpot", sigc::mem_fun(*this, &PointFile::prevLeakSpot));

	GlobalEventManager().addCommand("TogglePointfile", "TogglePointfile");
	GlobalEventManager().addCommand("NextLeakSpot", "NextLeakSpot");
	GlobalEventManager().addCommand("PrevLeakSpot", "PrevLeakSpot");
}

// RegisterableModule implementation
const std::string& PointFile::getName() const
{
	static std::string _name("PointFile");
	return _name;
}

const StringSet& PointFile::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_MAP);
	}

	return _dependencies;
}

void PointFile::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

	registerCommands();

	_renderstate = GlobalRenderSystem().capture("$POINTFILE");

	GlobalRenderSystem().attachRenderable(*this);

	GlobalMap().signal_mapEvent().connect(sigc::mem_fun(*this, &PointFile::onMapEvent));
}

void PointFile::shutdownModule()
{
	GlobalRenderSystem().detachRenderable(*this);
	_renderstate.reset();
}

module::StaticModule<PointFile> pointFileModule;

} // namespace map
