#ifndef MANIPULATORS_H_
#define MANIPULATORS_H_

/* greebo: This file contains the manipulator classes like 
 * 
 * - Translate Maniplator
 * - Rotate Manipulator
 * - Scale Manipulator
 * - Drag Manipulator
 * - Clip Manipulator
 * 
 * that derive from the abstract base class Manipulator
 * 
 * A Manipulator consists of several Manipulatables (e.g. a circle for the rotation manipulators) that
 * themselves derive from the Abstract Base Class <Manipulatable>
 */

#include "pivot.h"
#include "view.h"
#include "math/matrix.h"
#include "selectionlib.h"
#include "Manipulatables.h"
#include "Renderables.h"

struct Pivot2World {
  Matrix4 _worldSpace;
  Matrix4 _viewpointSpace;
  Matrix4 _viewplaneSpace;
  Vector3 _axisScreen;

  void update(const Matrix4& pivot2world, const Matrix4& modelview, const Matrix4& projection, const Matrix4& viewport)
  {
    Pivot2World_worldSpace(_worldSpace, pivot2world, modelview, projection, viewport);
    Pivot2World_viewpointSpace(_viewpointSpace, _axisScreen, pivot2world, modelview, projection, viewport);
    Pivot2World_viewplaneSpace(_viewplaneSpace, pivot2world, modelview, projection, viewport);
  }
};

// The (not purely) abstract base class for a manipulator
class Manipulator
{
public:
	// This returns a pointer to a manipulatable of this manipulator (this may point to the RadiantSelectionSystem itself) 
  	virtual Manipulatable* GetManipulatable() = 0;
  	virtual void testSelect(const View& view, const Matrix4& pivot2world) {}
  	
  	// This function is responsible for bringing the visual representation
  	// of this manipulator onto the screen
  	virtual void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world) {}
  	
  	virtual void setSelected(bool select) = 0;
  	virtual bool isSelected() const = 0;
};

// =======================================================================================

/*	The Manipulator for Rotations
 */
class RotateManipulator : public Manipulator {
private:
  RotateFree _rotateFree;
  RotateAxis _rotateAxis;
  Vector3 _axisScreen;
  RenderableSemiCircle _circleX;
  RenderableSemiCircle _circleY;
  RenderableSemiCircle _circleZ;
  RenderableCircle _circleScreen;
  RenderableCircle _circleSphere;
  SelectableBool _selectableX;
  SelectableBool _selectableY;
  SelectableBool _selectableZ;
  SelectableBool _selectableScreen;
  SelectableBool _selectableSphere;
  Pivot2World _pivot;
  Matrix4 _local2worldX;
  Matrix4 _local2worldY;
  Matrix4 _local2worldZ;
  bool _circleX_visible;
  bool _circleY_visible;
  bool _circleZ_visible;
  
public:
  static ShaderPtr _stateOuter;

  // Constructor
  RotateManipulator(Rotatable& rotatable, std::size_t segments, float radius);

  void UpdateColours();
  void updateCircleTransforms();
  
  void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world);
  
  void testSelect(const View& view, const Matrix4& pivot2world);
  
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);  
  bool isSelected() const;  
};

// =======================================================================================

/* The Manipulator for translation operations
 */ 
class TranslateManipulator : public Manipulator {
private:
  TranslateFree _translateFree;
  TranslateAxis _translateAxis;
  RenderableArrowLine _arrowX;
  RenderableArrowLine _arrowY;
  RenderableArrowLine _arrowZ;
  RenderableArrowHead _arrowHeadX;
  RenderableArrowHead _arrowHeadY;
  RenderableArrowHead _arrowHeadZ;
  RenderableQuad _quadScreen;
  SelectableBool _selectableX;
  SelectableBool _selectableY;
  SelectableBool _selectableZ;
  SelectableBool _selectableScreen;
  Pivot2World _pivot;
public:
  static ShaderPtr _stateWire;
  static ShaderPtr _stateFill;

  // Constructor
  TranslateManipulator(Translatable& translatable, std::size_t segments, float length);

  void UpdateColours();
  bool manipulator_show_axis(const Pivot2World& pivot, const Vector3& axis);
  
  void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world);
  void testSelect(const View& view, const Matrix4& pivot2world);
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);
  bool isSelected() const;  
}; // class TranslateManipulator

// =======================================================================================

/* The Manipulator for scale operations
 */ 
class ScaleManipulator : public Manipulator {
private:
  ScaleFree _scaleFree;
  ScaleAxis _scaleAxis;
  RenderableArrow _arrowX;
  RenderableArrow _arrowY;
  RenderableArrow _arrowZ;
  RenderableQuad _quadScreen;
  SelectableBool _selectableX;
  SelectableBool _selectableY;
  SelectableBool _selectableZ;
  SelectableBool _selectableScreen;
  Pivot2World _pivot;
  
public:
  // Constructor
  ScaleManipulator(Scalable& scalable, std::size_t segments, float length);

  Pivot2World& getPivot() {
    return _pivot;
  }

  void UpdateColours();
  
  void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world);
  void testSelect(const View& view, const Matrix4& pivot2world);
  Manipulatable* GetManipulatable();
  
  void setSelected(bool select);
  bool isSelected() const;
  
}; // class ScaleManipulator

// =======================================================================================

class DragManipulator : public Manipulator
{
  TranslateFree _freeResize;
  TranslateFree _freeDrag;
  ResizeTranslatable _resizeTranslatable;
  DragTranslatable _dragTranslatable;
  SelectableBool _dragSelectable;
public:
  bool _selected;

  DragManipulator() : _freeResize(_resizeTranslatable), _freeDrag(_dragTranslatable), _selected(false) {}

  Manipulatable* GetManipulatable();
  void testSelect(const View& view, const Matrix4& pivot2world);
  
  void setSelected(bool select);
  bool isSelected() const;
}; // class DragManipulator

// =======================================================================================

class ClipManipulator : public Manipulator {
public:
  Manipulatable* GetManipulatable() {
    ERROR_MESSAGE("clipper is not manipulatable");
    return 0;
  }

  void setSelected(bool select) {}
  bool isSelected() const {
    return false;
  }
}; // class ClipManipulator

#endif /*MANIPULATORS_H_*/
