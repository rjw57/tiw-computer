all: emulator os
.PHONY: all

emulator:
	$(MAKE) -C emulator SOURCES=src/mame/drivers/tiw.cpp
.PHONY: emulator

os:
	$(MAKE) -C os
.PHONY: os
