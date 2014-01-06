#pragma once

#include "Manipulator.h"
#include "Manipulatables.h"
#include "Renderables.h"
#include "Pivot2World.h"

#include "BasicSelectable.h"

/**
 * The Manipulator for scale operations
 */
class ScaleManipulator
: public Manipulator
{
private:
  ScaleFree _scaleFree;
  ScaleAxis _scaleAxis;
  RenderableArrowLine _arrowX;
  RenderableArrowLine _arrowY;
  RenderableArrowLine _arrowZ;
  RenderableQuad _quadScreen;
  selection::BasicSelectable _selectableX;
  selection::BasicSelectable _selectableY;
  selection::BasicSelectable _selectableZ;
  selection::BasicSelectable _selectableScreen;
  Pivot2World _pivot;

public:
  // Constructor
  ScaleManipulator(Scalable& scalable, std::size_t segments, float length);

  Pivot2World& getPivot() {
    return _pivot;
  }

  void UpdateColours();

  void render(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& pivot2world);
  void testSelect(const render::View& view, const Matrix4& pivot2world);
  ManipulatorComponent* getActiveComponent();

  void setSelected(bool select);
  bool isSelected() const;

}; // class ScaleManipulator

