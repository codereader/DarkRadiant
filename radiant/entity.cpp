/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "entity.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "ientity.h"
#include "iregistry.h"
#include "ieclass.h"
#include "iselection.h"
#include "imodel.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "editable.h"

#include "eclass.h"
#include "scenelib.h"
#include "os/path.h"
#include "os/file.h"

#include "gtkutil/FileChooser.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "selectionlib.h"
#include "map/Map.h"

#include "xyview/GlobalXYWnd.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/Curves.h"
#include "ui/modelselector/ModelSelector.h"
#include <boost/format.hpp>

#include <iostream>

class RefreshSkinWalker :
    public scene::NodeVisitor
{
public:
    bool pre(const scene::INodePtr& node) {
        // Check if we have a skinnable model
        SkinnedModelPtr skinned = boost::dynamic_pointer_cast<SkinnedModel>(node);

        if (skinned != NULL) {
            // Let the skinned model reload its current skin.
            skinned->skinChanged(skinned->getSkin());
        }

        return true; // traverse children
    }
};

void ReloadSkins(const cmd::ArgumentList& args) {
    GlobalModelSkinCache().refresh();
    RefreshSkinWalker walker;
    Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

    // Refresh the ModelSelector too
    ui::ModelSelector::refresh();
}

// This takes care of relading the entityDefs and refreshing the scenegraph
void ReloadDefs(const cmd::ArgumentList& args)
{
    // Disable screen updates for the scope of this function
    ui::ScreenUpdateBlocker blocker(_("Processing..."), _("Reloading Defs"));

    GlobalEntityClassManager().reloadDefs();
}

namespace entity
{

void registerCommands()
{
    GlobalCommandSystem().addCommand("ReloadSkins", ReloadSkins);
	GlobalCommandSystem().addCommand("ReloadDefs", ReloadDefs);

    GlobalEventManager().addCommand("ReloadSkins", "ReloadSkins");
	GlobalEventManager().addCommand("ReloadDefs", "ReloadDefs");
}

} // namespace entity
