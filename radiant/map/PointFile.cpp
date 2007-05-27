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

	// Constructor
	PointFile::PointFile()
	: _displayList(0)
	{}

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
			renderer.SetState(m_renderstate, Renderer::eWireframeOnly);
			renderer.SetState(m_renderstate, Renderer::eFullMaterials);
			renderer.addRenderable(*this, g_matrix4_identity);
		}
	}

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void PointFile::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		renderSolid(renderer, volume);
	}

  void PointFile::constructStatic()
  {
    m_renderstate = GlobalShaderCache().capture("$POINTFILE");
  }

  void PointFile::destroyStatic()
  {
    m_renderstate = ShaderPtr();
  }

// Static shader
ShaderPtr PointFile::m_renderstate;

namespace
{
  PointFile s_pointfile;
}

static PointFile::const_iterator s_check_point;

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

// old (but still relevant) pointfile code -------------------------------------

// advance camera to next point
void Pointfile_Next (void)
{
	// Return if pointfile is not visible
	if(!s_pointfile.isVisible())
    return;

	if (s_check_point+2 == s_pointfile.end())
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
  if(!s_pointfile.isVisible())
    return;

	if (s_check_point == s_pointfile.begin())
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
  s_pointfile.show(false);
}

void Pointfile_Toggle()
{
	s_pointfile.show(!s_pointfile.isVisible());

	s_check_point = s_pointfile.begin();
}

void Pointfile_Construct()
{
  PointFile::constructStatic();

  GlobalShaderCache().attachRenderable(s_pointfile);

  GlobalEventManager().addCommand("TogglePointfile", FreeCaller<Pointfile_Toggle>());
  GlobalEventManager().addCommand("NextLeakSpot", FreeCaller<Pointfile_Next>());
  GlobalEventManager().addCommand("PrevLeakSpot", FreeCaller<Pointfile_Prev>());
}

void Pointfile_Destroy()
{
  GlobalShaderCache().detachRenderable(s_pointfile);

  PointFile::destroyStatic();
}

} // namespace map
