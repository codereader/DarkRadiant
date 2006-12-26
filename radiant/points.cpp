/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
The following source code is licensed by Id Software and subject to the terms of 
its LIMITED USE SOFTWARE LICENSE AGREEMENT, a copy of which is included with 
GtkRadiant. If you did not receive a LIMITED USE SOFTWARE LICENSE AGREEMENT, 
please contact Id Software immediately at info@idsoftware.com.
*/

#include "points.h"

#include "debugging/debugging.h"

#include "irender.h"
#include "igl.h"
#include "renderable.h"

#include "gtkutil/dialog.h"

#include "map.h"
#include "qe3.h"
#include "camera/CamWnd.h"
#include "xywindow.h"
#include "mainframe.h"
#include "commands.h"

#include <iostream>
#include <fstream>

class CPointfile 
: public Renderable, 
  public OpenGLRenderable
{
	// Vector of point coordinates
	typedef std::vector<Vector3> VectorList;
	VectorList _points;
	
	// GL display list pointer for rendering the point path
	int _displayList;
	
	static Shader* m_renderstate;

private:

	// Parse the current pointfile and read the vectors into the point list
	void parse() {

		// Pointfile is the same as the map file but with a .lin extension
		// instead of .map
		std::string mapName = map::getFileName();
		std::string pfName = mapName.substr(0, mapName.rfind(".")) + ".lin";
		
		// Open the pointfile and get its input stream if possible
		std::ifstream inFile(pfName.c_str());
		if (!inFile) {
			gtkutil::errorDialog("Could not open pointfile:\n\n" + pfName);
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

public:

	// Constructor
	CPointfile()
	: _displayList(0)
	{}

	// Query whether the point path is currently visible
	bool isVisible() const {
		return _displayList != 0;
	}

  void GenerateDisplayList();
  const char* getName();

  typedef VectorList::const_iterator const_iterator;

  const_iterator begin() const
  {
    return _points.begin();
  }
  const_iterator end() const
  {
    return _points.end();
  }

	/*
	 * Toggle the status of the pointfile rendering. If the pointfile must be
	 * shown, the file is parsed automatically.
	 */
	void show(bool show) {
		
		// Update the status if required
		if(show && _displayList == 0) {
			// Parse the pointfile from disk
			parse();
			GenerateDisplayList();
		}
		else if(!show && _displayList != 0) {
			glDeleteLists (_displayList, 1);
			_displayList = 0;
		}
		
		// Redraw the scene
		SceneChangeNotify();
	}

	/*
	 * OpenGL render function (back-end).
	 */
	void render(RenderStateFlags state) const {
		glCallList(_displayList);
	}

	/*
	 * Solid renderable submission function (front-end)
	 */
	void renderSolid(Renderer& renderer, const VolumeTest& volume) const {
		if(isVisible()) {
			renderer.SetState(m_renderstate, Renderer::eWireframeOnly);
			renderer.SetState(m_renderstate, Renderer::eFullMaterials);
			renderer.addRenderable(*this, g_matrix4_identity);
		}
	}

	/*
	 * Wireframe renderable submission function (front-end).
	 */
	void renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
		renderSolid(renderer, volume);
	}

  static void constructStatic()
  {
    m_renderstate = GlobalShaderCache().capture("$POINTFILE");
  }

  static void destroyStatic()
  {
    GlobalShaderCache().release("$POINTFILE");
  }
};

Shader* CPointfile::m_renderstate = 0;

namespace
{
  CPointfile s_pointfile;
}

static CPointfile::const_iterator s_check_point;

// create the display list at the end
void CPointfile::GenerateDisplayList()
{
	_displayList = glGenLists(1);

	glNewList (_displayList,  GL_COMPILE);

	glBegin(GL_LINE_STRIP);
	for (VectorList::iterator i = _points.begin();
		 i != _points.end();
		 ++i)
	{
		glVertex3fv(*i);
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

  CPointfile::const_iterator i = ++s_check_point;


  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	camwnd.setCameraOrigin(*i);
	g_pParentWnd->GetXYWnd()->SetOrigin(*i);
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

	CPointfile::const_iterator i = --s_check_point;

  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
	camwnd.setCameraOrigin(*i);
	g_pParentWnd->GetXYWnd()->SetOrigin(*i);
  {
	  Vector3 dir((*(++i) - camwnd.getCameraOrigin()).getNormalised());
    Vector3 angles(camwnd.getCameraAngles());
	  angles[CAMERA_YAW] = static_cast<float>(radians_to_degrees(atan2(dir[1], dir[0])));
	  angles[CAMERA_PITCH] = static_cast<float>(radians_to_degrees(asin(dir[2])));
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
  CPointfile::constructStatic();

  GlobalShaderCache().attachRenderable(s_pointfile);

  GlobalCommands_insert("TogglePointfile", FreeCaller<Pointfile_Toggle>());
  GlobalCommands_insert("NextLeakSpot", FreeCaller<Pointfile_Next>(), Accelerator('K', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalCommands_insert("PrevLeakSpot", FreeCaller<Pointfile_Prev>(), Accelerator('L', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
}

void Pointfile_Destroy()
{
  GlobalShaderCache().detachRenderable(s_pointfile);

  CPointfile::destroyStatic();
}



const char* CPointfile::getName()
{
  return "Map leaked";
}
