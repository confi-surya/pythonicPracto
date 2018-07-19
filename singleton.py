class SingletonType(type):
    """ Class to create singleton classes object """
    __instance = None
    def __call__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(SingletonType, cls).__call__(*args, **kwargs)
        return cls.__instance


s1=SingletonType(1)
print id(s1)
s2=SingletonType(2)
print id(s2)
