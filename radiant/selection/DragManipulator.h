#pragma once

#include "Manipulator.h"
#include "Manipulatables.h"

#include "selectionlib.h"

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

  ManipulatorComponent* getActiveComponent();
  void testSelect(const View& view, const Matrix4& pivot2world);
  
  void setSelected(bool select);
  bool isSelected() const;
}; // class DragManipulator

