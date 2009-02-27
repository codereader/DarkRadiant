# Some interface tests

# Test the Registry interface
value = GlobalRegistry.get('user/paths/appPath')
print(value)

worldspawn = Radiant.findEntityByClassname("worldspawn")
worldspawn.setKeyValue('test', 'success')
print('Worldspawn edited')

# Test the EClassManager interface
eclass = GlobalEntityClassManager.findClass('atdm:func_shooter')
print(eclass.getAttribute('editor_usage').value)

# Try creating a func_shooter entity
if not eclass.isNull():
	shooter = GlobalEntityCreator.createEntity(eclass)

modelDef = GlobalEntityClassManager.findModel('builderforger')
print('ModelDef mesh for builderforger = ' + modelDef.mesh)

# Test iterating over C++ std::map
#for anim in modelDef.anims:
#	print(anim.key())
#	print(' = ')
#	print(anim.data())
#	print('')

# Test implementing a eclass visitor interface
#class TestVisitor(EntityClassVisitor) :
#	def visit(self, eclass):
#		print eclass.getAttribute('editor_usage').value

#eclassVisitor = TestVisitor()
#GlobalEntityClassManager.forEach(eclassVisitor)

# Test traversing the scenegraph
#class SceneWalker(SceneNodeVisitor) :
#	def pre(self, node):
#		print(node.getNodeType())
#		return 1

#walker = SceneWalker()
#GlobalSceneGraph.root().traverse(walker)

# Test traversing the current selection
class Walker(SelectionVisitor) :
	def visit(self, node):
		# Try to "cast" the node to a brush
		brush = node.getBrush()

		# If the Brush is not NULL the cast succeeded
		if not brush.isNull():
			print(brush.getNumFaces())
		else:
			print('Node is not a brush')

		# Try to cast the node to a patch
		patch = node.getPatch()

		# If the Patch is not NULL the cast succeeded
		if not patch.isNull():
			print('Node is a patch')
		else:
			print('Node is not a patch')


visitor = Walker()
GlobalSelectionSystem.foreachSelected(visitor)

# Try to find the map's worldspawn
worldspawn = GlobalMap.getWorldSpawn()

if not worldspawn.isNull():
	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()
	if not worldspawnent.isNull():
		print('Spawnclass of worldspawn: ' + worldspawnent.getKeyValue('spawnclass'))
else:
	print('There is no worldspawn in this map yet')

# Test the entity visitor interface
class TestEntityVisitor(EntityVisitor) :
	def visit(self, key, value):
		print('Worldspawn has spawnarg: ' + key + ' = ' + value)

if not worldspawn.isNull():
	tev = TestEntityVisitor()

	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()

	worldspawnent.forEachKeyValue(tev)

# Test the commandsystem
GlobalCommandSystem.execute('texscale "0 0.1"')

# Test the GameManager interface
print('Mod path = ' + GlobalGameManager.getModPath())

game = GlobalGameManager.currentGame()
print('Current game type: ' + game.getKeyValue('type'))

print('VFS Search paths:')
vfsPaths = GlobalGameManager.getVFSSearchPaths()

for path in vfsPaths:
	print(path)

print('')
