#ifndef _IPARTICLE_PREVIEW_H_
#define _IPARTICLE_PREVIEW_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace Gtk { class Widget; }

namespace particles
{

class IParticleDef;
typedef boost::shared_ptr<IParticleDef> IParticleDefPtr;

} // namespace

namespace ui
{

/**
 * The model preview can be packed into a GTK parent container
 * and provides methods to get/set the displayed IParticle.
 */
class IParticlePreview
{
public:
    /**
	 * Destructor
	 */
    virtual ~IParticlePreview() {}

	/** 
	 * Set the pixel size of the IParticlePreview widget. The widget is always 
	 * square. 
	 * 
	 * @param size
	 * The pixel size of the square widget.
	 */
	virtual void setSize(int size) = 0;

	/** 
	 * Initialise the GL preview. This clears the window and sets up the 
	 * initial matrices and lights.
	 * Call this before showing the parent container.
	 */
	virtual void initialisePreview() = 0;

	/** 
	 * Set the widget to display the given particle. If the particle name is the 
	 * empty string, the widget will release the currently displayed one.
	 * 
	 * @param
	 * String name of the particle to display.
	 */
	virtual void setParticle(const std::string& particle) = 0;

	// Retrieve the widget to pack this element into a parent container
	virtual Gtk::Widget* getWidget() = 0;

	/** 
	 * Get the particle from the widget, in order to display properties about it.
	 */
	virtual particles::IParticleDefPtr getParticle() = 0;
};
typedef boost::shared_ptr<IParticlePreview> IParticlePreviewPtr;

} // namespace ui

#endif /* _IPARTICLE_PREVIEW_H_ */
