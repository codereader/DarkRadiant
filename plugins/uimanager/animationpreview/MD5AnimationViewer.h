#pragma once

#include "imd5model.h"
#include "ieclass.h"

#include "AnimationPreview.h"
#include "icommandsystem.h"
#include "wxutil/VFSTreePopulator.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeModel.h"
#include "wxutil/TreeView.h"

namespace ui
{

class MD5AnimationViewer :
	public wxutil::DialogBase,
	public ModelDefVisitor,
	public wxutil::VFSTreePopulator::Visitor
{
public:
	// Treemodel definitions
	struct ModelListColumns :
		public wxutil::TreeModel::ColumnRecord
	{
		wxutil::TreeModel::Column name;

		ModelListColumns() : 
			name(add(wxutil::TreeModel::Column::String)) 
		{}
	};

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

	ModelListColumns _modelColumns;

	// Liststore for the model list, and its selection object
	wxutil::TreeModel::Ptr _modelList;
	wxutil::TreeView* _modelTreeView;

	wxutil::VFSTreePopulator _modelPopulator;

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

	void visit(const IModelDefPtr& modelDef);

	void visit(wxutil::TreeModel& store,
				wxutil::TreeModel::Row& row,
				const std::string& path,
				bool isExplicit);

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

	IModelDefPtr getSelectedModelDef();
};

}
