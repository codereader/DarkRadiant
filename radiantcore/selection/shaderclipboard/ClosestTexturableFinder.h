#pragma once

#include "inode.h"
#include "iselectiontest.h"

namespace selection
{

class Texturable;

namespace algorithm 
{

class ClosestTexturableFinder :
	public scene::NodeVisitor
{
private:
	Texturable& _texturable;
	SelectionTest& _selectionTest;

	// To store the best intersection candidate
	SelectionIntersection _bestIntersection;
public:
	// Constructor
	ClosestTexturableFinder(SelectionTest& test, Texturable& texturable);

	// The visitor function
	bool pre(const scene::INodePtr& node);
};

} // namespace

} // namespace
