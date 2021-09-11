#pragma once

#include <memory>
#include <string>

template<typename Element> class BasicVector2;
typedef BasicVector2<double> Vector2;

class Matrix4;
class VolumeTest;
class SelectionTest;
class RenderableCollector;

namespace selection
{

struct WorkZone;

/**
* A Manipulator is a renderable object which contains one or more
* ManipulatorComponents, each of which can be manipulated by the user. For
* example, the rotation Manipulator draws several circles which cause rotations
* around specific axes.
*/
class Manipulator
{
public:
	// Manipulator type enum, user-defined manipulators should return "Custom"
	enum Type
	{
		Drag,
		Translate,
		Rotate,
		Scale,
		Clip,
		ModelScale,
		Custom
	};

	/**
	* Part of a Manipulator which can be operated upon by the user.
	*
	* \see Manipulator
	*/
	class Component
	{
	public:
		virtual ~Component() {}

		/**
		 * Called when the user successfully activates this component. The calling code provides
		 * information about the view we're operating in, the starting device coords and the
		 * location of the current selection pivot.
		 */
		virtual void beginTransformation(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint) = 0;

		struct Constraint
		{
			enum Flags
			{
				Unconstrained = 0,	// no keyboard modifier held
				Type1 = 1 << 0,		// usually: shift held down
				Grid =  1 << 1,		// usually: ctrl NOT held down
				Type3 = 1 << 2,		// usually: alt held down
			};
		};

		/**
		 * Called during mouse movement, the component is asked to calculate the deltas and distances
		 * it needs to perform the translation/rotation/scale/whatever the operator does on the selected objects.
		 * The pivot2world transform relates to the original pivot location at the time the transformation started.
		 * If the constrained flags are not 0, they indicate the user is holding down a key during movement,
		 * usually the SHIFT or CTRL key. It's up to the component to decide how to handle the constraint.
		 */
		virtual void transform(const Matrix4& pivot2world, const VolumeTest& view, const Vector2& devicePoint, unsigned int flags) = 0;
	};

	virtual ~Manipulator() {}

	// ID and Type management
	virtual std::size_t getId() const = 0;
	virtual void setId(std::size_t id) = 0;

	virtual Type getType() const = 0;

	/**
	* Get the currently-active ManipulatorComponent. This is determined by the
	* most recent selection test.
	*/
	virtual Component* getActiveComponent() = 0;

	virtual void testSelect(SelectionTest& test, const Matrix4& pivot2world) {}

	// Renders the manipulator's visual representation to the scene
	virtual void render(RenderableCollector& collector, const VolumeTest& volume) = 0;

	virtual void setSelected(bool select) = 0;
	virtual bool isSelected() const = 0;

	// Manipulators should indicate whether component editing is supported or not
	virtual bool supportsComponentManipulation() const = 0;
};
typedef std::shared_ptr<Manipulator> ManipulatorPtr;

}
