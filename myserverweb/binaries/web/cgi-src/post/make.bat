call %VSCOMNTOOLS%\vsvars32.bat 
cl.exe /Zd /O1 /c post.cpp 
link.exe -dll /release /nodefaultlib:libcd /nologo /nod:libcpmt.lib kernel32.lib user32.lib mscoree.lib /DEF:definitions.def /out:post.mscgi post.obj
move *.mscgi ..\..\cgi-bin\
del *.obj
del *.exp
del *.lib
pause