#include "MD5AnimationViewer.h"

#include "i18n.h"
#include "imainframe.h"
#include "imodelcache.h"
#include "imd5anim.h"

#include <wx/splitter.h>
#include <wx/stattext.h>
#include <wx/sizer.h>

namespace ui
{

MD5AnimationViewer::MD5AnimationViewer(wxWindow* parent, RunMode runMode) :
	wxutil::DialogBase(_("MD5 Animation Viewer"), parent),
	_runMode(runMode),
	_modelList(new wxutil::TreeModel(_modelColumns)),
	_modelPopulator(_modelList),
	_animList(new wxutil::TreeModel(_animColumns, true))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxSplitterWindow* splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition,
		wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
	splitter->SetMinimumPaneSize(10); // disallow unsplitting

									  // Preview goes to the right
	_preview.reset(new AnimationPreview(splitter));

	splitter->SplitVertically(createListPane(splitter), _preview->getWidget());

	GetSizer()->Add(splitter, 1, wxEXPAND | wxALL, 12);

	if (_runMode == RunMode::Selection)
	{
		GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
		SetAffirmativeId(wxID_OK);
	}
	else
	{
		GetSizer()->Add(CreateStdDialogButtonSizer(wxCLOSE), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
		SetAffirmativeId(wxID_CLOSE);
	}

	FitToScreen(0.8f, 0.7f);

	// Set the default size of the window
	splitter->SetSashPosition(static_cast<int>(GetSize().GetWidth() * 0.25f));

	// Populate with model names
	populateModelList();

	Bind(wxEVT_IDLE, [&](wxIdleEvent& ev)
	{
		ev.Skip();

		if (!_modelToSelect.empty())
		{
			setSelectedModel(_modelToSelect);
		}

		if (!_animToSelect.empty())
		{
			setSelectedAnim(_animToSelect);
		}
	});
}

void MD5AnimationViewer::Show(const cmd::ArgumentList& args)
{
	MD5AnimationViewer* viewer = new MD5AnimationViewer(nullptr, RunMode::ViewOnly);

	viewer->ShowModal();
	viewer->Destroy();
}

wxWindow* MD5AnimationViewer::createListPane(wxWindow* parent)
{
	wxPanel* listPane = new wxPanel(parent, wxID_ANY);
	listPane->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxStaticText* modelLabel = new wxStaticText(listPane, wxID_ANY, _("Model Definition"));
	modelLabel->SetFont(modelLabel->GetFont().Bold());

	wxStaticText* animLabel = new wxStaticText(listPane, wxID_ANY, _("Available Animations"));
	animLabel->SetFont(animLabel->GetFont().Bold());
	
	listPane->GetSizer()->Add(modelLabel, 0, wxEXPAND | wxBOTTOM, 6);
	listPane->GetSizer()->Add(createModelTreeView(listPane), 1, wxEXPAND | wxBOTTOM | wxTOP, 6);
	listPane->GetSizer()->Add(animLabel, 0, wxEXPAND | wxBOTTOM | wxTOP, 6);
	listPane->GetSizer()->Add(createAnimTreeView(listPane), 1, wxEXPAND | wxBOTTOM | wxTOP, 6);

	return listPane;
}

wxWindow* MD5AnimationViewer::createModelTreeView(wxWindow* parent)
{
	_modelTreeView = wxutil::TreeView::CreateWithModel(parent, _modelList, wxDV_SINGLE | wxDV_NO_HEADER);
	_modelTreeView->SetMinClientSize(wxSize(300, -1));

	// Single text column
	_modelTreeView->AppendTextColumn(_("Model Definition"), _modelColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	// Apply full-text search to the column
	_modelTreeView->AddSearchColumn(_modelColumns.name);

	// Connect up the selection changed callback
	_modelTreeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(MD5AnimationViewer::_onModelSelChanged), NULL, this);

	// Pack into scrolled window and return
	return _modelTreeView;
}

wxWindow* MD5AnimationViewer::createAnimTreeView(wxWindow* parent)
{
	_animTreeView = wxutil::TreeView::CreateWithModel(parent, _animList, wxDV_SINGLE | wxDV_NO_HEADER);
	_animTreeView->EnableAutoColumnWidthFix(false);

	_animTreeView->SetMinClientSize(wxSize(300, -1));

	// Single text column
	_animTreeView->AppendTextColumn(_("Animation"), _animColumns.name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
	_animTreeView->AppendTextColumn(_("File"), _animColumns.filename.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	// Apply full-text search to the column
	_animTreeView->AddSearchColumn(_animColumns.name);

	// Connect up the selection changed callback
	_animTreeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(MD5AnimationViewer::_onAnimSelChanged), NULL, this);

	return _animTreeView;
}

void MD5AnimationViewer::_onModelSelChanged(wxDataViewEvent& ev)
{
	handleModelSelectionChange();
}

void MD5AnimationViewer::handleModelSelectionChange()
{
	IModelDefPtr modelDef = getSelectedModelDef();

	if (!modelDef)
	{
		_animTreeView->Enable(false);
		return;
	}

	_animTreeView->Enable(true);

	scene::INodePtr modelNode =  GlobalModelCache().getModelNode(modelDef->mesh);
	_preview->setAnim(md5::IMD5AnimPtr());
	_preview->setModelNode(modelNode);
	
	populateAnimationList();
}

std::string MD5AnimationViewer::getSelectedModel()
{
	wxDataViewItem item = _modelTreeView->GetSelection();

	if (!item.IsOk())
	{
		return std::string();
	}

	wxutil::TreeModel::Row row(item, *_modelList);

	return static_cast<std::string>(row[_modelColumns.name]);
}

void MD5AnimationViewer::setSelectedModel(const std::string& model)
{
	if (IsShownOnScreen())
	{
		wxDataViewItem item = _modelList->FindString(model, _modelColumns.name);

		if (item.IsOk())
		{
			_modelTreeView->Select(item);
			_modelTreeView->EnsureVisible(item);
			handleModelSelectionChange();
		}

		_modelToSelect.clear();
		return;
	}

	_modelToSelect = model;
}

std::string MD5AnimationViewer::getSelectedAnim()
{
	wxDataViewItem item = _animTreeView->GetSelection();

	if (!item.IsOk())
	{
		return std::string();
	}

	wxutil::TreeModel::Row row(item, *_animList);
	return static_cast<std::string>(row[_animColumns.name]);
}

void MD5AnimationViewer::setSelectedAnim(const std::string& anim)
{
	if (IsShownOnScreen())
	{
		wxDataViewItem item = _animList->FindString(anim, _animColumns.name);

		if (item.IsOk())
		{
			_animTreeView->Select(item);
			_animTreeView->EnsureVisible(item);
			handleAnimSelectionChange();
		}

		_animToSelect.clear();
		return;
	}

	_animToSelect = anim;
}

IModelDefPtr MD5AnimationViewer::getSelectedModelDef()
{
	std::string modelDefName = getSelectedModel();

	if (modelDefName.empty())
	{
		return IModelDefPtr();
	}

	return GlobalEntityClassManager().findModel(modelDefName);
}

void MD5AnimationViewer::_onAnimSelChanged(wxDataViewEvent& ev)
{
	handleAnimSelectionChange();
}

void MD5AnimationViewer::handleAnimSelectionChange()
{
	IModelDefPtr modelDef = getSelectedModelDef();

	if (!modelDef) 
	{
		_preview->setAnim(md5::IMD5AnimPtr());
		return;
	}

	wxDataViewItem item = _animTreeView->GetSelection();

	if (!item.IsOk())
	{
		_preview->setAnim(md5::IMD5AnimPtr());
		return;
	}

	wxutil::TreeModel::Row row(item, *_animList);
	std::string filename = row[_animColumns.filename];

	// Assign preview animation
	md5::IMD5AnimPtr anim = GlobalAnimationCache().getAnim(filename);
	_preview->setAnim(anim);
}

void MD5AnimationViewer::visit(const IModelDefPtr& modelDef)
{
	_modelPopulator.addPath(modelDef->getModName() + "/" + modelDef->name);
}

void MD5AnimationViewer::visit(wxutil::TreeModel& /* store */,
	wxutil::TreeModel::Row& row, const std::string& path, bool isExplicit)
{
	// Get the display path, everything after rightmost slash
	row[_modelColumns.name] = path.substr(path.rfind("/") + 1);

	row.SendItemAdded();
}

void MD5AnimationViewer::populateModelList()
{
	_modelList->Clear();

	GlobalEntityClassManager().forEachModelDef(*this);

	_modelPopulator.forEachNode(*this);

	_modelTreeView->ExpandTopLevelItems();
}

void MD5AnimationViewer::populateAnimationList()
{
	_animList->Clear();

	IModelDefPtr modelDef = getSelectedModelDef();

	if (!modelDef) return;

	for (IModelDef::Anims::const_iterator i = modelDef->anims.begin(); i != modelDef->anims.end(); ++i)
	{
		wxutil::TreeModel::Row row = _animList->AddItem();

		row[_animColumns.name] = i->first;		// anim name
		row[_animColumns.filename] = i->second;	// anim filename

		row.SendItemAdded();
	}
}

} // namespace
