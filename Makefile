INSTALLPATH=$(HOME)/bin

install:
	mkdir -p $(INSTALLPATH)
	install -m 0755 zknock $(INSTALLPATH)
