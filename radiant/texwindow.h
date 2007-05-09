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

#if !defined(INCLUDED_TEXWINDOW_H)
#define INCLUDED_TEXWINDOW_H

#include "iregistry.h"
#include "generic/callbackfwd.h"
#include "signal/signalfwd.h"
#include "gtkutil/nonmodal.h"
#include "gtkutil/cursor.h"
#include "texturelib.h"
#include <string>

// texture browser

class TextureBrowser;
TextureBrowser& GlobalTextureBrowser();

typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;
GtkWidget* TextureBrowser_constructWindow(GtkWindow* toplevel);
void TextureBrowser_destroyWindow();


void TextureBrowser_ShowStartupShaders(TextureBrowser& textureBrowser);

void TextureBrowser_Construct();
void TextureBrowser_Destroy();

void TextureBrowser_addShadersRealiseCallback(const SignalHandler& handler);

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkAdjustment GtkAdjustment;

enum StartupShaders {
	STARTUPSHADERS_NONE = 0,
	STARTUPSHADERS_COMMON,
	STARTUPSHADERS_ALL,
};

class DeferredAdjustment
{
	gdouble m_value;
	guint m_handler;
	typedef void (*ValueChangedFunction)(void* data, gdouble value);
	ValueChangedFunction m_function;
	void* m_data;

public:
	DeferredAdjustment(ValueChangedFunction function, void* data);
	
	void flush();
	void value_changed(gdouble value);

	static void adjustment_value_changed(GtkAdjustment *adjustment, DeferredAdjustment* self);

private:
	static gboolean deferred_value_changed(gpointer data);
};

class TextureLayout {
public:
	TextureLayout() :
		current_x(8),
		current_y(-8),
		current_row(0)
	{}
		
	int current_x;
	int current_y;
	int current_row;
};

class TextureBrowser :
	public RegistryKeyObserver
{
public:
	int width, height;
	int originy;
	int m_nTotalHeight;

	std::string shader;

  GtkEntry* m_filter;
  NonModalEntry m_filterEntry;

  GtkWindow* m_parent;
  GtkWidget* m_gl_widget;

  guint m_sizeHandler;
  guint m_exposeHandler;

  GtkWidget* m_texture_scroll;

  bool m_heightChanged;
  bool m_originInvalid;

  DeferredAdjustment m_scrollAdjustment;
  FreezePointer m_freezePointer;

  // the increment step we use against the wheel mouse
  std::size_t m_mouseWheelScrollIncrement;
  std::size_t m_textureScale;
  bool m_showTextureFilter;
  // make the texture increments match the grid changes
  bool m_showTextureScrollbar;
  StartupShaders m_startupShaders;
  // if true, the texture window will only display in-use shaders
  // if false, all the shaders in memory are displayed
  bool m_hideUnused;
  
  // If true, textures are resized to a uniform size when displayed in the texture browser.
  // If false, textures are displayed in proportion to their pixel size.
  bool m_resizeTextures;
  // The uniform size (in pixels) that textures are resized to when m_resizeTextures is true.
  int m_uniformTextureSize;
  
	void clearFilter();
	typedef MemberCaller<TextureBrowser, &TextureBrowser::clearFilter> ClearFilterCaller;

	// RegistryKeyObserver implementation
	void keyChanged();
  
	// Return the display width of a texture in the texture browser
	int getTextureWidth(TexturePtr tex);
	// Return the display height of a texture in the texture browser
	int getTextureHeight(TexturePtr tex);

	// Constructor
	TextureBrowser();
  
	void construct();
	void destroy();
  
	void queueDraw();
	typedef MemberCaller<TextureBrowser, &TextureBrowser::queueDraw> TextureBrowserQueueDrawCaller;
  
	// greebo: This gets called as soon as the texture mode gets changed
	void textureModeChanged();
	
	// Legacy function needed for DeferredAdjustment (TODO!) 
	static void scrollChanged(void* data, gdouble value);

private:
	void setScaleFromRegistry();

public:
	void heightChanged();
	
	void updateScroll();
	
	// Returns the font height of the text in the opengl rendered window
	int getFontHeight();
	
	/** greebo: Returns the currently selected shader
	 */
	std::string getSelectedShader();
	
	/** greebo: Sets the currently selected shader to <newShader> and
	 * 			refocuses the texturebrowser to that shader.
	 */
	void setSelectedShader(const std::string& newShader);
	
	/** greebo: Returns the currently active filter string or "" if 
	 * 			the filter is not active.
	 */
	std::string getFilter();
	
	/** greebo: Sets the focus of the texture browser to the shader 
	 * 			with the given name.
	 */
	void focus(const std::string& name);
	
	void evaluateHeight();
	
	/** greebo: Returns the total height of the GL content
	 */
	int getTotalHeight();
	
	void clampOriginY();
	
	int getOriginY();
	void setOriginY(int newOriginY);
};

#endif
