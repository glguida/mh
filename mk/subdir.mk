.PHONY : subdirs clean $(SUBDIRS)
ALL_TARGET+= subdirs
CLEAN_TARGET+= clean_subdirs
INSTALL_TARGET+= install_subdirs

all: subdirs 

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ all

clean_subdirs:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean ; done

install_subdirs: subdirs
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir install; done
