#pragma once

#include "ieclass.h"

#include "AnimationPreview.h"
#include "icommandsystem.h"
#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeView.h"

namespace ui
{

class MD5AnimationViewer :
	public wxutil::DialogBase
{
public:
	struct AnimListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		wxutil::TreeModel::Column name;
		wxutil::TreeModel::Column filename;

		AnimListColumns() :
			name(add(wxutil::TreeModel::Column::String)),
			filename(add(wxutil::TreeModel::Column::String))
		{}
	};

	enum class RunMode
	{
		ViewOnly,	// viewing purposes, provides just an OK button
		Selection,	// selection purposes
	};

private:
	RunMode _runMode;

	wxutil::DeclarationTreeView::Columns _modelColumns;
	wxutil::DeclarationTreeView* _modelTreeView;

	AnimListColumns _animColumns;

	// Liststore for the anim list, and its selection object
	wxutil::TreeModel::Ptr _animList;
	wxutil::TreeView* _animTreeView;

	// Animation preview widget
	AnimationPreviewPtr _preview;

	std::string _modelToSelect;
	std::string _animToSelect;

protected:
	MD5AnimationViewer(wxWindow* parent, RunMode runMode);

public:
	static void Show(const cmd::ArgumentList& args);

	std::string getSelectedModel();
	void setSelectedModel(const std::string& model);

	std::string getSelectedAnim();
	void setSelectedAnim(const std::string& anim);

private:
	// callbacks
	void _onModelSelChanged(wxDataViewEvent& ev);
	void handleModelSelectionChange();

	void _onAnimSelChanged(wxDataViewEvent& ev);
	void handleAnimSelectionChange();

	wxWindow* createListPane(wxWindow* parent);
	wxWindow* createModelTreeView(wxWindow* parent);
	wxWindow* createAnimTreeView(wxWindow* parent);

	// Populate with names
	void populateModelList();
	void populateAnimationList();

	IModelDef::Ptr getSelectedModelDef();
};

}
