class Macros:
	HASH_PATH_SUFFIX = ''
	HASH_PATH_PREFIX = ''

	@classmethod
	def setValue(cls, macro, val):
		if macro.upper() not in cls.__dict__:
			raise KeyError("%s is not defined" %macro.upper())
		setattr(cls, macro.upper(), val)

	@classmethod
	def getValue(cls, macro):
		if macro.upper() not in cls.__dict__:
			raise KeyError("%s is not defined" %macro.upper())
		return getattr(cls, macro.upper())

	@classmethod
	def setNewMacro(cls, macro, val):
		setattr(cls, macro.upper(), val)
