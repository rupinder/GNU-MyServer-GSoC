
CNFG_FILE = "myserver-get.xml"

REP_LIST_FILE = "repositories.xml"

PLUGIN_LIST_FILES_DIR = "list"

PLUGIN_LIST_PREFIX = "myserver-get-"

PLUGIN_LIST_SUFFIX = "-plugin-list.xml"

VERBOSE = False

class ConfigLoader:
    def __init__(self, configFile):
        self.__configFile = configFile