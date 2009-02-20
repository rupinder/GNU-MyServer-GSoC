

class Error(Exception):
    def __init__(self, value):
        self.value = value
    
    def __str__(self):
        return repr(self.value)

class FatalError(Error):
    def __init__(self, value):
        self.value = value
    
    def __str__(self):
        return repr(self.value)
    
class Warning(Exception):
    def __init__(self, value):
        self.value = value
    
    def __str__(self):
        return repr(self.value)
    

