#include "PointFile.h"

#include "igl.h"
#include "iscenegraph.h"
#include "ieventmanager.h"
#include <fstream>
#include <iostream>

#include "generic/callback.h"
#include "gtkutil/dialog.h"
#include "math/matrix.h"
#include "math/Vector3.h"
#include "map.h"
#include "mainframe.h"
#include "camera/CamWnd.h"
#include "xyview/GlobalXYWnd.h"

namespace map {

// Constructor
PointFile::PointFile()
: _displayList(0)
{}

// Static accessor method
PointFile& PointFile::Instance() {
	static PointFile _instance;
	return _instance;
}

// Query whether the point path is currently visible
bool PointFile::isVisible() const {
	return _displayList != 0;
}

  PointFile::const_iterator PointFile::begin() const
  {
    return _points.begin();
  }
  PointFile::const_iterator PointFile::end() const
  {
    return _points.end();
  }

	/*
 * Toggle the status of the pointfile rendering. If the pointfile must be
 * shown, the file is parsed automatically.
 */
void PointFile::show(bool show) {
	
	// Update the status if required
	if(show && _displayList == 0) {
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
	
	// Redraw the scene
	SceneChangeNotify();
}

/*
 * OpenGL render function (back-end).
 */
void PointFile::render(RenderStateFlags state) const {
	glCallList(_displayList);
}

/*
 * Solid renderable submission function (front-end)
 */
void PointFile::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	if(isVisible()) {
		renderer.SetState(_renderstate, Renderer::eWireframeOnly);
		renderer.SetState(_renderstate, Renderer::eFullMaterials);
		renderer.addRenderable(*this, g_matrix4_identity);
	}
}

/*
 * Wireframe renderable submission function (front-end).
 */
void PointFile::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	renderSolid(renderer, volume);
}

// Parse the current pointfile and read the vectors into the point list
void PointFile::parse() {

	// Pointfile is the same as the map file but with a .lin extension
	// instead of .map
	std::string mapName = map::getFileName();
	std::string pfName = mapName.substr(0, mapName.rfind(".")) + ".lin";
	
	// Open the pointfile and get its input stream if possible
	std::ifstream inFile(pfName.c_str());
	if (!inFile) {
		gtkutil::errorDialog("Could not open pointfile:\n\n" + pfName,
							 MainFrame_getWindow());
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
void PointFile::generateDisplayList() {
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

  void PointFile::constructStatic()
  {
    _renderstate = GlobalShaderCache().capture("$POINTFILE");
  }

  void PointFile::destroyStatic()
  {
    _renderstate = ShaderPtr();
  }

// Static shader
ShaderPtr PointFile::_renderstate;

namespace
{
  //PointFile PointFile::Instance();
}

static PointFile::const_iterator s_check_point;

// old (but still relevant) pointfile code -------------------------------------

// advance camera to next point
void Pointfile_Next (void)
{
	// Return if pointfile is not visible
	if(!PointFile::Instance().isVisible())
    return;

	if (s_check_point+2 == PointFile::Instance().end())
	{
		globalOutputStream() << "End of pointfile\n";
		return;
	}

  PointFile::const_iterator i = ++s_check_point;


  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	camwnd.setCameraOrigin(*i);
	GlobalXYWnd().getActiveXY()->setOrigin(*i);
  {
	  Vector3 dir((*(++i) - camwnd.getCameraOrigin()).getNormalised());
    Vector3 angles(camwnd.getCameraAngles());
	  angles[CAMERA_YAW] = static_cast<float>(radians_to_degrees(atan2(dir[1], dir[0])));
	  angles[CAMERA_PITCH] = static_cast<float>(radians_to_degrees(asin(dir[2])));
    camwnd.setCameraAngles(angles);
  }
}

// advance camera to previous point
void Pointfile_Prev (void)
{
  if(!PointFile::Instance().isVisible())
    return;

	if (s_check_point == PointFile::Instance().begin())
	{
		globalOutputStream() << "Start of pointfile\n";
		return;
	}

	PointFile::const_iterator i = --s_check_point;

  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	camwnd.setCameraOrigin(*i);
	GlobalXYWnd().getActiveXY()->setOrigin(*i);
  {
	  Vector3 dir((*(++i) - camwnd.getCameraOrigin()).getNormalised());
    Vector3 angles(camwnd.getCameraAngles());
	  angles[CAMERA_YAW] = static_cast<double>(radians_to_degrees(atan2(dir[1], dir[0])));
	  angles[CAMERA_PITCH] = static_cast<double>(radians_to_degrees(asin(dir[2])));
    camwnd.setCameraAngles(angles);
  }
}

void Pointfile_Clear()
{
  PointFile::Instance().show(false);
}

void Pointfile_Toggle()
{
	PointFile::Instance().show(!PointFile::Instance().isVisible());

	s_check_point = PointFile::Instance().begin();
}

void Pointfile_Construct()
{
  PointFile::constructStatic();

  GlobalShaderCache().attachRenderable(PointFile::Instance());

  GlobalEventManager().addCommand("TogglePointfile", FreeCaller<Pointfile_Toggle>());
  GlobalEventManager().addCommand("NextLeakSpot", FreeCaller<Pointfile_Next>());
  GlobalEventManager().addCommand("PrevLeakSpot", FreeCaller<Pointfile_Prev>());
}

void Pointfile_Destroy()
{
  GlobalShaderCache().detachRenderable(PointFile::Instance());

  PointFile::destroyStatic();
}

} // namespace map
