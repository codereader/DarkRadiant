#include "ShaderSelector.h"

#include "i18n.h"

#include "wxutil/dataview/TreeView.h"
#include "wxutil/dataview/VFSTreePopulator.h"
#include "wxutil/DeclarationSourceView.h"

#include "texturelib.h"
#include "ishaders.h"

#include <wx/dataview.h>
#include <wx/sizer.h>

#include <GL/glew.h>

#include <vector>
#include <string>

#include "string/split.h"
#include "string/predicate.h"
#include <functional>

#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

/* CONSTANTS */

namespace
{
	constexpr const char* const TEXTURE_ICON = "icon_texture.png";
}

/**
 * Visitor class to retrieve material names and add them to folders.
 */
class ThreadedMaterialLoader final :
    public wxutil::ThreadedDeclarationTreePopulator
{
private:
    const wxutil::DeclarationTreeView::Columns& _columns;
    const ShaderSelector::PrefixList& _prefixes;

public:
    ThreadedMaterialLoader(const wxutil::DeclarationTreeView::Columns& columns, const ShaderSelector::PrefixList& prefixes) :
        ThreadedDeclarationTreePopulator(decl::Type::Material, columns, TEXTURE_ICON),
        _columns(columns),
        _prefixes(prefixes)
    {}

    ~ThreadedMaterialLoader()
    {
        EnsureStopped();
    }

protected:
    void PopulateModel(const wxutil::TreeModel::Ptr& model) override
    {
        wxutil::VFSTreePopulator populator(model);

        GlobalMaterialManager().foreachShaderName([&](const std::string& materialName)
        {
            for (const std::string& prefix : _prefixes)
            {
                if (string::istarts_with(materialName, prefix + "/"))
                {
                    populator.addPath(materialName, [&](wxutil::TreeModel::Row& row,
                        const std::string& path, const std::string& leafName, bool isFolder)
                    {
                        AssignValuesToRow(row, path, path, leafName, isFolder);
                    });
                    break; // don't consider any further prefixes
                }
            }
        });
    }

    void SortModel(const wxutil::TreeModel::Ptr& model) override
    {
        model->SortModelFoldersFirst(_columns.leafName, _columns.isFolder);
    }
};

// Constructor creates elements
ShaderSelector::ShaderSelector(wxWindow* parent, Client* client, const std::string& prefixes, bool isLightTexture) :
	wxPanel(parent, wxID_ANY),
	_treeView(nullptr),
	_glWidget(nullptr),
	_client(client),
	_isLightTexture(isLightTexture),
	_infoStore(new wxutil::TreeModel(_infoStoreColumns, true)) // is a listmodel
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// Split the given comma-separated list into the vector
	string::split(_prefixes, prefixes, ",");

	// Pack in TreeView and info panel
	createTreeView();
	createPreview();
}

void ShaderSelector::_onShowShaderDefinition()
{
    // Construct a definition view and pass the material name
    auto view = new wxutil::DeclarationSourceView(this);
    view->setDeclaration(decl::Type::Material, getSelection());

    view->ShowModal();
    view->Destroy();
}

// Return the selection to the calling code
std::string ShaderSelector::getSelection()
{
    return _treeView->GetSelectedDeclName();
}

// Set the selection in the treeview
void ShaderSelector::setSelection(const std::string& sel)
{
    _treeView->SetSelectedDeclName(sel);
}

void ShaderSelector::createTreeView()
{
    _treeView = new wxutil::DeclarationTreeView(this, decl::Type::Material, 
        _shaderTreeColumns, wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/shader name and the icon
    _treeView->AppendIconTextColumn(_("Value"), _shaderTreeColumns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_shaderTreeColumns.leafName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ShaderSelector::_onSelChange, this);

    GetSizer()->Add(_treeView, 1, wxEXPAND);

    _treeView->Populate(std::make_shared<ThreadedMaterialLoader>(_shaderTreeColumns, _prefixes));
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
