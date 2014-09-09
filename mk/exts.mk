.phony: exts clean_exts

exts:
	for i in $(EXTS); do rm -rf $(EXTSRCDIR)/$$i; done
	for i in $(EXTS); do (cd $(EXTSRCDIR); $(TAR) xzf $$i.tar.gz; $(PATCH) -p0 < $$i.patch;); done

clean_exts:
	for i in $(EXTS); do rm -rf $(EXTSRCDIR)/$$i; done

ALL_PREDEP+= exts
CLEAN_TARGET+= clean_exts
