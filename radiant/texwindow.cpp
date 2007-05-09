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

//
// Texture Window
//
// Leonardo Zide (leo@lokigames.com)
//

#include "texwindow.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include "ieventmanager.h"
#include "iuimanager.h"
#include "iimage.h"
#include "ifilesystem.h"
#include "ishaders.h"
#include "iscriplib.h"
#include "iselection.h"
#include "iscenegraph.h"
#include "irender.h"
#include "iundo.h"
#include "igl.h"
#include "iarchive.h"
#include "moduleobserver.h"

#include <set>

#include <gtk/gtk.h>

#include "signal/signal.h"
#include "texturelib.h"
#include "string/string.h"
#include "shaderlib.h"
#include "os/path.h"
#include "stream/memstream.h"
#include "stream/textfilestream.h"
#include "stream/stringstream.h"
#include "cmdlib.h"
#include "convert.h"

#include "gtkutil/image.h"
#include "gtkutil/nonmodal.h"
#include "gtkutil/cursor.h"
#include "gtkutil/widget.h"
#include "gtkutil/glwidget.h"

#include "error.h"
#include "map.h"
#include "qgl.h"
#include "select.h"
#include "brush/TextureProjection.h"
#include "brushmanip.h"
#include "patchmanip.h"
#include "plugin.h"
#include "qe3.h"
#include "gtkdlgs.h"
#include "gtkmisc.h"
#include "mainframe.h"
#include "ui/groupdialog/GroupDialog.h"
#include "preferences.h"
#include "selection/algorithm/Shader.h"
#include "settings/GameManager.h"

void TextureBrowser_queueDraw(TextureBrowser& textureBrowser);

namespace {
	const std::string RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
	const std::string RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
	const std::string RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
	const std::string RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
	const std::string RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
	const std::string RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
	
	bool g_TexturesMenu_shaderlistOnly = false;
}

DeferredAdjustment::DeferredAdjustment(ValueChangedFunction function, void* data) : 
	m_value(0), 
	m_handler(0), 
	m_function(function), 
	m_data(data)
{}

void DeferredAdjustment::flush() {
	if (m_handler != 0) {
		g_source_remove(m_handler);
		deferred_value_changed(this);
	}
}

void DeferredAdjustment::value_changed(gdouble value) {
	m_value = value;
	if (m_handler == 0) {
		m_handler = g_idle_add(deferred_value_changed, this);
	}
}

void DeferredAdjustment::adjustment_value_changed(GtkAdjustment *adjustment, DeferredAdjustment* self) {
	self->value_changed(adjustment->value);
}

gboolean DeferredAdjustment::deferred_value_changed(gpointer data) {
	DeferredAdjustment* self = reinterpret_cast<DeferredAdjustment*>(data);
	
	self->m_function(self->m_data, self->m_value);
	self->m_handler = 0;
	self->m_value = 0;

	return FALSE;
}

// ---- TextureBrowser implementation ------------------------------------------

TextureBrowser::TextureBrowser() :
	m_filter(0),
	m_filterEntry(TextureBrowserQueueDrawCaller(*this), ClearFilterCaller(*this)),
	m_texture_scroll(0),
	m_heightChanged(true),
	m_originInvalid(true),
	m_scrollAdjustment(scrollChanged, this),
	m_mouseWheelScrollIncrement(64),
	m_textureScale(50),
	m_showTextureFilter(false),
	m_showShaders(true),
	m_showTextureScrollbar(true),
	m_hideUnused(false),
	m_resizeTextures(true),
	m_uniformTextureSize(128)
{}

void TextureBrowser::construct() {
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_HIDE_UNUSED);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SCALE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_UNIFORM_SIZE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_SCROLLBAR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_FILTER);
			
	m_hideUnused = (GlobalRegistry().get(RKEY_TEXTURES_HIDE_UNUSED) == "1");
	m_showTextureFilter = (GlobalRegistry().get(RKEY_TEXTURE_SHOW_FILTER) == "1");
	m_uniformTextureSize = GlobalRegistry().getInt(RKEY_TEXTURE_UNIFORM_SIZE);
	m_showTextureScrollbar = (GlobalRegistry().get(RKEY_TEXTURE_SHOW_SCROLLBAR) == "1");
	m_mouseWheelScrollIncrement = GlobalRegistry().getInt(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	
	setScaleFromRegistry();
}

void TextureBrowser::destroy() {
}

void TextureBrowser::queueDraw() {
	if (m_gl_widget != NULL) {
		gtk_widget_queue_draw(m_gl_widget);
	}
}

void TextureBrowser::textureModeChanged() {
	queueDraw();
}

void TextureBrowser::setScaleFromRegistry() {
	int index = GlobalRegistry().getInt(RKEY_TEXTURE_SCALE);
	
	switch (index) {
		case 0: m_textureScale = 10; break;
		case 1: m_textureScale = 25; break;
		case 2: m_textureScale = 50; break;
		case 3: m_textureScale = 100; break;
		case 4: m_textureScale = 200; break;
	};
	
	queueDraw();
}

void TextureBrowser::clearFilter() {
	gtk_entry_set_text(m_filter, "");
	TextureBrowser_queueDraw(*this);
}

void TextureBrowser::keyChanged() {
	m_hideUnused = (GlobalRegistry().get(RKEY_TEXTURES_HIDE_UNUSED) == "1");
	m_showTextureFilter = (GlobalRegistry().get(RKEY_TEXTURE_SHOW_FILTER) == "1");
	m_uniformTextureSize = GlobalRegistry().getInt(RKEY_TEXTURE_UNIFORM_SIZE);
	m_showTextureScrollbar = (GlobalRegistry().get(RKEY_TEXTURE_SHOW_SCROLLBAR) == "1");
	m_mouseWheelScrollIncrement = GlobalRegistry().getInt(RKEY_TEXTURE_MOUSE_WHEEL_INCR);
  	
	widget_set_visible(m_texture_scroll, m_showTextureScrollbar);
	widget_set_visible(GTK_WIDGET(m_filter), m_showTextureFilter);
  	
	setScaleFromRegistry();
  	
	heightChanged();
	m_originInvalid = true;
}

// Return the display width of a texture in the texture browser
int TextureBrowser::getTextureWidth(TexturePtr tex) {
    int width;
    if (!m_resizeTextures) {
		// Don't use uniform size
		width = (int)(tex->width * ((float)m_textureScale / 100));
	}
	else if (tex->width >= tex->height) {
		// Texture is square, or wider than it is tall
		width = m_uniformTextureSize;
	}
	else {
		// Otherwise, preserve the texture's aspect ratio
		width = (int)(m_uniformTextureSize * ((float)tex->width / tex->height));
	}
    
    return width;
}

int TextureBrowser::getTextureHeight(TexturePtr tex) {
	int height;
	if (!m_resizeTextures) {
		// Don't use uniform size
		height = (int)(tex->height * ((float)m_textureScale / 100));
	}
    else if (tex->height >= tex->width) {
		// Texture is square, or taller than it is wide
		height = m_uniformTextureSize;
	}
	else {
		// Otherwise, preserve the texture's aspect ratio
		height = (int)(m_uniformTextureSize * ((float)tex->height / tex->width));
	}
    
	return height;
}

void TextureBrowser_updateScroll(TextureBrowser& textureBrowser);

const char* TextureBrowser_getComonShadersDir()
{
  const char* value = game::Manager::Instance().currentGame()->getKeyValue("common_shaders_dir");
  if(!string_empty(value))
  {
    return value;
  }
  return "common/";
}

const char* TextureBrowser_getFilter(TextureBrowser& textureBrowser)
{
  if(textureBrowser.m_showTextureFilter)
  {
    return gtk_entry_get_text(textureBrowser.m_filter);
  }
  return 0;
}

inline int TextureBrowser_fontHeight(TextureBrowser& textureBrowser)
{
  return GlobalOpenGL().m_fontHeight;
}

const char* TextureBrowser_GetSelectedShader(TextureBrowser& textureBrowser)
{
  return textureBrowser.shader.c_str();
}

void TextureBrowser_SetStatus(TextureBrowser& textureBrowser, const char* name)
{
	// greebo: Disabled this, the status bar reflects the shaderclipboard now
	return;
  IShaderPtr shader = QERApp_Shader_ForName( name);
  TexturePtr q = shader->getTexture();
  StringOutputStream strTex(256);
  strTex << name << " W: " << Unsigned(q->width) << " H: " << Unsigned(q->height);
  g_pParentWnd->SetStatusText(g_pParentWnd->m_texture_status, strTex.c_str());
}

void TextureBrowser_Focus(TextureBrowser& textureBrowser, const char* name);

void TextureBrowser_SetSelectedShader(TextureBrowser& textureBrowser, const std::string& shader)
{
  textureBrowser.shader = shader.c_str();
  TextureBrowser_SetStatus(textureBrowser, shader.c_str());
  TextureBrowser_Focus(textureBrowser, shader.c_str());
}


std::string g_TextureBrowser_currentDirectory;

/*
============================================================================

TEXTURE LAYOUT

TTimo: now based on a rundown through all the shaders
NOTE: we expect the Active shaders count doesn't change during a Texture_StartPos .. Texture_NextPos cycle
  otherwise we may need to rely on a list instead of an array storage
============================================================================
*/

class TextureLayout
{
public:
  // texture layout functions
  // TTimo: now based on shaders
  int current_x, current_y, current_row;
};

void Texture_StartPos(TextureLayout& layout)
{
  layout.current_x = 8;
  layout.current_y = -8;
  layout.current_row = 0;
}

void Texture_NextPos(TextureBrowser& textureBrowser, TextureLayout& layout, TexturePtr current_texture, int *x, int *y)
{
  TexturePtr q = current_texture;

  int nWidth = textureBrowser.getTextureWidth(q);
  int nHeight = textureBrowser.getTextureHeight(q);
  if (layout.current_x + nWidth > textureBrowser.width-8 && layout.current_row)
  { // go to the next row unless the texture is the first on the row
    layout.current_x = 8;
    layout.current_y -= layout.current_row + TextureBrowser_fontHeight(textureBrowser) + 4;
    layout.current_row = 0;
  }

  *x = layout.current_x;
  *y = layout.current_y;

  // Is our texture larger than the row? If so, grow the
  // row height to match it

  if (layout.current_row < nHeight)
    layout.current_row = nHeight;

  // never go less than 96, or the names get all crunched up
  layout.current_x += nWidth < 96 ? 96 : nWidth;
  layout.current_x += 8;
}

// if texture_showinuse jump over non in-use textures
bool Texture_IsShown(IShaderPtr shader, bool show_shaders, bool hideUnused, const char* filter)
{
  if(!shader_equal_prefix(shader->getName(), "textures/"))
    return false;

  if (!show_shaders && !shader->IsDefault())
    return false;

  if(hideUnused && !shader->IsInUse())
    return false;

  if(!string_empty(g_TextureBrowser_currentDirectory.c_str()))
  {
    if(!shader_equal_prefix(shader_get_textureName(shader->getName()), g_TextureBrowser_currentDirectory.c_str()))
    {
      return false;
    }
  }

  if (filter != 0)
  {
    // some basic filtering
    if (strstr( shader_get_textureName(shader->getName()), filter ) == 0)
      return false;
  }

  return true;
}

void TextureBrowser::heightChanged() {
	m_heightChanged = true;

	TextureBrowser_updateScroll(*this);
	TextureBrowser_queueDraw(*this);
}

void TextureBrowser_evaluateHeight(TextureBrowser& textureBrowser)
{
  if(textureBrowser.m_heightChanged)
  {
    textureBrowser.m_heightChanged = false;

    textureBrowser.m_nTotalHeight = 0;

    TextureLayout layout;
    Texture_StartPos(layout);
    for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
    {
      IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

      if(!Texture_IsShown(shader, textureBrowser.m_showShaders, textureBrowser.m_hideUnused, TextureBrowser_getFilter(textureBrowser)))
        continue;

      int   x, y;
      Texture_NextPos(textureBrowser, layout, shader->getTexture(), &x, &y);
      textureBrowser.m_nTotalHeight = std::max(textureBrowser.m_nTotalHeight, abs(layout.current_y) + TextureBrowser_fontHeight(textureBrowser) + textureBrowser.getTextureHeight(shader->getTexture()) + 4);
    }
  }
}

int TextureBrowser_TotalHeight(TextureBrowser& textureBrowser)
{
  TextureBrowser_evaluateHeight(textureBrowser);
  return textureBrowser.m_nTotalHeight;
}

inline const int& min_int(const int& left, const int& right)
{
  return std::min(left, right);
}

void TextureBrowser_clampOriginY(TextureBrowser& textureBrowser)
{
  if(textureBrowser.originy > 0)
  {
    textureBrowser.originy = 0;
  }
  int lower = min_int(textureBrowser.height - TextureBrowser_TotalHeight(textureBrowser), 0);
  if(textureBrowser.originy < lower)
  {
    textureBrowser.originy = lower;
  }
}

int TextureBrowser_getOriginY(TextureBrowser& textureBrowser)
{
  if(textureBrowser.m_originInvalid)
  {
    textureBrowser.m_originInvalid = false;
    TextureBrowser_clampOriginY(textureBrowser);
    TextureBrowser_updateScroll(textureBrowser);
  }
  return textureBrowser.originy;
}

void TextureBrowser_setOriginY(TextureBrowser& textureBrowser, int originy)
{
  textureBrowser.originy = originy;
  TextureBrowser_clampOriginY(textureBrowser);
  TextureBrowser_updateScroll(textureBrowser);
  TextureBrowser_queueDraw(textureBrowser);
}


Signal0 g_activeShadersChangedCallbacks;

void TextureBrowser_addActiveShadersChangedCallback(const SignalHandler& handler)
{
  g_activeShadersChangedCallbacks.connectLast(handler);
}

class ShadersObserver : public ModuleObserver
{
  Signal0 m_realiseCallbacks;
public:
  void realise()
  {
    m_realiseCallbacks();
  }
  void unrealise()
  {
  }
  void insert(const SignalHandler& handler)
  {
    m_realiseCallbacks.connectLast(handler);
  }
};

namespace
{
  ShadersObserver g_ShadersObserver;
}

void TextureBrowser_addShadersRealiseCallback(const SignalHandler& handler)
{
  g_ShadersObserver.insert(handler);
}

void TextureBrowser_activeShadersChanged(TextureBrowser& textureBrowser)
{
  textureBrowser.heightChanged();
  textureBrowser.m_originInvalid = true;

  g_activeShadersChangedCallbacks();
}

/*
==============
TextureBrowser_ShowDirectory
relies on texture_directory global for the directory to use
1) Load the shaders for the given directory
2) Scan the remaining texture, load them and assign them a default shader (the "noshader" shader)
NOTE: when writing a texture plugin, or some texture extensions, this function may need to be overriden, and made
  available through the IShaders interface
NOTE: for texture window layout:
  all shaders are stored with alphabetical order after load
  previously loaded and displayed stuff is hidden, only in-use and newly loaded is shown
  ( the GL textures are not flushed though)
==============
*/
bool texture_name_ignore(const char* name)
{
  StringOutputStream strTemp(string_length(name));
  strTemp << LowerCase(name);

  return strstr(strTemp.c_str(), ".specular") != 0 ||
    strstr(strTemp.c_str(), ".glow") != 0 ||
    strstr(strTemp.c_str(), ".bump") != 0 ||
    strstr(strTemp.c_str(), ".diffuse") != 0 ||
    strstr(strTemp.c_str(), ".blend") != 0 ||
	  strstr(strTemp.c_str(), ".alpha") != 0;
}

//void TextureBrowser_SetHideUnused(TextureBrowser& textureBrowser, bool hideUnused);

void TextureBrowser_toggleShown() {
	ui::GroupDialog::Instance().setPage("textures");
}

class TextureCategoryLoadShader
{
  const char* m_directory;
  std::size_t& m_count;
public:
  typedef const char* first_argument_type;

  TextureCategoryLoadShader(const char* directory, std::size_t& count)
    : m_directory(directory), m_count(count)
  {
    m_count = 0;
  }
  void operator()(const char* name) const
  {
    if(shader_equal_prefix(name, "textures/")
      && shader_equal_prefix(name + string_length("textures/"), m_directory))
    {
      ++m_count;
      // request the shader, this will load the texture if needed
      // this Shader_ForName call is a kind of hack
      IShaderPtr pFoo = QERApp_Shader_ForName(name);
    }
  }
};

void TextureDirectory_loadTexture(const char* directory, const char* texture)
{
  StringOutputStream name(256);
  name << directory << StringRange(texture, path_get_filename_base_end(texture));

  if(texture_name_ignore(name.c_str()))
  {
    return;
  }

  if (!shader_valid(name.c_str()))
  {
    globalOutputStream() << "Skipping invalid texture name: [" << name.c_str() << "]\n";
    return;
  }

  // if a texture is already in use to represent a shader, ignore it
  IShaderPtr shader = QERApp_Shader_ForName(name.c_str());
}
typedef ConstPointerCaller1<char, const char*, TextureDirectory_loadTexture> TextureDirectoryLoadTextureCaller;

class LoadTexturesByTypeVisitor : public ImageModules::Visitor
{
  const char* m_dirstring;
public:
  LoadTexturesByTypeVisitor(const char* dirstring)
    : m_dirstring(dirstring)
  {
  }
  void visit(const char* minor, const _QERPlugImageTable& table)
  {
    GlobalFileSystem().forEachFile(m_dirstring, minor, TextureDirectoryLoadTextureCaller(m_dirstring));
  }
};

void TextureBrowser_showShadersExport(const BoolImportCallback& importer)
{
  importer(GlobalTextureBrowser().m_showShaders);
}
typedef FreeCaller1<const BoolImportCallback&, TextureBrowser_showShadersExport> TextureBrowserShowShadersExport;

void TextureBrowser_showShaderlistOnly(const BoolImportCallback& importer)
{
  importer(g_TexturesMenu_shaderlistOnly);
}
typedef FreeCaller1<const BoolImportCallback&, TextureBrowser_showShaderlistOnly> TextureBrowserShowShaderlistOnlyExport;

class TexturesMenu
{
public:
  ToggleItem m_showshaders_item;
  ToggleItem m_showshaderlistonly_item;

  TexturesMenu() :
    m_showshaders_item(TextureBrowserShowShadersExport()),
    m_showshaderlistonly_item(TextureBrowserShowShaderlistOnlyExport())
  {
  }
};

TexturesMenu g_TexturesMenu;

//++timo NOTE: this is a mix of Shader module stuff and texture explorer
// it might need to be split in parts or moved out .. dunno
// scroll origin so the specified texture is completely on screen
// if current texture is not displayed, nothing is changed
void TextureBrowser_Focus(TextureBrowser& textureBrowser, const char* name)
{
  TextureLayout layout;
  // scroll origin so the texture is completely on screen
  Texture_StartPos(layout);
  
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if(!Texture_IsShown(shader, textureBrowser.m_showShaders, textureBrowser.m_hideUnused, TextureBrowser_getFilter(textureBrowser)))
      continue;

    int x, y;
    Texture_NextPos(textureBrowser, layout, shader->getTexture(), &x, &y);
    TexturePtr q = shader->getTexture();
    if (!q)
      break;

    // we have found when texdef->name and the shader name match
    // NOTE: as everywhere else for our comparisons, we are not case sensitive
    if (shader_equal(name, shader->getName()))
    {
      int textureHeight = textureBrowser.getTextureHeight(q)
        + 2 * TextureBrowser_fontHeight(textureBrowser);

      int originy = TextureBrowser_getOriginY(textureBrowser);
      if (y > originy)
      {
        originy = y;
      }

      if (y - textureHeight < originy - textureBrowser.height)
      {
        originy = (y - textureHeight) + textureBrowser.height;
      }

      TextureBrowser_setOriginY(textureBrowser, originy);
      return;
    }
  }
}

// greebo: Workaround for being unable to return NULL (Texture_At function)
class NoTextureSelectedException
: public std::runtime_error
{
public:
	// Constructor
	NoTextureSelectedException(const std::string& what)
	: std::runtime_error(what) {}
};

IShaderPtr Texture_At(TextureBrowser& textureBrowser, int mx, int my)
{
  my += TextureBrowser_getOriginY(textureBrowser) - textureBrowser.height;

  TextureLayout layout;
  Texture_StartPos(layout);
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if(!Texture_IsShown(shader, textureBrowser.m_showShaders, textureBrowser.m_hideUnused, TextureBrowser_getFilter(textureBrowser)))
      continue;

    int   x, y;
    Texture_NextPos(textureBrowser, layout, shader->getTexture(), &x, &y);
    TexturePtr q = shader->getTexture();
    if (!q)
      break;

    int nWidth = textureBrowser.getTextureWidth(q);
    int nHeight = textureBrowser.getTextureHeight(q);
    if (mx > x && mx - x < nWidth
      && my < y && y - my < nHeight + TextureBrowser_fontHeight(textureBrowser))
    {
      return shader;
    }
  }

  throw NoTextureSelectedException("no texture selected");
}

/*
==============
SelectTexture

  By mouse click
==============
*/
void SelectTexture(TextureBrowser& textureBrowser, int mx, int my, bool bShift)
{
	try {
  		IShaderPtr shader = Texture_At(textureBrowser, mx, my);
  		TextureBrowser_SetSelectedShader(textureBrowser, shader->getName());
  		
		// Apply the shader to the current selection
		selection::algorithm::applyShaderToSelection(shader->getName());
	}
	catch (NoTextureSelectedException n) {
		
	}
}

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

void TextureBrowser_trackingDelta(int x, int y, unsigned int state, void* data)
{
  TextureBrowser& textureBrowser = *reinterpret_cast<TextureBrowser*>(data);
  if(y != 0)
  {
    int scale = 1;

    if(state & GDK_SHIFT_MASK)
      scale = 4;

    int originy = TextureBrowser_getOriginY(textureBrowser);
    originy += y * scale;
    TextureBrowser_setOriginY(textureBrowser, originy);
  }
}

void TextureBrowser_Tracking_MouseDown(TextureBrowser& textureBrowser)
{
  textureBrowser.m_freezePointer.freeze_pointer(textureBrowser.m_parent, TextureBrowser_trackingDelta, &textureBrowser);
}

void TextureBrowser_Tracking_MouseUp(TextureBrowser& textureBrowser)
{
  textureBrowser.m_freezePointer.unfreeze_pointer(textureBrowser.m_parent);
}

void TextureBrowser_Selection_MouseDown(TextureBrowser& textureBrowser, guint32 flags, int pointx, int pointy)
{
  SelectTexture(textureBrowser, pointx, textureBrowser.height - 1 - pointy, (flags & GDK_SHIFT_MASK) != 0);
}

/*
============================================================================

DRAWING

============================================================================
*/

/*
============
Texture_Draw
TTimo: relying on the shaders list to display the textures
we must query all Texture* to manage and display through the IShaders interface
this allows a plugin to completely override the texture system
============
*/
void Texture_Draw(TextureBrowser& textureBrowser)
{
  int originy = TextureBrowser_getOriginY(textureBrowser);

  Vector3 colorBackground = ColourSchemes().getColourVector3("texture_background");
  glClearColor(colorBackground[0], colorBackground[1], colorBackground[2], 0);
  glViewport(0, 0, textureBrowser.width, textureBrowser.height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable (GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glOrtho (0, textureBrowser.width, originy-textureBrowser.height, originy, -100, 100);
  glEnable (GL_TEXTURE_2D);

  glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

  int last_y = 0, last_height = 0;

  TextureLayout layout;
  Texture_StartPos(layout);
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if(!Texture_IsShown(shader, textureBrowser.m_showShaders, textureBrowser.m_hideUnused, TextureBrowser_getFilter(textureBrowser)))
      continue;

    int x, y;
    Texture_NextPos(textureBrowser, layout, shader->getTexture(), &x, &y);
    TexturePtr q = shader->getTexture();
    if (!q)
      break;

    int nWidth = textureBrowser.getTextureWidth(q);
    int nHeight = textureBrowser.getTextureHeight(q);

    if (y != last_y)
    {
      last_y = y;
      last_height = 0;
    }
    last_height = std::max (nHeight, last_height);

    // Is this texture visible?
    if ((y-nHeight-TextureBrowser_fontHeight(textureBrowser) < originy)
        && (y > originy - textureBrowser.height))
    {
      // borders rules:
      // if it's the current texture, draw a thick red line, else:
      // shaders have a white border, simple textures don't
      // if !texture_showinuse: (some textures displayed may not be in use)
      // draw an additional square around with 0.5 1 0.5 color
      if (shader_equal(TextureBrowser_GetSelectedShader(textureBrowser), shader->getName()))
      {
	      glLineWidth (3);
	      glColor3f (1,0,0);
	      glDisable (GL_TEXTURE_2D);

	      glBegin (GL_LINE_LOOP);
	      glVertex2i (x-4,y-TextureBrowser_fontHeight(textureBrowser)+4);
	      glVertex2i (x-4,y-TextureBrowser_fontHeight(textureBrowser)-nHeight-4);
	      glVertex2i (x+4+nWidth,y-TextureBrowser_fontHeight(textureBrowser)-nHeight-4);
	      glVertex2i (x+4+nWidth,y-TextureBrowser_fontHeight(textureBrowser)+4);
	      glEnd();

	      glEnable (GL_TEXTURE_2D);
	      glLineWidth (1);
      }
      else
      {
	      glLineWidth (1);
	      // shader border:
	      if (!shader->IsDefault())
	      {
	        glColor3f (1,1,1);
	        glDisable (GL_TEXTURE_2D);

	        glBegin (GL_LINE_LOOP);
	        glVertex2i (x-1,y+1-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x-1,y-nHeight-1-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x+1+nWidth,y-nHeight-1-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x+1+nWidth,y+1-TextureBrowser_fontHeight(textureBrowser));
	        glEnd();
	        glEnable (GL_TEXTURE_2D);
	      }

	      // highlight in-use textures
	      if (!textureBrowser.m_hideUnused && shader->IsInUse())
	      {
	        glColor3f (0.5,1,0.5);
	        glDisable (GL_TEXTURE_2D);
	        glBegin (GL_LINE_LOOP);
	        glVertex2i (x-3,y+3-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x-3,y-nHeight-3-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x+3+nWidth,y-nHeight-3-TextureBrowser_fontHeight(textureBrowser));
	        glVertex2i (x+3+nWidth,y+3-TextureBrowser_fontHeight(textureBrowser));
	        glEnd();
	        glEnable (GL_TEXTURE_2D);
	      }
      }

      // Draw the texture
      glBindTexture (GL_TEXTURE_2D, q->texture_number);
      GlobalOpenGL_debugAssertNoErrors();
      glColor3f (1,1,1);
      glBegin (GL_QUADS);
      glTexCoord2i (0,0);
      glVertex2i (x,y-TextureBrowser_fontHeight(textureBrowser));
      glTexCoord2i (1,0);
      glVertex2i (x+nWidth,y-TextureBrowser_fontHeight(textureBrowser));
      glTexCoord2i (1,1);
      glVertex2i (x+nWidth,y-TextureBrowser_fontHeight(textureBrowser)-nHeight);
      glTexCoord2i (0,1);
      glVertex2i (x,y-TextureBrowser_fontHeight(textureBrowser)-nHeight);
      glEnd();

      // draw the texture name
      glDisable (GL_TEXTURE_2D);
      glColor3f (1,1,1);

      glRasterPos2i (x, y-TextureBrowser_fontHeight(textureBrowser)+5);

      // don't draw the directory name
      const char* name = shader->getName();
      name += strlen(name);
      while(name != shader->getName() && *(name-1) != '/' && *(name-1) != '\\')
        name--;

      GlobalOpenGL().drawString(name);
      glEnable (GL_TEXTURE_2D);
    }

    //int totalHeight = abs(y) + last_height + TextureBrowser_fontHeight(textureBrowser) + 4;
  }


  // reset the current texture
  glBindTexture(GL_TEXTURE_2D, 0);
  //qglFinish();
}

void TextureBrowser_queueDraw(TextureBrowser& textureBrowser) {
	textureBrowser.queueDraw();
}

void TextureBrowser_MouseWheel(TextureBrowser& textureBrowser, bool bUp)
{
  int originy = TextureBrowser_getOriginY(textureBrowser);

  if (bUp)
  {
    originy += int(textureBrowser.m_mouseWheelScrollIncrement);
  }
  else
  {
    originy -= int(textureBrowser.m_mouseWheelScrollIncrement);
  }

  TextureBrowser_setOriginY(textureBrowser, originy);
}


// GTK callback for toggling uniform texture sizing
void TextureBrowser_toggleResizeTextures(GtkToggleToolButton* button, TextureBrowser* textureBrowser) {
  if (gtk_toggle_tool_button_get_active(button) == TRUE)
  {
    textureBrowser->m_resizeTextures = true;
  }
  else
  {
    textureBrowser->m_resizeTextures = false;
  }
  
  // Update texture browser
  textureBrowser->heightChanged();
}


gboolean TextureBrowser_button_press(GtkWidget* widget, GdkEventButton* event, TextureBrowser* textureBrowser)
{
  if(event->type == GDK_BUTTON_PRESS)
  {
    if(event->button == 3)
    {
      TextureBrowser_Tracking_MouseDown(*textureBrowser);
    }
    else if(event->button == 1)
    {
      TextureBrowser_Selection_MouseDown(*textureBrowser, event->state, static_cast<int>(event->x), static_cast<int>(event->y));
    }
  }
  return FALSE;
}

gboolean TextureBrowser_button_release(GtkWidget* widget, GdkEventButton* event, TextureBrowser* textureBrowser)
{
  if(event->type == GDK_BUTTON_RELEASE)
  {
    if(event->button == 3)
    {
      TextureBrowser_Tracking_MouseUp(*textureBrowser);
    }
  }
  return FALSE;
}

gboolean TextureBrowser_motion(GtkWidget *widget, GdkEventMotion *event, TextureBrowser* textureBrowser)
{
  return FALSE;
}

gboolean TextureBrowser_scroll(GtkWidget* widget, GdkEventScroll* event, TextureBrowser* textureBrowser)
{
  if(event->direction == GDK_SCROLL_UP)
  {
    TextureBrowser_MouseWheel(*textureBrowser, true);
  }
  else if(event->direction == GDK_SCROLL_DOWN)
  {
    TextureBrowser_MouseWheel(*textureBrowser, false);
  }
  return FALSE;
}

void TextureBrowser::scrollChanged(void* data, gdouble value) {
	//globalOutputStream() << "vertical scroll\n";
	TextureBrowser_setOriginY(*reinterpret_cast<TextureBrowser*>(data), -(int)value);
}

static void TextureBrowser_verticalScroll(GtkAdjustment *adjustment, TextureBrowser* textureBrowser)
{
  textureBrowser->m_scrollAdjustment.value_changed(adjustment->value);
}

void TextureBrowser_updateScroll(TextureBrowser& textureBrowser)
{
  if(textureBrowser.m_showTextureScrollbar)
  {
    int totalHeight = TextureBrowser_TotalHeight(textureBrowser);

    totalHeight = std::max(totalHeight, textureBrowser.height);

    GtkAdjustment *vadjustment = gtk_range_get_adjustment(GTK_RANGE(textureBrowser.m_texture_scroll));

    vadjustment->value = -TextureBrowser_getOriginY(textureBrowser);
    vadjustment->page_size = textureBrowser.height;
    vadjustment->page_increment = textureBrowser.height/2;
    vadjustment->step_increment = 20;
    vadjustment->lower = 0;
    vadjustment->upper = totalHeight;

    g_signal_emit_by_name(G_OBJECT (vadjustment), "changed");
  }
}

gboolean TextureBrowser_size_allocate(GtkWidget* widget, GtkAllocation* allocation, TextureBrowser* textureBrowser)
{
  textureBrowser->width = allocation->width;
  textureBrowser->height = allocation->height;
  textureBrowser->heightChanged();
  textureBrowser->m_originInvalid = true;
  TextureBrowser_queueDraw(*textureBrowser);
  return FALSE;
}

gboolean TextureBrowser_expose(GtkWidget* widget, GdkEventExpose* event, TextureBrowser* textureBrowser)
{
  if(glwidget_make_current(textureBrowser->m_gl_widget) != FALSE)
  {
    GlobalOpenGL_debugAssertNoErrors();
    TextureBrowser_evaluateHeight(*textureBrowser);
    Texture_Draw(*textureBrowser);
    GlobalOpenGL_debugAssertNoErrors();
    glwidget_swap_buffers(textureBrowser->m_gl_widget);
  }
  return FALSE;
}


TextureBrowser g_TextureBrowser;

TextureBrowser& GlobalTextureBrowser()
{
  return g_TextureBrowser;
}

GtkWidget* TextureBrowser_constructWindow(GtkWindow* toplevel)
{
	g_TextureBrowser.construct();
	
  GlobalShaderSystem().setActiveShadersChangedNotify(ReferenceCaller<TextureBrowser, TextureBrowser_activeShadersChanged>(g_TextureBrowser));

  GtkWidget* hbox = gtk_hbox_new (FALSE, 0);

  g_TextureBrowser.m_parent = toplevel;

  {
	  GtkWidget* w = gtk_vscrollbar_new (GTK_ADJUSTMENT (gtk_adjustment_new (0,0,0,1,1,1)));
	  gtk_widget_show (w);
	  gtk_box_pack_end (GTK_BOX (hbox), w, FALSE, TRUE, 0);
	  g_TextureBrowser.m_texture_scroll = w;

    GtkAdjustment *vadjustment = gtk_range_get_adjustment (GTK_RANGE (g_TextureBrowser.m_texture_scroll));
    g_signal_connect(G_OBJECT(vadjustment), "value_changed", G_CALLBACK(TextureBrowser_verticalScroll), &g_TextureBrowser);

    widget_set_visible(g_TextureBrowser.m_texture_scroll, g_TextureBrowser.m_showTextureScrollbar);
  }
  {
	  GtkWidget* texbox = gtk_vbox_new (FALSE, 0);
	  gtk_widget_show(texbox);
	  gtk_box_pack_start(GTK_BOX(hbox), texbox, TRUE, TRUE, 0);
    
    // Load the texture toolbar from the registry
    IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
    GtkToolbar* textureToolbar = tbCreator.getToolbar("texture");
    if (textureToolbar != NULL) {
		gtk_widget_show(GTK_WIDGET(textureToolbar));
		gtk_box_pack_start(GTK_BOX(texbox), GTK_WIDGET(textureToolbar), FALSE, FALSE, 0);
		
		// Button for toggling the resizing of textures
		
		GtkToolItem* sizeToggle = gtk_toggle_tool_button_new();
		
    	GdkPixbuf* pixBuf = gtkutil::getLocalPixbuf("texwindow_uniformsize.png");    	
    	GtkWidget* toggle_image = GTK_WIDGET(gtk_image_new_from_pixbuf(pixBuf));
    	
    	GtkTooltips* barTips = gtk_tooltips_new();
    	gtk_tool_item_set_tooltip(sizeToggle, barTips, "Clamp texture thumbnails to constant size", "");
    
    	gtk_tool_button_set_label(GTK_TOOL_BUTTON(sizeToggle), "Constant size");
    	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(sizeToggle), toggle_image);
    	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(sizeToggle), TRUE);
    	
    	// Insert button and connect callback
    	gtk_toolbar_insert(GTK_TOOLBAR(textureToolbar), sizeToggle, 0);
    	g_signal_connect(G_OBJECT(sizeToggle), 
    						 "toggled",
    						 G_CALLBACK(TextureBrowser_toggleResizeTextures), 
    				 		&g_TextureBrowser);
    
    	gdk_pixbuf_unref(pixBuf);
    	
    	gtk_widget_show_all(GTK_WIDGET(textureToolbar));
    }

        	
	  {
		  GtkEntry* entry = GTK_ENTRY(gtk_entry_new());
		  gtk_box_pack_start(GTK_BOX(texbox), GTK_WIDGET(entry), FALSE, FALSE, 0);

		  g_TextureBrowser.m_filter = entry;
      if(g_TextureBrowser.m_showTextureFilter)
      {
        gtk_widget_show(GTK_WIDGET(g_TextureBrowser.m_filter));
      }

      g_TextureBrowser.m_filterEntry.connect(entry);
	  }

	  {
      g_TextureBrowser.m_gl_widget = glwidget_new(FALSE);
      gtk_widget_ref(g_TextureBrowser.m_gl_widget);

      gtk_widget_set_events(g_TextureBrowser.m_gl_widget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
      GTK_WIDGET_SET_FLAGS(g_TextureBrowser.m_gl_widget, GTK_CAN_FOCUS);

		  gtk_box_pack_start(GTK_BOX(texbox), g_TextureBrowser.m_gl_widget, TRUE, TRUE, 0);
		  gtk_widget_show(g_TextureBrowser.m_gl_widget);

      g_TextureBrowser.m_sizeHandler = g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "size_allocate", G_CALLBACK(TextureBrowser_size_allocate), &g_TextureBrowser);
      g_TextureBrowser.m_exposeHandler = g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "expose_event", G_CALLBACK(TextureBrowser_expose), &g_TextureBrowser);

      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "button_press_event", G_CALLBACK(TextureBrowser_button_press), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "button_release_event", G_CALLBACK(TextureBrowser_button_release), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "motion_notify_event", G_CALLBACK(TextureBrowser_motion), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "scroll_event", G_CALLBACK(TextureBrowser_scroll), &g_TextureBrowser);
	  }
	}
  TextureBrowser_updateScroll(g_TextureBrowser);

  gtk_container_set_focus_chain(GTK_CONTAINER(hbox), NULL);

  return hbox;
}

void TextureBrowser_destroyWindow()
{
	g_TextureBrowser.destroy();
  GlobalShaderSystem().setActiveShadersChangedNotify(Callback());

  g_signal_handler_disconnect(G_OBJECT(g_TextureBrowser.m_gl_widget), g_TextureBrowser.m_sizeHandler);
  g_signal_handler_disconnect(G_OBJECT(g_TextureBrowser.m_gl_widget), g_TextureBrowser.m_exposeHandler);

  gtk_widget_unref(g_TextureBrowser.m_gl_widget);
}

void TextureBrowser_ToggleShowShaders() 
{
  g_TextureBrowser.m_showShaders ^= 1;
  g_TexturesMenu.m_showshaders_item.update();
  TextureBrowser_queueDraw(g_TextureBrowser);
}

void TextureBrowser_ToggleShowShaderListOnly() 
{
  g_TexturesMenu_shaderlistOnly ^= 1;
  g_TexturesMenu.m_showshaderlistonly_item.update();
}

void TextureBrowser_showAll()
{
  g_TextureBrowser_currentDirectory = "";
  g_TextureBrowser.heightChanged();
}

void TextureBrowser_registerPreferencesPage() {
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Texture Browser");
	
	// Create the string list containing the texture scalings
	std::list<std::string> textureScaleList;
	
	textureScaleList.push_back("10%");
	textureScaleList.push_back("25%");
	textureScaleList.push_back("50%");
	textureScaleList.push_back("100%");
	textureScaleList.push_back("200%");
	
	page->appendCombo("Texture Thumbnail Scale", RKEY_TEXTURE_SCALE, textureScaleList);
	
	page->appendEntry("Uniform texture thumbnail size (pixels)", RKEY_TEXTURE_UNIFORM_SIZE);
	page->appendCheckBox("", "Texture scrollbar", RKEY_TEXTURE_SHOW_SCROLLBAR);
	page->appendEntry("Mousewheel Increment", RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	
	page->appendCheckBox("", "Show Texture Filter", RKEY_TEXTURE_SHOW_FILTER);
}


#include "preferencesystem.h"
#include "stringio.h"

void TextureBrowser_Construct() {
	GlobalEventManager().addRegistryToggle("ShowInUse", RKEY_TEXTURES_HIDE_UNUSED);
	GlobalEventManager().addCommand("ShowAllTextures", FreeCaller<TextureBrowser_showAll>());
	GlobalEventManager().addCommand("ViewTextures", FreeCaller<TextureBrowser_toggleShown>());

	g_TextureBrowser.shader = texdef_name_default().c_str();

	TextureBrowser_registerPreferencesPage();

	GlobalShaderSystem().attach(g_ShadersObserver);
}

void TextureBrowser_Destroy() {
	GlobalShaderSystem().detach(g_ShadersObserver);
}
