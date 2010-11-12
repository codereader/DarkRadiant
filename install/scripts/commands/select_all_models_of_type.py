# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'SelectAllModelsOfType'
__commandDisplayName__ = 'Select all Models of same type'

# The actual algorithm called by DarkRadiant is contained in the execute() function
def execute():
	# Collect all currently selected models
	selectedModelNames = {}

	class Walker(SelectionVisitor) :
		def visit(self, node):
			# Try to "cast" the node to an entity
			entity = node.getEntity()

			if not entity.isNull():
				if not entity.getKeyValue('model') == '':
					selectedModelNames[entity.getKeyValue('model')] = 1

	visitor = Walker()
	GlobalSelectionSystem.foreachSelected(visitor)

	print('Unique models currently selected: ' + str(len(selectedModelNames)))

	# Now traverse the scenegraph, selecting all of the same names
	class SceneWalker(SceneNodeVisitor) :
		def pre(self, node):

			# Try to "cast" the node to an entity
			entity = node.getEntity()

			if not entity.isNull():
				modelName = entity.getKeyValue('model')

				if not modelName == '' and modelName in selectedModelNames:
					# match, select this node
					node.setSelected(1);

			return 0 # don't traverse this entity's children

	walker = SceneWalker()
	GlobalSceneGraph.root().traverse(walker)

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
	execute()
