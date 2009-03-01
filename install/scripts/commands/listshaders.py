#The above line indicates that this is an executable DarkRadiant command

commandName = 'ListShaders'

# initCommand evaluates to true during DarkRadiant startup only
if initCommand:
	print('Command initialised: ' + commandName)
else:
	print('Command execution')
