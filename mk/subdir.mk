.PHONY : subdirs clean $(SUBDIRS)
ALL_TARGET+= subdirs
CLEAN_TARGET+= clean_subdirs
INSTALL_TARGET+= install_subdirs

all: subdirs 

subdirs:
	for dir in $(SUBDIRS); do (cd $$dir; $(MAKE) all) ; done

clean_subdirs:
	for dir in $(SUBDIRS); do (cd $$dir; $(MAKE) clean) ; done

install_subdirs: subdirs
	for dir in $(SUBDIRS); do (cd $$dir; $(MAKE) install); done
