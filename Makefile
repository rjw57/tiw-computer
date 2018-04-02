all: emulator os
.PHONY: all

emulator:
	$(MAKE) -C emulator REGENIE=1 SOURCES=src/mame/drivers/tiw.cpp
.PHONY: emulator

os:
	$(MAKE) -C os
.PHONY: os
