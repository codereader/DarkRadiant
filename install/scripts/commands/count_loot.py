__commandName__ = 'CountLoot'
__commandDisplayName__ = 'Count Loot'

def execute():
	import darkradiant as dr

	class SceneLootCounter(dr.SceneNodeVisitor):
		loot_sum = 0
		def pre(self, node):
			entity = node.getEntity()
			if not entity.isNull():
				try:
					self.loot_sum += int(entity.getKeyValue("inv_loot_value"))
				except:
					pass
			return 1

	class SelectionLootCounter(dr.SelectionVisitor):
		loot_sum = 0
		def visit(self, node):
			entity = node.getEntity()
			if not entity.isNull():
				try:
					self.loot_sum += int(entity.getKeyValue("inv_loot_value"))
				except:
					pass

	scene_counter = SceneLootCounter()
	GlobalSceneGraph.root().traverse(scene_counter)
	
	selection_counter = SelectionLootCounter()
	GlobalSelectionSystem.foreachSelected(selection_counter)

	result = "Total loot: " + str(scene_counter.loot_sum)
	if selection_counter.loot_sum > 0:
		result += "\nSelection: " + str(selection_counter.loot_sum)
	
	GlobalDialogManager.createMessageBox("Loot Count Results", result, dr.Dialog.CONFIRM).run()

if __executeCommand__:
	execute()
