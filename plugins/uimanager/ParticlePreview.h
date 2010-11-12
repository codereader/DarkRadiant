#ifndef _PARTICLE_PREVIEW_H_
#define _PARTICLE_PREVIEW_H_

#include "iparticles.h"
#include "iparticlepreview.h"
#include "irendersystemfactory.h"

#include "math/matrix.h"

#include <string>
#include <map>
#include "gtkutil/GLWidget.h"
#include "gtkutil/Timer.h"
#include <gtkmm/frame.h>

#include "ParticleRenderer.h"

namespace Gtk
{
	class ToolButton;
	class ToggleToolButton;
}

namespace ui
{

class ParticlePreview :
	public IParticlePreview,
	public Gtk::Frame
{
private:
	// GL widget
	gtkutil::GLWidget* _glWidget;

	Gtk::ToolButton* _startButton;
	Gtk::ToolButton* _pauseButton;
	Gtk::ToolButton* _stopButton;
	Gtk::ToggleToolButton* _showAxesButton;
	Gtk::ToggleToolButton* _showWireFrameButton;

	// The backend rendersystem instance
	RenderSystemPtr _renderSystem;

	// The front-end renderer, collecting the OpenGLRenderables
	// from the particle system
	ParticleRenderer _renderer;
	ParticleVolumeTest _volumeTest;

	// Current particle to display
	particles::IRenderableParticlePtr _particle;

	// Current distance between camera and preview
	GLfloat _camDist;

	// Current rotation matrix
	Matrix4 _rotation;

	std::string _lastParticle;

	// Increasing preview time in msecs
	std::size_t _previewTimeMsec;

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
	void callbackToggleAxes();
	void onSizeAllocate(Gtk::Allocation& allocation);

	static Matrix4 getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height);

public:

	/** Construct a ParticlePreview widget.
	 */
	ParticlePreview();

	~ParticlePreview();

	/**
	 * Set the pixel size of the ParticlePreview widget. The widget is always
	 * square.
	 *
	 * @param size
	 * The pixel size of the square widget.
	 */
	void setSize(int size);

	/**
	 * Initialise the GL preview. This clears the window and sets up the
	 * initial matrices and lights.
	 */
	void initialisePreview();

	/**
	 * Set the widget to display the given particle. If the particle name is the
	 * empty string, the widget will release the currently displayed one.
	 *
	 * @param
	 * String name of the particle to display.
	 */
	void setParticle(const std::string& particle);

	Gtk::Widget* getWidget();

	/**
	 * Get the model from the widget, in order to display properties about it.
	 */
	particles::IParticleDefPtr getParticle()
	{
		return _particle ? _particle->getParticleDef() : particles::IParticleDefPtr();
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
typedef boost::shared_ptr<ParticlePreview> ParticlePreviewPtr;

} // namespace

#endif /* _PARTICLE_PREVIEW_H_ */
