APP := slime_md_viewer
SRC := src/main.cpp
BUILD_DIR := build
OUT := $(BUILD_DIR)/$(APP)

QT_PKG := $(shell pkg-config --exists Qt6Widgets Qt6Network && echo "Qt6Widgets Qt6Network" || echo "Qt5Widgets Qt5Network")
CXX := g++
CXXFLAGS := -std=c++17 -fPIC -Wall -Wextra $(shell pkg-config --cflags $(QT_PKG))
LDLIBS := $(shell pkg-config --libs $(QT_PKG))

.PHONY: all clean install uninstall package

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -rf $(BUILD_DIR)

install: $(OUT)
	./install.sh

uninstall:
	./uninstall.sh

package: $(OUT)
	./package-release.sh
