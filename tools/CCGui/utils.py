# -*- python -*-

from signalslot import Signal

# Singleton implementation from:
# http://stackoverflow.com/questions/31875/is-there-a-simple-elegant-way-to-define-singletons-in-python
# Thanks!

def singleton(cls):
    obj = cls()

    # always return same object
    cls.__new__ = staticmethod(lambda cls: obj)

    # remove constructor
    try:
        del cls.__init__
    except AttributeError:
        pass

    return cls
