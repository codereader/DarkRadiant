# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'Example' # should not contain spaces
__commandDisplayName__ = 'Nice display name for the menus' # should not contain spaces

# The actual algorithm called by DarkRadiant is contained in the execute() function
def execute():
	shader = GlobalShaderSystem.getShaderForName('bc_rat')
	print(shader.getName())

# __executeCommand__ evaluates to true after DarkRadiant has successfully initialised
if __executeCommand__:
	execute()
