# Set the command name so that DarkRadiant recognises this file
__commandName__ = 'ShiftTexturesUpwardsRandomly'
__commandDisplayName__ = 'Shift Textures randomly upwards'

# The actual algorithm called by DarkRadiant
# is contained in the execute() function

def execute():
	import random
	s = random.randint(0, 256)

	for i in range(0, s+1):
		GlobalCommandSystem.execute('texshiftup "' + str(s) + '"')
	else:
		print("texture translated over " + str(s) )

# The variable __executeCommand__ evaluates to true
# when DarkRadiant executes this command
if __executeCommand__:
	execute()
