--------------------------------------------------------------------------------------------------------------------------
----------------------------------------------------MYSERVER--------------------------------------------------------------
--------------------------------------------------------------------------------------------------------------------------


----------------------------------------------------BRIEF NOTES--------------------------------------------------------------
The primary goal of this software is to create a free and simple to manage web server. MyServer is distribuited under the terms of the GNU Library General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.


----------------------------------------------------INSTALLATION NOTES-------------------------------------------------------
This software is developed to run under all versions of windows and linux.
This software is supported on the windows platform in two ways: the first like a normal application, the second like a service. Running like a service is supported onto Windows 2000 and XP and give options security and the application can be managed from the service manager; running like a normal application the Administrator hadn't any security and all the contents in the web directory are visible to everyone. For run MyServer like a service we need before to register it, for register it we can run the batch file "REGISTER SERVICE.bat" that register it in the service pool manager. At this point we can start it or from the service control manager or running the batch file "START SERVICE.bat". For stop the service execution you need to run the "STOP SERVICE.bat" file. For easily manage the service you can start the control.exe application that allows with a simple GUI to start and stop MyServer.For run MyServer like a normal application you can start the "START CONSOLE.bat", for stop MyServer you must press Ctrl+C in the console window.
The program control.exe is developed to help you in MyServer management.
The files contained in the web directory must be overwritten with your files. They are present only for a demostration of MyServer functionality.
The linux binaries are myserver and control are dynamicaly linked with glibc6 and wxWindows 2.4. If you cannot get the dinamic ones to run even with wxWindows installed, download the souce and run make in the root dir of the uncompressed source. The myserver program only has console mode and that is the default running mode.  The control program can also start myserver in console mode by invokeing xterm.  To run them, open a terminal, cd into the drectory that myserver and control are located and type ./myserver for the server itself or ./control for the contol program that also can run the myserver program. The MIMEtypes.txt.linux file is a preconfiged mime types file for linux and is to be copied over top of the MIMEtypes.txt file.  In this file are the configured location of programs such as php that are used with the CGI function.  Please change them acordingly with control if they differ from your system.

----------------------------------------------------CURRENT STATUS----------------------------------------------------------- MyServer isn't at a mature status too if you are interested to complete anything in the TODO list please contact the
project administrator on SourceForge.
The linux portion is not suitable for running as root due to posable security flaws in the program.  To run the server as a user, you must change the port to something like 8080 in the myserver.xml file because users can only use port numbers larger than 1024. 

-----------------------------------------------ADDITIONAL INFO---------------------------------------------------------------                                                     
For additional info about this project see the site at:  	
http://myserverweb.sourceforge.net


--------------------------------------------------HELP-----------------------------------------------------------------------
If you report problems running MyServer, post your messages on the forum at:
http://myserverweb.sourceforge.net/forum







We hope that this software could be useful for you.
