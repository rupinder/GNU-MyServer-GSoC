call %VSCOMNTOOLS%\vsvars32.bat 
cl.exe /Zd /O1 /c math_sum.cpp 
link.exe -dll /release /nodefaultlib:libcd /nologo /nod:libcpmt.lib kernel32.lib user32.lib mscoree.lib /DEF:definitions.def /out:math_sum.mscgi math_sum.obj
move *.mscgi ..\..\cgi-bin\
del *.lib
del *.obj
del *.exp

pause