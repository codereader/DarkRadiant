#include "TexToolItem.h"

namespace textool {

void TexToolItem::addChild(TexToolItemPtr child) {
	_children.push_back(child);
}

TexToolItemVec& TexToolItem::getChildren() {
	return _children;
}

void TexToolItem::foreachItem(ItemVisitor& visitor) {
	for (std::size_t i = 0; i < _children.size(); i++) {
		// Visit the children
		visitor.visit(_children[i]);
		
		// Propagate the visitor class down the hierarchy
		_children[i]->foreachItem(visitor);
	}
}

TexToolItemVec TexToolItem::getSelectableChilds(const Rectangle& rectangle) {
	TexToolItemVec returnVector;
	
	for (std::size_t i = 0; i < _children.size(); i++) {
		// Add every children to the list
		if (_children[i]->testSelect(rectangle)) {
			returnVector.push_back(_children[i]);
		}
	}
	
	return returnVector;
}

void TexToolItem::transform(const Matrix4& matrix) {
	// Cycle through all the children and ask them to render themselves
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->transform(matrix);
	}
	update();
}

void TexToolItem::transformSelected(const Matrix4& matrix) {
	// If this object is selected, transform <self>
	if (_selected) {
		transform(matrix);
	}
	else {
		// Object is not selected, propagate the call to the children
		for (std::size_t i = 0; i < _children.size(); i++) {
			_children[i]->transformSelected(matrix);
		}
	}
	update();
}

void TexToolItem::flipSelected(const int& axis) {
	// Default behaviour: Propagate the call to the children
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->flipSelected(axis);
	}
	update();
}

void TexToolItem::snapSelectedToGrid(float grid) {
	// Default behaviour: Propagate the call to the children
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->snapSelectedToGrid(grid);
	}
	update();
}

AABB TexToolItem::getExtents() {
	AABB returnValue;
	
	// Cycle through all the children and include their AABB
	for (std::size_t i = 0; i < _children.size(); i++) {
		returnValue.includeAABB(_children[i]->getExtents());
	}
	
	return returnValue;
}

AABB TexToolItem::getSelectedExtents() {
	AABB returnValue;
	
	// Add <self> to the resulting AABB if <self> is selected
	if (_selected) {
		returnValue.includeAABB(getExtents());
	}
	
	// Cycle through all the children and include their AABB
	for (std::size_t i = 0; i < _children.size(); i++) {
		if (_children[i]->isSelected()) {
			returnValue.includeAABB(_children[i]->getExtents());
		}
	}
	
	return returnValue;
}

void TexToolItem::moveSelectedTo(const Vector2& targetCoords) {
	// Default: Cycle through all children and move the selected
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->moveSelectedTo(targetCoords);
	}
	update();
}

void TexToolItem::render() {
	// Cycle through all the children and ask them to render themselves
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->render();
	}
}

void TexToolItem::beginTransformation() {
	// Cycle through all the children and pass the call
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->beginTransformation();
	}
}

void TexToolItem::endTransformation() {
	// Cycle through all the children and pass the call
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->endTransformation();
	}
	update();
}

void TexToolItem::selectRelated() {
	// Default: Cycle through all the children and pass the call
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->selectRelated();
	}
}

void TexToolItem::update() {
	// Default: Cycle through all the children and pass the call
	for (std::size_t i = 0; i < _children.size(); i++) {
		_children[i]->update();
	}
}

} // namespace textool
