.PHONY: all clean

TOP_DIR = $(shell pwd)

default:all

all:
	@gn gen out && cd out && ninja -v

clean:
	@echo "clean..."
	@cd out && ninja -t clean