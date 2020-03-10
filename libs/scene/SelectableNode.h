#pragma once

#include "scene/Node.h"
#include "iselectiongroup.h"
#include "iundo.h"

namespace scene
{

/**
 * \brief
 * Subclass of scene::Node which implements the Selectable interface.
 *
 * The GlobalSelectionSystem will be notified of selection changes.
 */
class SelectableNode :
	public scene::Node,
	public IGroupSelectable,
	public IUndoable
{
private:
	// Current selection state
	bool _selected;

	// The groups this node is a member of. The last value in the list 
	// represents the group this node has been added to most recently
	GroupIds _groups;

	IUndoStateSaver* _undoStateSaver;

public:
	SelectableNode();

	SelectableNode(const SelectableNode& other);

	virtual ~SelectableNode();

	virtual void onInsertIntoScene(IMapRootNode& root) override;

    // override scene::Inode::onRemoveFromScene to de-select self
	virtual void onRemoveFromScene(IMapRootNode& root) override;

	/**
	* \brief
	* Set the selection state.
	*/
	virtual void setSelected(bool select) override;

	virtual void addToGroup(std::size_t groupId) override;
	virtual void removeFromGroup(std::size_t groupId) override;

	virtual bool isGroupMember() override;
	virtual std::size_t getMostRecentGroupId() override;
	virtual const GroupIds& getGroupIds() const override;
	virtual void setSelected(bool select, bool changeGroupStatus) override;
	virtual bool isSelected() const override;

	IUndoMementoPtr exportState() const override;
	void importState(const IUndoMementoPtr& state) override;

protected:
	/**
     * \brief
     * Invoked when the selection status changes.
     */
	virtual void onSelectionStatusChange(bool changeGroupStatus);

	virtual void connectUndoSystem(IMapFileChangeTracker& changeTracker) override;
	virtual void disconnectUndoSystem(IMapFileChangeTracker& changeTracker) override;

private:
	void undoSave();
};

} // namespace
