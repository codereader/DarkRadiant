import darkradiant as dr
from internal import CommandUtil

# Python commands would be classes with static members
class ResizeSelectedBrushesToBounds:
    # We can use type hints to make it possible to register its Signature in the CommandSystem
    @staticmethod
    def execute(min: dr.Vector3, max: dr.Vector3, material: str):
        print(min)
        print(max)
        print(material)

    @staticmethod
    def canExecute():
        return true

print('We have the module: ' + __name__)

# Proves that it's possible to access previously registered DR interfaces
print(dr.ScriptingSystemInterface)
print(dr.ScriptingSystem)

CommandUtil.registerModule(__name__)