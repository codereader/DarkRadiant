#pragma once

#include <sigc++/connection.h>
#include "ideclmanager.h"
#include "DefinitionView.h"

namespace wxutil
{

/**
 * DefinitionView implementation specialising on Declarations
 */
class DeclarationSourceView :
    public DefinitionView
{
private:
    decl::IDeclaration::Ptr _decl;
    decl::Type _activeSourceViewType;

    sigc::connection _declChangedConn;

public:
    DeclarationSourceView(wxWindow* parent);

    ~DeclarationSourceView() override;

    void setDeclaration(const decl::IDeclaration::Ptr& decl);

    void setDeclaration(decl::Type type, const std::string& declName);

protected:
    bool isEmpty() const override;
    std::string getDeclName() override;
    std::string getDeclFileName() override;
    std::string getDefinition() override;

private:
    void updateTitle();
    void updateSourceView();
};

}
