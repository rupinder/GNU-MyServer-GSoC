install:
if BUILD_GUI
	python setup.py install --prefix=${prefix}
	mkdir -p ${bindir}
	cp MyServerControl.py PyGTKControl.glade ${bindir}
endif
