exts:
	for i in $(EXTS); do rm -rf $(EXTSRCDIR)/$$i; done
	for i in $(EXTS); do (cd $(EXTSRCDIR); $(TAR) xvzf $$i.tar.gz; $(PATCH) -p0 < $$i.patch;); done

ALL_TARGET+= exts
