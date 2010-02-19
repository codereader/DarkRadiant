#ifndef GuiRenderer_h__
#define GuiRenderer_h__

#include "math/Vector2.h"
#include "gtkutil/GLWidget.h"
#include <boost/noncopyable.hpp>

#include "Gui.h"
#include "irenderable.h"
#include <vector>

namespace gui
{

class GuiRenderer :
	public boost::noncopyable,
	public RenderableCollector
{
private:
	GuiPtr _gui;

	Vector2 _viewPortTopLeft;
	Vector2 _viewPortBottomRight;

	// Renderer State 
	typedef std::vector<const OpenGLRenderable*> RenderableList;
	typedef std::map<ShaderPtr, RenderableList> ShaderBuckets;
	ShaderBuckets _buckets;

	ShaderPtr _curState;

public:
	// Construct a new renderer
	GuiRenderer();

	void setGui(const GuiPtr& gui);

	// Starts rendering the attached GUI
	// The GL context must be set before calling this method
	void render();

	// RenderableCollector implementation
	void PushState();
	void PopState();
  	void SetState(const ShaderPtr& state, EStyle mode);
	
	void addRenderable(const OpenGLRenderable& renderable, const Matrix4& world);
  
	const EStyle getStyle() const;
	
	void Highlight(EHighlightMode mode, bool bEnable = true);

private:
	void render(const GuiWindowDefPtr& window);

	void flushRenderables();
};

}

#endif // GuiRenderer_h__
