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
#include "gtkutil/GLWidgetSentry.h"

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

namespace {
	const std::string RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
	const std::string RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
	const std::string RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
	const std::string RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
	const std::string RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
	const std::string RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
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
	queueDraw();
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

int TextureBrowser::getFontHeight() {
	return GlobalOpenGL().m_fontHeight;
}

std::string TextureBrowser::getSelectedShader() {
	return shader;
}

std::string TextureBrowser::getFilter() {
	if (m_showTextureFilter) {
		return gtk_entry_get_text(m_filter);
	}
	return "";
}

void TextureBrowser::setSelectedShader(const std::string& newShader) {
	shader = newShader;
	focus(shader);
}

void Texture_NextPos(TextureBrowser& textureBrowser, TextureLayout& layout, TexturePtr current_texture, int *x, int *y)
{
  TexturePtr q = current_texture;

  int nWidth = textureBrowser.getTextureWidth(q);
  int nHeight = textureBrowser.getTextureHeight(q);
  if (layout.current_x + nWidth > textureBrowser.width-8 && layout.current_row)
  { // go to the next row unless the texture is the first on the row
    layout.current_x = 8;
    layout.current_y -= layout.current_row + textureBrowser.getFontHeight() + 4;
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
bool Texture_IsShown(IShaderPtr shader, bool hideUnused, const std::string& filter)
{
  if(!shader_equal_prefix(shader->getName(), "textures/"))
    return false;

  if(hideUnused && !shader->IsInUse())
    return false;

  	if (!filter.empty()) {
		// some basic filtering
		if (strstr( shader_get_textureName(shader->getName()), filter.c_str() ) == 0)
			return false;
	}

  return true;
}

void TextureBrowser::heightChanged() {
	m_heightChanged = true;

	updateScroll();
	queueDraw();
}

void TextureBrowser::evaluateHeight() {
	if (m_heightChanged) {
		m_heightChanged = false;

		m_nTotalHeight = 0;

		TextureLayout layout;
		
	    for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
	    {
	      IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();
	
	      if(!Texture_IsShown(shader, m_hideUnused, getFilter()))
	        continue;
	
	      int   x, y;
	      Texture_NextPos(*this, layout, shader->getTexture(), &x, &y);
	      m_nTotalHeight = std::max(m_nTotalHeight, abs(layout.current_y) + getFontHeight() + getTextureHeight(shader->getTexture()) + 4);
	    }
	}
}

int TextureBrowser::getTotalHeight() {
	evaluateHeight();
	return m_nTotalHeight;
}

void TextureBrowser::clampOriginY() {
	if (originy > 0) {
		originy = 0;
	}
	
	int lower = std::min(height - getTotalHeight(), 0);
	
	if (originy < lower) {
		originy = lower;
	}
}

int TextureBrowser::getOriginY() {
	if (m_originInvalid) {
		m_originInvalid = false;
		clampOriginY();
		updateScroll();
	}
	return originy;
}

void TextureBrowser::setOriginY(int newOriginY) {
	originy = newOriginY;
	clampOriginY();
	updateScroll();
	queueDraw();
}

void TextureBrowser::activeShadersChanged() {
	heightChanged();
	m_originInvalid = true;
}

// Static command target
void TextureBrowser::toggle() {
	ui::GroupDialog::Instance().setPage("textures");
}

//++timo NOTE: this is a mix of Shader module stuff and texture explorer
// it might need to be split in parts or moved out .. dunno
// scroll origin so the specified texture is completely on screen
// if current texture is not displayed, nothing is changed
void TextureBrowser::focus(const std::string& name) {
  // scroll origin so the texture is completely on screen
  TextureLayout layout;
  
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if(!Texture_IsShown(shader, m_hideUnused, getFilter()))
      continue;

    int x, y;
    Texture_NextPos(*this, layout, shader->getTexture(), &x, &y);
    TexturePtr q = shader->getTexture();
    if (!q)
      break;

    // we have found when texdef->name and the shader name match
    // NOTE: as everywhere else for our comparisons, we are not case sensitive
    if (shader_equal(name, shader->getName()))
    {
      int textureHeight = getTextureHeight(q) + 2 * getFontHeight();

      int originy = getOriginY();
      if (y > originy)
      {
        originy = y;
      }

      if (y - textureHeight < originy - height)
      {
        originy = (y - textureHeight) + height;
      }

      setOriginY(originy);
      return;
    }
  }
}

IShaderPtr TextureBrowser::getShaderAtCoords(int mx, int my) {
	my += getOriginY() - height;

	TextureLayout layout;
	for(QERApp_ActiveShaders_IteratorBegin(); 
		!QERApp_ActiveShaders_IteratorAtEnd(); 
		QERApp_ActiveShaders_IteratorIncrement())
	{
		IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

		if(!Texture_IsShown(shader, m_hideUnused, getFilter()))
			continue;

		int x, y;
		Texture_NextPos(*this, layout, shader->getTexture(), &x, &y);
		TexturePtr q = shader->getTexture();
		if (!q)
			break;

		int nWidth = getTextureWidth(q);
		int nHeight = getTextureHeight(q);
		if (mx > x && mx - x < nWidth && my < y && y - my < nHeight + getFontHeight()) {
			return shader;
		}
	}

	return IShaderPtr();
}

void TextureBrowser::selectTextureAt(int mx, int my) {
	IShaderPtr shader = getShaderAtCoords(mx, my);
	
	if (shader != NULL) {
  		setSelectedShader(shader->getName());
  		
		// Apply the shader to the current selection
		selection::algorithm::applyShaderToSelection(shader->getName());
	}
}

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/
void TextureBrowser::trackingDelta(int x, int y, unsigned int state, void* data) {
	TextureBrowser& self = *reinterpret_cast<TextureBrowser*>(data);
	
	if (y != 0) {
		int scale = 1;

		if (state & GDK_SHIFT_MASK)
			scale = 4;

		int originy = self.getOriginY();
		originy += y * scale;
		self.setOriginY(originy);
	}
}

/*
============
Texture_Draw
TTimo: relying on the shaders list to display the textures
we must query all Texture* to manage and display through the IShaders interface
this allows a plugin to completely override the texture system
============
*/
void TextureBrowser::draw() {
  int originy = getOriginY();

  Vector3 colorBackground = ColourSchemes().getColourVector3("texture_background");
  glClearColor(colorBackground[0], colorBackground[1], colorBackground[2], 0);
  glViewport(0, 0, width, height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable (GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glOrtho (0, width, originy-height, originy, -100, 100);
  glEnable (GL_TEXTURE_2D);

  glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

  int last_y = 0, last_height = 0;

  TextureLayout layout;
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    IShaderPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if(!Texture_IsShown(shader, m_hideUnused, getFilter()))
      continue;

    int x, y;
    Texture_NextPos(*this, layout, shader->getTexture(), &x, &y);
    TexturePtr q = shader->getTexture();
    if (!q)
      break;

    int nWidth = getTextureWidth(q);
    int nHeight = getTextureHeight(q);

    if (y != last_y)
    {
      last_y = y;
      last_height = 0;
    }
    last_height = std::max (nHeight, last_height);

    // Is this texture visible?
    if ((y - nHeight - getFontHeight() < originy)
        && (y > originy - height))
    {
      // borders rules:
      // if it's the current texture, draw a thick red line, else:
      // shaders have a white border, simple textures don't
      // if !texture_showinuse: (some textures displayed may not be in use)
      // draw an additional square around with 0.5 1 0.5 color
      if (shader_equal(getSelectedShader(), shader->getName()))
      {
	      glLineWidth (3);
	      glColor3f (1,0,0);
	      glDisable (GL_TEXTURE_2D);

	      glBegin (GL_LINE_LOOP);
	      glVertex2i (x-4,y-getFontHeight()+4);
	      glVertex2i (x-4,y-getFontHeight()-nHeight-4);
	      glVertex2i (x+4+nWidth,y-getFontHeight()-nHeight-4);
	      glVertex2i (x+4+nWidth,y-getFontHeight()+4);
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
	        glVertex2i (x-1,y+1-getFontHeight());
	        glVertex2i (x-1,y-nHeight-1-getFontHeight());
	        glVertex2i (x+1+nWidth,y-nHeight-1-getFontHeight());
	        glVertex2i (x+1+nWidth,y+1-getFontHeight());
	        glEnd();
	        glEnable (GL_TEXTURE_2D);
	      }

	      // highlight in-use textures
	      if (!m_hideUnused && shader->IsInUse())
	      {
	        glColor3f (0.5,1,0.5);
	        glDisable (GL_TEXTURE_2D);
	        glBegin (GL_LINE_LOOP);
	        glVertex2i (x-3,y+3-getFontHeight());
	        glVertex2i (x-3,y-nHeight-3-getFontHeight());
	        glVertex2i (x+3+nWidth,y-nHeight-3-getFontHeight());
	        glVertex2i (x+3+nWidth,y+3-getFontHeight());
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
      glVertex2i (x,y-getFontHeight());
      glTexCoord2i (1,0);
      glVertex2i (x+nWidth,y-getFontHeight());
      glTexCoord2i (1,1);
      glVertex2i (x+nWidth,y-getFontHeight()-nHeight);
      glTexCoord2i (0,1);
      glVertex2i (x,y-getFontHeight()-nHeight);
      glEnd();

      // draw the texture name
      glDisable (GL_TEXTURE_2D);
      glColor3f (1,1,1);

      glRasterPos2i (x, y-getFontHeight()+5);

      // don't draw the directory name
      const char* name = shader->getName();
      name += strlen(name);
      while(name != shader->getName() && *(name-1) != '/' && *(name-1) != '\\')
        name--;

      GlobalOpenGL().drawString(name);
      glEnable (GL_TEXTURE_2D);
    }

    //int totalHeight = abs(y) + last_height + textureBrowser.getFontHeight() + 4;
  }

  // reset the current texture
  glBindTexture(GL_TEXTURE_2D, 0);
  //qglFinish();
}

void TextureBrowser::doMouseWheel(bool wheelUp) {
	int originy = getOriginY();

	if (wheelUp) {
		originy += int(m_mouseWheelScrollIncrement);
	}
	else {
		originy -= int(m_mouseWheelScrollIncrement);
	}

	setOriginY(originy);
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


gboolean TextureBrowser_button_press(GtkWidget* widget, GdkEventButton* event, TextureBrowser* textureBrowser) {
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 3) {
			textureBrowser->m_freezePointer.freeze_pointer(textureBrowser->m_parent, TextureBrowser::trackingDelta, textureBrowser);
		}
		else if(event->button == 1) {
			textureBrowser->selectTextureAt(
    			static_cast<int>(event->x), 
    			textureBrowser->height - 1 - static_cast<int>(event->y)
    		);
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
    	textureBrowser->m_freezePointer.unfreeze_pointer(textureBrowser->m_parent);
    }
  }
  return FALSE;
}

gboolean TextureBrowser_motion(GtkWidget *widget, GdkEventMotion *event, TextureBrowser* textureBrowser)
{
  return FALSE;
}

gboolean TextureBrowser_scroll(GtkWidget* widget, GdkEventScroll* event, TextureBrowser* textureBrowser) {
	if (event->direction == GDK_SCROLL_UP) {
		textureBrowser->doMouseWheel(true);
	}
	else if(event->direction == GDK_SCROLL_DOWN) {
		textureBrowser->doMouseWheel(false);
	}
	return FALSE;
}

void TextureBrowser::scrollChanged(void* data, gdouble value) {
	//globalOutputStream() << "vertical scroll\n";
	reinterpret_cast<TextureBrowser*>(data)->setOriginY(-(int)value);
}

static void TextureBrowser_verticalScroll(GtkAdjustment *adjustment, TextureBrowser* textureBrowser)
{
  textureBrowser->m_scrollAdjustment.value_changed(adjustment->value);
}

void TextureBrowser::updateScroll() {
  if(m_showTextureScrollbar)
  {
    int totalHeight = getTotalHeight();

    totalHeight = std::max(totalHeight, height);

    GtkAdjustment *vadjustment = gtk_range_get_adjustment(GTK_RANGE(m_texture_scroll));

    vadjustment->value = -getOriginY();
    vadjustment->page_size = height;
    vadjustment->page_increment = height/2;
    vadjustment->step_increment = 20;
    vadjustment->lower = 0;
    vadjustment->upper = totalHeight;

    g_signal_emit_by_name(G_OBJECT(vadjustment), "changed");
  }
}

gboolean TextureBrowser_size_allocate(GtkWidget* widget, GtkAllocation* allocation, TextureBrowser* textureBrowser)
{
  textureBrowser->width = allocation->width;
  textureBrowser->height = allocation->height;
  textureBrowser->heightChanged();
  textureBrowser->m_originInvalid = true;
  textureBrowser->queueDraw();
  return FALSE;
}

gboolean TextureBrowser::onExpose(GtkWidget* widget, GdkEventExpose* event, TextureBrowser* self) {
	// This calls glwidget_make_current() for us and swap_buffers at the end of scope
	gtkutil::GLWidgetSentry sentry(self->m_gl_widget);
	
    GlobalOpenGL_debugAssertNoErrors();
    self->evaluateHeight();
    self->draw();
    GlobalOpenGL_debugAssertNoErrors();

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
	
  GlobalShaderSystem().setActiveShadersChangedNotify(MemberCaller<TextureBrowser, &TextureBrowser::activeShadersChanged>(g_TextureBrowser));

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
      g_TextureBrowser.m_exposeHandler = g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "expose_event", G_CALLBACK(TextureBrowser::onExpose), &g_TextureBrowser);

      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "button_press_event", G_CALLBACK(TextureBrowser_button_press), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "button_release_event", G_CALLBACK(TextureBrowser_button_release), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "motion_notify_event", G_CALLBACK(TextureBrowser_motion), &g_TextureBrowser);
      g_signal_connect(G_OBJECT(g_TextureBrowser.m_gl_widget), "scroll_event", G_CALLBACK(TextureBrowser_scroll), &g_TextureBrowser);
	  }
	}
  g_TextureBrowser.updateScroll();

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

void TextureBrowser_showAll()
{
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

class ShadersObserver : 
	public ModuleObserver
{
	Signal0 m_realiseCallbacks;
public:
	void realise() {
		m_realiseCallbacks();
	}
	
	void unrealise() {
	}

	void insert(const SignalHandler& handler) {
		m_realiseCallbacks.connectLast(handler);
	}
};

namespace {
	ShadersObserver g_ShadersObserver;
}

#include "preferencesystem.h"
#include "stringio.h"

void TextureBrowser_Construct() {
	GlobalEventManager().addRegistryToggle("ShowInUse", RKEY_TEXTURES_HIDE_UNUSED);
	GlobalEventManager().addCommand("ShowAllTextures", FreeCaller<TextureBrowser_showAll>());
	GlobalEventManager().addCommand("ViewTextures", FreeCaller<TextureBrowser::toggle>());

	g_TextureBrowser.shader = texdef_name_default().c_str();

	TextureBrowser_registerPreferencesPage();

	GlobalShaderSystem().attach(g_ShadersObserver);
}

void TextureBrowser_Destroy() {
	GlobalShaderSystem().detach(g_ShadersObserver);
}
