#include "TextureBrowser.h"

#include "ieventmanager.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "iradiant.h"
#include "ipreferencesystem.h"

#include <gtk/gtk.h>
#include "gtkutil/widget.h"
#include "gtkutil/GLWidgetSentry.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/LeftAlignedLabel.h"

#include "shaderlib.h"
#include "selection/algorithm/Shader.h"
#include "ui/mediabrowser/MediaBrowser.h"

#include <boost/algorithm/string/predicate.hpp>

namespace ui {

namespace {
	const std::string RKEY_TEXTURES_HIDE_UNUSED = "user/ui/textures/browser/hideUnused";
	const std::string RKEY_TEXTURE_SCALE = "user/ui/textures/browser/textureScale";
	const std::string RKEY_TEXTURE_UNIFORM_SIZE = "user/ui/textures/browser/uniformSize";
	const std::string RKEY_TEXTURE_SHOW_SCROLLBAR = "user/ui/textures/browser/showScrollBar";
	const std::string RKEY_TEXTURE_MOUSE_WHEEL_INCR = "user/ui/textures/browser/mouseWheelIncrement";
	const std::string RKEY_TEXTURE_SHOW_FILTER = "user/ui/textures/browser/showFilter";
	const std::string RKEY_TEXTURE_CONTEXTMENU_EPSILON = "user/ui/textures/browser/contextMenuMouseEpsilon";
	const std::string RKEY_TEXTURE_MAX_NAME_LENGTH = "user/ui/textures/browser/maxShadernameLength";
	
	const std::string SEEK_IN_MEDIA_BROWSER_TEXT = "Seek in Media Browser";
	const char* TEXTURE_ICON = "icon_texture.png";
}

TextureBrowser::TextureBrowser() :
	_popupX(-1),
	_popupY(-1),
	_startOrigin(-1),
	_epsilon(GlobalRegistry().getFloat(RKEY_TEXTURE_CONTEXTMENU_EPSILON)),
	_popupMenu(gtk_menu_new()),
	m_filter(0),
	m_filterEntry(TextureBrowserQueueDrawCaller(*this), ClearFilterCaller(*this)),
	m_texture_scroll(0),
	m_heightChanged(true),
	m_originInvalid(true),
	m_scrollAdjustment(scrollChanged, this),
	m_mouseWheelScrollIncrement(GlobalRegistry().getInt(RKEY_TEXTURE_MOUSE_WHEEL_INCR)),
	m_textureScale(50),
	m_showTextureFilter(GlobalRegistry().get(RKEY_TEXTURE_SHOW_FILTER) == "1"),
	m_showTextureScrollbar(GlobalRegistry().get(RKEY_TEXTURE_SHOW_SCROLLBAR) == "1"),
	m_hideUnused(GlobalRegistry().get(RKEY_TEXTURES_HIDE_UNUSED) == "1"),
	m_resizeTextures(true),
	m_uniformTextureSize(GlobalRegistry().getInt(RKEY_TEXTURE_UNIFORM_SIZE))
{
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURES_HIDE_UNUSED);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SCALE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_UNIFORM_SIZE);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_SCROLLBAR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_MOUSE_WHEEL_INCR);
	GlobalRegistry().addKeyObserver(this, RKEY_TEXTURE_SHOW_FILTER);
			
	shader = texdef_name_default();
	
	setScaleFromRegistry();
	
	// Construct the popup context menu
	_seekInMediaBrowser = gtkutil::IconTextMenuItem(
		GlobalUIManager().getLocalPixbuf(TEXTURE_ICON), 
		SEEK_IN_MEDIA_BROWSER_TEXT
	);
	g_signal_connect(G_OBJECT(_seekInMediaBrowser), "activate", G_CALLBACK(onSeekInMediaBrowser), this);
	
	_shaderLabel = gtk_menu_item_new_with_label("No shader");
	gtk_widget_set_sensitive(_shaderLabel, FALSE);
	
	gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), _shaderLabel);
	gtk_menu_shell_append(GTK_MENU_SHELL(_popupMenu), _seekInMediaBrowser);
	gtk_widget_show_all(_popupMenu);
}

void TextureBrowser::queueDraw() {
	if (_glWidget != NULL) {
		// Cast to GtkWidget* and issue a queue draw
		GtkWidget* glWidget = *_glWidget;
		gtk_widget_queue_draw(glWidget);
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

void TextureBrowser::keyChanged(const std::string& key, const std::string& val) 
{
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
		width = (int)(tex->getWidth() * ((float)m_textureScale / 100));
	}
	else if (tex->getWidth() >= tex->getHeight()) {
		// Texture is square, or wider than it is tall
		width = m_uniformTextureSize;
	}
	else {
		// Otherwise, preserve the texture's aspect ratio
		width = (int)(m_uniformTextureSize * ((float)tex->getWidth() / tex->getHeight()));
	}
    
    return width;
}

int TextureBrowser::getTextureHeight(TexturePtr tex) {
	int height;
	if (!m_resizeTextures) {
		// Don't use uniform size
		height = (int)(tex->getHeight() * ((float)m_textureScale / 100));
	}
    else if (tex->getHeight() >= tex->getWidth()) {
		// Texture is square, or taller than it is wide
		height = m_uniformTextureSize;
	}
	else {
		// Otherwise, preserve the texture's aspect ratio
		height = (int)(m_uniformTextureSize * ((float)tex->getHeight() / tex->getWidth()));
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

void TextureBrowser::nextTexturePos(TextureLayout& layout, TexturePtr tex, int *x, int *y) {
	int nWidth = getTextureWidth(tex);
	int nHeight = getTextureHeight(tex);
  
	// go to the next row unless the texture is the first on the row
	if (layout.current_x + nWidth > width-8 && layout.current_row) { 
		layout.current_x = 8;
		layout.current_y -= layout.current_row + getFontHeight() + 4;
		layout.current_row = 0;
	}

	// Emit the new coordinates
	*x = layout.current_x;
	*y = layout.current_y;

	// Is our texture larger than the row? If so, grow the
	// row height to match it

	if (layout.current_row < nHeight) {
		layout.current_row = nHeight;
	}

	// never go less than 96, or the names get all crunched up, plus spacing
	layout.current_x += std::max(96, nWidth) + 8;
}

// if texture_showinuse jump over non in-use textures
bool TextureBrowser::shaderIsVisible(MaterialPtr shader) 
{
	if (shader == NULL) {
		return false;
	}
	
	if (!boost::algorithm::istarts_with(shader->getName(), "textures/")) 
    {
		return false;
	}

	if (m_hideUnused && !shader->IsInUse()) {
		return false;
	}

  	if (!getFilter().empty()) 
    {
		// some basic filtering
		if (strstr( shader_get_textureName(shader->getName().c_str()), getFilter().c_str() ) == 0)
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
	// greebo: Let the texture browser re-evaluate the scrollbar each frame
	//if (m_heightChanged) {
		m_heightChanged = false;

		m_nTotalHeight = 0;

		TextureLayout layout;
		
	    for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
	    {
	      MaterialPtr shader = QERApp_ActiveShaders_IteratorCurrent();
	
	      if (!shaderIsVisible(shader))
	        continue;
	
	      int   x, y;
	      nextTexturePos(layout, shader->getEditorImage(), &x, &y);
	      m_nTotalHeight = std::max(m_nTotalHeight, abs(layout.current_y) + getFontHeight() + getTextureHeight(shader->getEditorImage()) + 4);
	    }
	//}
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
void TextureBrowser::toggle(const cmd::ArgumentList& args) {
	GlobalGroupDialog().togglePage("textures");
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
    MaterialPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if (!shaderIsVisible(shader))
      continue;

    int x, y;
    nextTexturePos(layout, shader->getEditorImage(), &x, &y);
    TexturePtr q = shader->getEditorImage();
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

MaterialPtr TextureBrowser::getShaderAtCoords(int mx, int my) {
	my += getOriginY() - height;

	TextureLayout layout;
	for(QERApp_ActiveShaders_IteratorBegin(); 
		!QERApp_ActiveShaders_IteratorAtEnd(); 
		QERApp_ActiveShaders_IteratorIncrement())
	{
		MaterialPtr shader = QERApp_ActiveShaders_IteratorCurrent();

		if (!shaderIsVisible(shader))
			continue;

		int x, y;
		nextTexturePos(layout, shader->getEditorImage(), &x, &y);
		
		TexturePtr tex = shader->getEditorImage();
		if (tex == NULL) {
			break;
		}

		int nWidth = getTextureWidth(tex);
		int nHeight = getTextureHeight(tex);
		if (mx > x && mx - x < nWidth && my < y && y - my < nHeight + getFontHeight()) {
			return shader;
		}
	}

	return MaterialPtr();
}

void TextureBrowser::selectTextureAt(int mx, int my) {
	MaterialPtr shader = getShaderAtCoords(mx, my);
	
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
we must query all Texture* to manage and display through the Materials interface
this allows a plugin to completely override the texture system
============
*/
void TextureBrowser::draw() {
  int originy = getOriginY();

  Vector3 colorBackground = ColourSchemes().getColour("texture_background");
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

	unsigned int maxNameLength = GlobalRegistry().getInt(RKEY_TEXTURE_MAX_NAME_LENGTH);

  TextureLayout layout;
  for(QERApp_ActiveShaders_IteratorBegin(); !QERApp_ActiveShaders_IteratorAtEnd(); QERApp_ActiveShaders_IteratorIncrement())
  {
    MaterialPtr shader = QERApp_ActiveShaders_IteratorCurrent();

    if (!shaderIsVisible(shader))
      continue;

    int x, y;
    nextTexturePos(layout, shader->getEditorImage(), &x, &y);
    TexturePtr q = shader->getEditorImage();
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
      glBindTexture (GL_TEXTURE_2D, q->getGLTexNum());
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
      std::string name = shader->getName();
      name = name.substr(name.rfind("/") + 1);
      
      // Ellipsize the name if it's too long
      if (name.size() > maxNameLength) {
      	name = name.substr(0, maxNameLength/2) + 
      		   "..." + 
      		   name.substr(name.size() - maxNameLength/2);  
      }
      
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

void TextureBrowser::openContextMenu() {
	
	std::string shaderText = "No shader";
	
	if (_popupX > 0 && _popupY > 0) {
		MaterialPtr shader = getShaderAtCoords(_popupX, _popupY);
		
		if (shader != NULL) {
			shaderText = shader->getName();
			shaderText = shaderText.substr(shaderText.rfind("/")+1);
			gtk_widget_set_sensitive(_seekInMediaBrowser, TRUE);
		}
		else {
			gtk_widget_set_sensitive(_seekInMediaBrowser, FALSE);
		}
	}
	
	gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(_shaderLabel))), shaderText.c_str());
	
	gtk_menu_popup(GTK_MENU(_popupMenu), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

// Static
void TextureBrowser::onSeekInMediaBrowser(GtkMenuItem* item, TextureBrowser* self) {
	
	if (self->_popupX > 0 && self->_popupY > 0) {
		MaterialPtr shader = self->getShaderAtCoords(self->_popupX, self->_popupY);
	
		if (shader != NULL) {
			// Focus the MediaBrowser selection to the given shader
			GlobalGroupDialog().setPage("mediabrowser");
			MediaBrowser::getInstance().setSelection(shader->getName());
		}
	}
	
	self->_popupX = -1;
	self->_popupY = -1;
}

// GTK callback for toggling uniform texture sizing
void TextureBrowser::onResizeToggle(GtkWidget* button, TextureBrowser* self) {
	self->m_resizeTextures = 
		gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button)) ? true : false;

	// Update texture browser
	self->heightChanged();
}

gboolean TextureBrowser::onButtonPress(GtkWidget* widget, GdkEventButton* event, TextureBrowser* self) {
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 3) {
			self->m_freezePointer.freeze_pointer(self->m_parent, trackingDelta, self);
			
			// Store the coords of the mouse pointer for later reference
			self->_popupX = static_cast<int>(event->x);
			self->_popupY = self->height - 1 - static_cast<int>(event->y);
			self->_startOrigin = self->originy;
		}
		else if (event->button == 1) {
			self->selectTextureAt(
    			static_cast<int>(event->x), 
    			self->height - 1 - static_cast<int>(event->y)
    		);
    	}
	}

	return FALSE;
}

gboolean TextureBrowser::onButtonRelease(GtkWidget* widget, GdkEventButton* event, TextureBrowser* self) {
	if (event->type == GDK_BUTTON_RELEASE) {
		if (event->button == 3) {
			self->m_freezePointer.unfreeze_pointer(self->m_parent);

			// See how much we've been scrolling since mouseDown			
			int delta = abs(self->originy - self->_startOrigin);
			
			if (delta <= self->_epsilon) {
				 self->openContextMenu();
			}
			
			self->_startOrigin = -1;
		}
	}

	return FALSE;
}

gboolean TextureBrowser::onMouseMotion(GtkWidget *widget, GdkEventMotion *event, TextureBrowser* self) {
	return FALSE;
}

gboolean TextureBrowser::onMouseScroll(GtkWidget* widget, GdkEventScroll* event, TextureBrowser* self) {
	if (event->direction == GDK_SCROLL_UP) {
		self->doMouseWheel(true);
	}
	else if(event->direction == GDK_SCROLL_DOWN) {
		self->doMouseWheel(false);
	}
	return FALSE;
}

void TextureBrowser::scrollChanged(void* data, gdouble value) {
	reinterpret_cast<TextureBrowser*>(data)->setOriginY(-(int)value);
}

void TextureBrowser::onVerticalScroll(GtkAdjustment *adjustment, TextureBrowser* self) {
	self->m_scrollAdjustment.value_changed(adjustment->value);
}

void TextureBrowser::updateScroll() {
  if(m_showTextureScrollbar)
  {
    int totalHeight = getTotalHeight();

    totalHeight = std::max(totalHeight, height);

    GtkAdjustment *vadjustment = gtk_range_get_adjustment(GTK_RANGE(m_texture_scroll));

    if (vadjustment != NULL) {
	    vadjustment->value = -getOriginY();
	    vadjustment->page_size = height;
	    vadjustment->page_increment = height/2;
	    vadjustment->step_increment = 20;
	    vadjustment->lower = 0;
	    vadjustment->upper = totalHeight;
	
	    g_signal_emit_by_name(G_OBJECT(vadjustment), "changed");
    }
  }
}

gboolean TextureBrowser::onSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, TextureBrowser* self) {
	self->width = allocation->width;
	self->height = allocation->height;
	self->heightChanged();
	self->m_originInvalid = true;
	self->queueDraw();

	return FALSE;
}

gboolean TextureBrowser::onExpose(GtkWidget* widget, GdkEventExpose* event, TextureBrowser* self) {
	// No widget, no drawing
	if (self->_glWidget == NULL) return FALSE;

	// This calls glwidget_make_current() for us and swap_buffers at the end of scope
	gtkutil::GLWidgetSentry sentry(*self->_glWidget);
	
    GlobalOpenGL_debugAssertNoErrors();
    self->evaluateHeight();
	self->updateScroll();
    self->draw();
    GlobalOpenGL_debugAssertNoErrors();

	return FALSE;
}

GtkWidget* TextureBrowser::constructWindow(GtkWindow* parent) {
	
	m_parent = parent;

	// Instantiate a new GLwidget without z-buffering
	_glWidget = gtkutil::GLWidgetPtr(
        new gtkutil::GLWidget(false, "TextureBrowser")
    );
	
	GlobalMaterialManager().setActiveShadersChangedNotify(
		MemberCaller<TextureBrowser, &TextureBrowser::activeShadersChanged>(*this)
	);

	GtkWidget* hbox = gtk_hbox_new(FALSE, 0);

	{
		m_texture_scroll = gtk_vscrollbar_new(GTK_ADJUSTMENT(gtk_adjustment_new (0,0,0,1,1,1)));
		gtk_widget_show(m_texture_scroll);
		gtk_box_pack_end(GTK_BOX (hbox), m_texture_scroll, FALSE, TRUE, 0);
	  
		GtkAdjustment *vadjustment = gtk_range_get_adjustment(GTK_RANGE(m_texture_scroll));
		g_signal_connect(G_OBJECT(vadjustment), "value-changed", G_CALLBACK(onVerticalScroll), this);

		widget_set_visible(m_texture_scroll, m_showTextureScrollbar);
	}
	
	{
		GtkWidget* texbox = gtk_vbox_new(FALSE, 0);
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
			
	    	GdkPixbuf* pixBuf = GlobalUIManager().getLocalPixbuf("texwindow_uniformsize.png");    	
	    	GtkWidget* toggle_image = GTK_WIDGET(gtk_image_new_from_pixbuf(pixBuf));
	    	
	    	GtkTooltips* barTips = gtk_tooltips_new();
	    	gtk_tool_item_set_tooltip(sizeToggle, barTips, "Clamp texture thumbnails to constant size", "");
	    
	    	gtk_tool_button_set_label(GTK_TOOL_BUTTON(sizeToggle), "Constant size");
	    	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(sizeToggle), toggle_image);
	    	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(sizeToggle), TRUE);
	    	
	    	// Insert button and connect callback
	    	gtk_toolbar_insert(GTK_TOOLBAR(textureToolbar), sizeToggle, 0);
	    	g_signal_connect(G_OBJECT(sizeToggle), "toggled", G_CALLBACK(onResizeToggle), this);

	    	gtk_widget_show_all(GTK_WIDGET(textureToolbar));
		}

		{
			m_filter = GTK_ENTRY(gtk_entry_new());
			gtk_box_pack_start(GTK_BOX(texbox), GTK_WIDGET(m_filter), FALSE, FALSE, 0);

			if (m_showTextureFilter) {
				gtk_widget_show(GTK_WIDGET(m_filter));
			}

			m_filterEntry.connect(m_filter);
		}

		{
			// Cast gtkutil::GLWidget to GtkWidget*
			GtkWidget* glWidget = *_glWidget;

			gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
			GTK_WIDGET_SET_FLAGS(glWidget, GTK_CAN_FOCUS);

			gtk_box_pack_start(GTK_BOX(texbox), glWidget, TRUE, TRUE, 0);
			gtk_widget_show(glWidget);

			g_signal_connect(G_OBJECT(glWidget), "size-allocate", G_CALLBACK(onSizeAllocate), this);
			g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(onExpose), this);

			g_signal_connect(G_OBJECT(glWidget), "button-press-event", G_CALLBACK(onButtonPress), this);
			g_signal_connect(G_OBJECT(glWidget), "button-release-event", G_CALLBACK(onButtonRelease), this);
			g_signal_connect(G_OBJECT(glWidget), "motion-notify-event", G_CALLBACK(onMouseMotion), this);
			g_signal_connect(G_OBJECT(glWidget), "scroll-event", G_CALLBACK(onMouseScroll), this);
		}
	}
	
	updateScroll();
	gtk_container_set_focus_chain(GTK_CONTAINER(hbox), NULL);

	return hbox;
}

void TextureBrowser::destroyWindow() {
	GlobalMaterialManager().setActiveShadersChangedNotify(Callback());

	_glWidget = gtkutil::GLWidgetPtr();
}

void TextureBrowser::registerPreferencesPage() {
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
	page->appendSpinner("Max shadername length", RKEY_TEXTURE_MAX_NAME_LENGTH, 4, 100, 1);
	
	page->appendCheckBox("", "Show Texture Filter", RKEY_TEXTURE_SHOW_FILTER);
}

void TextureBrowser::construct() {
	GlobalEventManager().addRegistryToggle("ShowInUse", RKEY_TEXTURES_HIDE_UNUSED);
	GlobalCommandSystem().addCommand("ViewTextures", TextureBrowser::toggle);
	GlobalEventManager().addCommand("ViewTextures", "ViewTextures");

	TextureBrowser::registerPreferencesPage();
}

void TextureBrowser::update() {
	heightChanged();
}

} // namespace ui

/** greebo: The accessor method, use this to call non-static TextureBrowser methods
 */
ui::TextureBrowser& GlobalTextureBrowser() {
	static ui::TextureBrowser _instance;
	return _instance;
}
