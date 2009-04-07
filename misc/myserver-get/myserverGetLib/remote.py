# MyServer
# Copyright (C) 2009 The MyServer Team
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import string

class Repository:
    def __init__(self,url):
        self.url = url
        

    def synchronizeListWithRepository(self, list):
        pass
    
    def getPluginBinary(self,list,plugin):
        pass
    
    def getPluginSource(self,list,source,dir):
        pass
    
class RepositoryManager:
    def __init__(self):
        self.__reps = {}
    
    def addSupportedRepository(self, protocol, repository):
        self.__reps[protocol] = repository
    
    def getRepository(self, url):
        url = string.lower(url)
        if string.find(url,"svn")==0:
            return self.__reps["svn"]
        elif string.find(url,"http")==0:
            return self.__reps["http"]
        elif string.find(url,"ftp")==0:
            return self.__reps["ftp"]
        return None
            
if __name__ == "__main__":
    list = local.ListManager("localhost.it",".")
    rep = Repository("svn://svn.savannah.gnu.org/myserver/trunk/plugins")
    rep.synchronizeListWithRepository(list)