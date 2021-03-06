#include "ShaderSelector.h"

#include "i18n.h"

#include "wxutil/dataview/TreeModel.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "ui/materials/MaterialDefinitionView.h"

#include "texturelib.h"
#include "string/string.h"
#include "ishaders.h"
#include "iregistry.h"

#include <wx/dataview.h>
#include <wx/sizer.h>
#include "wxutil/Bitmap.h"

#include <GL/glew.h>

#include <vector>
#include <string>
#include <map>

#include "string/split.h"
#include "string/predicate.h"
#include "string/case_conv.h"
#include <functional>

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* const FOLDER_ICON = "folder16.png";
	const char* const TEXTURE_ICON = "icon_texture.png";

    constexpr const char* const SHOW_MATERIAL_DEF_TEXT = N_("Show Material Definition");
    constexpr const char* const SHOW_MATERIAL_DEF_ICON = "icon_script.png";
}

// Constructor creates elements
ShaderSelector::ShaderSelector(wxWindow* parent, Client* client, const std::string& prefixes, bool isLightTexture) :
	wxPanel(parent, wxID_ANY),
	_treeView(NULL),
	_treeStore(NULL),
	_glWidget(NULL),
	_client(client),
	_isLightTexture(isLightTexture),
	_infoStore(new wxutil::TreeModel(_infoStoreColumns, true)), // is a listmodel
    _contextMenu(new wxutil::PopupMenu)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Split the given comma-separated list into the vector
	string::split(_prefixes, prefixes, ",");

	// Pack in TreeView and info panel
	createTreeView();
	createPreview();

    // Construct the context menu
    _contextMenu->addItem(
        new wxutil::IconTextMenuItem(_(SHOW_MATERIAL_DEF_TEXT), SHOW_MATERIAL_DEF_ICON),
        std::bind(&ShaderSelector::_onShowShaderDefinition, this),
        [this]() { return !getSelection().empty(); }
    );
}

void ShaderSelector::_onShowShaderDefinition()
{
    // Construct a definition view and pass the material name
    auto view = new MaterialDefinitionView(getSelection());
    view->ShowModal();
    view->Destroy();
}

// Return the selection to the calling code
std::string ShaderSelector::getSelection()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (!item.IsOk()) return "";

	wxutil::TreeModel::Row row(item, *_treeView->GetModel());

	return row[_shaderTreeColumns.shaderName];
}

// Set the selection in the treeview
void ShaderSelector::setSelection(const std::string& sel)
{
	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (sel.empty())
	{
        _treeView->UnselectAll();
		_treeView->Collapse(_treeStore->GetRoot());
		return;
	}

	// Use the wxutil TreeModel algorithms to select the shader
	wxDataViewItem found = _treeStore->FindString(
		string::to_lower_copy(sel), _shaderTreeColumns.shaderName);

	if (found.IsOk())
	{
		_treeView->Select(found);
		_treeView->EnsureVisible(found);
	}
}

namespace
{
	// VFSPopulatorVisitor to fill in column data for the populator tree nodes
	class DataInserter :
		public wxutil::VFSTreePopulator::Visitor
	{
	private:
		const ShaderSelector::ShaderTreeColumns& _columns;

		wxIcon _folderIcon;
		wxIcon _textureIcon;

	public:
		DataInserter(const ShaderSelector::ShaderTreeColumns& columns) :
			_columns(columns)
		{
			_folderIcon.CopyFromBitmap(
				wxutil::GetLocalBitmap(FOLDER_ICON));
			_textureIcon.CopyFromBitmap(
				wxutil::GetLocalBitmap(TEXTURE_ICON));
		}

		virtual ~DataInserter() {}

		// Required visit function
		void visit(wxutil::TreeModel& /* store */,
				   wxutil::TreeModel::Row& row,
				   const std::string& path,
				   bool isExplicit)
		{
			// Get the display name by stripping off everything before the last
			// slash
			std::string displayName = path.substr(path.rfind("/") + 1);

			// Pathname is the model VFS name for a model, and blank for a folder
			std::string fullPath = isExplicit ? path : "";

			// Fill in the column values
			row[_columns.iconAndName] = wxVariant(
				wxDataViewIconText(displayName, isExplicit ? _textureIcon : _folderIcon));
			row[_columns.shaderName] = fullPath;
		}
	};
}

// Create the Tree View
void ShaderSelector::createTreeView()
{
    _treeStore = new wxutil::TreeModel(_shaderTreeColumns);

    // Instantiate the helper class that populates the tree according to the paths
    wxutil::VFSTreePopulator populator(_treeStore);

    // Iterate over material names and pass matching shaders to the populator
    GlobalMaterialManager().foreachShaderName(
        [&](const std::string& shaderName)
        {
            for (const std::string& prefix : _prefixes)
            {
                if (!shaderName.empty() && string::istarts_with(shaderName, prefix + "/"))
                {
                    populator.addPath(string::to_lower_copy(shaderName));
                    break; // don't consider any further prefixes
                }
            }
        }
    );

    // Now visit the created iterators to load the actual data into the tree
    DataInserter inserter(_shaderTreeColumns);
    populator.forEachNode(inserter);

    // Tree view
    _treeView = wxutil::TreeView::CreateWithModel(this, _treeStore.get(), wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/shader name and the icon
    _treeView->AppendIconTextColumn(_("Value"), _shaderTreeColumns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_shaderTreeColumns.iconAndName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ShaderSelector::_onSelChange, this);
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &ShaderSelector::_onContextMenu, this);

    GetSizer()->Add(_treeView, 1, wxEXPAND);
}

void ShaderSelector::_onContextMenu(wxDataViewEvent& ev)
{
    _contextMenu->show(_treeView);
}

// Create the preview panel (GL widget and info table)
void ShaderSelector::createPreview()
{
	// HBox contains the preview GL widget along with a texture attributes pane.
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	// Cast the GLWidget object to GtkWidget
	_glWidget = new wxutil::GLWidget(this, std::bind(&ShaderSelector::onPreviewRender, this), "ShaderSelector");
	_glWidget->SetMinClientSize(wxSize(128, 128));

	// Attributes table
	wxDataViewCtrl* tree = new wxDataViewCtrl(this, wxID_ANY,
		wxDefaultPosition, wxDefaultSize, wxDV_NO_HEADER | wxDV_SINGLE);

	tree->AssociateModel(_infoStore.get());

	tree->AppendTextColumn(_("Attribute"), _infoStoreColumns.attribute.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
	tree->AppendTextColumn(_("Value"), _infoStoreColumns.value.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

	sizer->Add(_glWidget, 0, wxEXPAND);
	sizer->Add(tree, 1, wxEXPAND);

	GetSizer()->Add(sizer, 0, wxEXPAND | wxTOP, 3);
}

// Get the selected shader
MaterialPtr ShaderSelector::getSelectedShader() {
	return GlobalMaterialManager().getMaterial(getSelection());
}

// Update the attributes table
void ShaderSelector::updateInfoTable()
{
	_infoStore->Clear();

	// Get the selected texture name. If nothing is selected, we just leave the
	// infotable empty.
	std::string selName = getSelection();

	// Notify the client of the change to give it a chance to update the infostore
	if (_client != NULL && !selName.empty())
	{
		_client->shaderSelectionChanged(selName, *_infoStore);
	}
}

// Callback to redraw the GL widget
bool ShaderSelector::onPreviewRender()
{
	// Get the viewport size from the GL widget
	wxSize req = _glWidget->GetClientSize();

	if (req.GetWidth() == 0 || req.GetHeight() == 0) return false;

	glViewport(0, 0, req.GetWidth(), req.GetHeight());

	// Initialise
	glClearColor(0.3f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	glOrtho(0, req.GetWidth(), 0, req.GetHeight(), -100, 100);
	glEnable (GL_TEXTURE_2D);

	// Get the selected texture, and set up OpenGL to render it on
	// the quad.
	MaterialPtr shader = getSelectedShader();

	bool drawQuad = false;
	TexturePtr tex;

	// Check what part of the shader we should display in the preview
	if (_isLightTexture) {
		// This is a light, take the first layer texture
		const IShaderLayer* first = shader->firstLayer();
		if (first != NULL) {
			tex = shader->firstLayer()->getTexture();
			glBindTexture (GL_TEXTURE_2D, tex->getGLTexNum());
			drawQuad = true;
		}
	}
	else {
		// This is an "ordinary" texture, take the editor image
		tex = shader->getEditorImage();
		if (tex != NULL) {
			glBindTexture (GL_TEXTURE_2D, tex->getGLTexNum());
			drawQuad = true;
		}
	}

	if (drawQuad)
    {
		// Calculate the correct aspect ratio for preview.
      float aspect = float(tex->getWidth()) / float(tex->getHeight());

		float hfWidth, hfHeight;
		if (aspect > 1.0) {
			hfWidth = 0.5*req.GetWidth();
			hfHeight = 0.5*req.GetHeight() / aspect;
		}
		else {
			hfHeight = 0.5*req.GetWidth();
			hfWidth = 0.5*req.GetHeight() * aspect;
		}

		// Draw a quad to put the texture on
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex2f(0.5*req.GetWidth() - hfWidth, 0.5*req.GetHeight() - hfHeight);
		glTexCoord2i(1, 1);
		glVertex2f(0.5*req.GetWidth() + hfWidth, 0.5*req.GetHeight() - hfHeight);
		glTexCoord2i(1, 0);
		glVertex2f(0.5*req.GetWidth() + hfWidth, 0.5*req.GetHeight() + hfHeight);
		glTexCoord2i(0, 0);
		glVertex2f(0.5*req.GetWidth() - hfWidth, 0.5*req.GetHeight() + hfHeight);
		glEnd();
	}

	return true;
}

namespace
{

// Helper function
void addInfoItem(wxutil::TreeModel& listStore, const std::string& attr, const std::string& value,
				 int attrCol, int valueCol)
{
	wxDataViewItemAttr bold;
	bold.SetBold(true);

	wxDataViewItem item = listStore.AddItem().getItem();

	listStore.SetValue(attr, item, attrCol);
	listStore.SetAttr(item, attrCol, bold);
	listStore.SetValue(value, item, valueCol);

	listStore.ItemAdded(listStore.GetRoot(), item);
}

}

void ShaderSelector::displayShaderInfo(const MaterialPtr& shader,
									   wxutil::TreeModel& listStore,
									   int attrCol, int valueCol)
{
	// Update the infostore in the ShaderSelector
	addInfoItem(listStore, _("Shader"), shader->getName(), attrCol, valueCol);
	addInfoItem(listStore, _("Defined in"), shader->getShaderFileName(), attrCol, valueCol);
	addInfoItem(listStore, _("Description"), shader->getDescription(), attrCol, valueCol);
}

void ShaderSelector::displayLightShaderInfo(const MaterialPtr& shader,
											wxutil::TreeModel& listStore,
											int attrCol, int valueCol)
{
	const IShaderLayer* first = shader->firstLayer();
	std::string texName = _("None");

	if (first != NULL)
	{
		TexturePtr tex = shader->firstLayer()->getTexture();
		texName = tex->getName();
	}

	addInfoItem(listStore, _("Image map"), texName, attrCol, valueCol);
	addInfoItem(listStore, _("Defined in"), shader->getShaderFileName(), attrCol, valueCol);

	// Light types, from the Material
	std::string lightType;
	if (shader->isAmbientLight())
		lightType.append("ambient ");
	if (shader->isBlendLight())
		lightType.append("blend ");
	if (shader->isFogLight())
		lightType.append("fog");
	if (lightType.size() == 0)
		lightType.append("-");

	addInfoItem(listStore, _("Light flags"), lightType, attrCol, valueCol);

	std::string descr = shader->getDescription();
	addInfoItem(listStore, _("Description"), descr.empty() ? "-" : descr, attrCol, valueCol);
}

// Callback for selection changed
void ShaderSelector::_onSelChange(wxDataViewEvent& ev)
{
	updateInfoTable();
	_glWidget->Refresh();
}

} // namespace ui
