# Some interface tests

# Test brush manipulation
class BrushManipulator(SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a brush from this node
		brush = node.getBrush()

		if not brush.isNull():
			print('Brush information:')
			print('Number of faces: ' + str(brush.getNumFaces()))

			i = 0
			while i < brush.getNumFaces():
				face = brush.getFace(i)
				print('Face #' + str(i) +' has shader ' + face.getShader())
				winding = face.getWinding()

				for w in winding:
					print('w.vertex=' + str(w.vertex.x()) + ',' + str(w.vertex.y()) + ',' + str(w.vertex.z()))

				face.fitTexture(1,1)

				i += 1

		return 1

walker = BrushManipulator()
GlobalSceneGraph.root().traverse(walker)

print('')

