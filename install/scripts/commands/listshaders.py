# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'ListShaders'

def execute():
	shader = GlobalShaderSystem.getShaderForName('bc_rat')
	print(shader.getName())

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
	execute()
