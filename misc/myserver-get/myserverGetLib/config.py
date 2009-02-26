
CNFG_FILE = "myserver-get.xml"

REP_LIST_FILE = "repositories.xml"

PLUGIN_LIST_FILES_DIR = "list"

PLUGIN_LIST_PREFIX = "myserver-get-"

PLUGIN_LIST_SUFFIX = "-plugin-list.xml"

MYSERVER_PLUGIN_DIR = "../../myserver/binaries/plugins"

MYSERVER_VERSION = "0.9"

verbose = False

arch = ""

class ConfigLoader:
    def __init__(self, configFile):
        self.__configFile = configFile