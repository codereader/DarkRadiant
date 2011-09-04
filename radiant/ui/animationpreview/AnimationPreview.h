#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "imd5model.h"
#include "inode.h"
#include "ieclass.h"
#include "iscenegraph.h"
#include "irendersystemfactory.h"
#include "math/Matrix4.h"

#include "render/NopVolumeTest.h"
#include "render/ShaderStateRenderer.h"

#include <gtkmm/frame.h>
#include "gtkutil/GLWidget.h"
#include "gtkutil/Timer.h"

namespace Gtk
{
	class ToolButton;
	class ToggleToolButton;
}

namespace ui
{

class AnimationPreview :
	public Gtk::Frame
{
private:
	// GL widget
	gtkutil::GLWidget* _glWidget;

	Gtk::ToolButton* _startButton;
	Gtk::ToolButton* _pauseButton;
	Gtk::ToolButton* _stopButton;

	// The local scenegraph
	scene::GraphPtr _scene;

	// The backend rendersystem instance
	RenderSystemPtr _renderSystem;

	// The front-end renderer, collecting the OpenGLRenderables
	render::ShaderStateRenderer _renderer;
	render::NopVolumeTest _volumeTest;

	// Current MD5 model node to display
	scene::INodePtr _model;

	// Each model node needs a parent entity to be properly renderable
	scene::INodePtr _parentEntity;

	// The animation to play on this model
	md5::IMD5AnimPtr _anim;

	// Current distance between camera and preview
	GLfloat _camDist;

	// Current rotation matrix
	Matrix4 _rotation;

	// Mutex flag to avoid draw call bunching
	bool _renderingInProgress;

	gtkutil::Timer _timer;

	int _previewWidth;
	int _previewHeight;

private:
	// gtkmm callbacks
	bool callbackGLDraw(GdkEventExpose*);
	bool callbackGLMotion(GdkEventMotion*);
	bool callbackGLScroll(GdkEventScroll*);
	void callbackStart();
	void callbackPause();
	void callbackStop();
	void callbackStepForward();
	void callbackStepBack();
	void onSizeAllocate(Gtk::Allocation& allocation);

	static Matrix4 getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height);

public:

	/** Construct a AnimationPreview widget.
	 */
	AnimationPreview();

	~AnimationPreview();

	void setSize(int width, int height);

	/**
	 * Initialise the GL preview. This clears the window and sets up the
	 * initial matrices and lights.
	 */
	void initialisePreview();

	void setModelNode(const scene::INodePtr& model);
	void setAnim(const md5::IMD5AnimPtr& anim);

	Gtk::Widget* getWidget();

	const scene::INodePtr& getModelNode() const
	{
		return _model;
	}

	const md5::IMD5AnimPtr& getAnim() const
	{
		return _anim;
	}

private:
	// Called each frame by gtkutil::Timer
	static gboolean _onFrame(gpointer data);

	void drawAxes();
	void drawTime();
	void drawDebugInfo();

	void startPlayback();
	void stopPlayback();
};
typedef boost::shared_ptr<AnimationPreview> AnimationPreviewPtr;

}
