ifeq ($(OS),Windows_NT)
    ifndef VCPKG_ROOT
        VCPKG_FROM_PATH := $(shell where.exe vcpkg)
        ifneq ($(VCPKG_FROM_PATH),)
            VCPKG_ROOT := $(patsubst %/,%,$(dir $(VCPKG_FROM_PATH)))
        endif
    endif
    ifndef VCPKG_ROOT
        $(error "vcpkg not found! Please set VCPKG_ROOT environment variable or add vcpkg to PATH")
    endif
    
    VCPKG_INCLUDE_DIR := $(VCPKG_ROOT)/packages/sdl3_x64-windows/include
    VCPKG_INCLUDE_DIR := $(VCPKG_INCLUDE_DIR) $(VCPKG_ROOT)/packages/sdl3-image_x64-windows/include
    VCPKG_INCLUDE_DIR := $(VCPKG_INCLUDE_DIR) $(VCPKG_ROOT)/packages/sdl3-ttf_x64-windows/include
    VCPKG_INCLUDE_DIR := $(VCPKG_INCLUDE_DIR) $(VCPKG_ROOT)/packages/ffmpeg_x64-windows/include

    VCPKG_LIB_DIR := $(VCPKG_ROOT)/packages/sdl3_x64-windows/lib
    VCPKG_LIB_DIR := $(VCPKG_LIB_DIR) $(VCPKG_ROOT)/packages/sdl3-image_x64-windows/lib
    VCPKG_LIB_DIR := $(VCPKG_LIB_DIR) $(VCPKG_ROOT)/packages/sdl3-ttf_x64-windows/lib
    VCPKG_LIB_DIR := $(VCPKG_LIB_DIR) $(VCPKG_ROOT)/packages/ffmpeg_x64-windows/lib

    EXE_EXT := .exe
    LIB_EXT := .dll

    # Compiler settings
    CC := gcc
    C_FLAGS := -Wall -Wextra -O2 -g3 -I include -I $(VCPKG_ROOT)/packages/sdl3_x64-windows/include -I $(VCPKG_ROOT)/packages/sdl3-image_x64-windows/include -I $(VCPKG_ROOT)/packages/sdl3-ttf_x64-windows/include -I $(VCPKG_ROOT)/packages/ffmpeg_x64-windows/include -fPIC -DBAPI_LOG_ENABLED

    LD_FLAGS := -L $(VCPKG_ROOT)/packages/sdl3_x64-windows/lib -L $(VCPKG_ROOT)/packages/sdl3-image_x64-windows/lib -L $(VCPKG_ROOT)/packages/sdl3-ttf_x64-windows/lib -L $(VCPKG_ROOT)/packages/ffmpeg_x64-windows/lib
    LIBS := -lSDL3 -lSDL3_image -lSDL3_ttf -lavcodec -lavformat -lavutil -lswscale -lswresample -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32
else
    # Linux settings
    # Extension settings
    EXE_EXT :=
    LIB_EXT := .so

    # Compiler settings
    CC := gcc
    C_FLAGS := -Wall -Wextra -O2 -g3 -I include -fPIC -DBAPI_LOG_ENABLED

    LD_FLAGS :=
    LIBS := -lSDL3 -lSDL3_image -lSDL3_ttf -lavcodec -lavformat -lavutil -lswscale -lswresample -lGL -lm
endif

# Source files (new directory layout)
CORE_SOURCES := engine/platform/platform.c engine/master/state.c engine/master/init.c engine/master/version.c engine/render/draw.c
CORE_SOURCES += engine/text/text.c engine/log/log.c engine/mouse_drawing.c
CORE_SOURCES += engine/button/button.c engine/scene/scene.c engine/level/level.c
CORE_SOURCES += engine/xml/xml_loader.c engine/audio/audio.c
CORE_SOURCES += engine/math/math.c engine/texture/texture.c engine/input/input.c engine/camera/camera.c
C_SOURCES := $(CORE_SOURCES) engine/platform/sdl3/platform_sdl3.c engine/video/video.c
LIB_OBJS := $(C_SOURCES:%.c=%.o)
MAIN_OBJS := $(LIB_OBJS) main.o

all: lib main

# Compile rules
%.o: %.c
	@echo "CC $< -> $@"
	@$(CC) $(C_FLAGS) -c -o $@ $<

# Build shared library
lib: $(LIB_OBJS)
	@echo "LIB $^ -> libbridgeengine$(LIB_EXT)"
	@$(CC) $(C_FLAGS) $(LD_FLAGS) -shared -fPIC $^ -o libbridgeengine$(LIB_EXT) $(LIBS)

# Build static library
staticlib: $(LIB_OBJS)
	@ar cr libbridgeengine.a $^



XJ380_SDK := $(CURDIR)/XJ380_XACT_2026v4_xj380/depend

XJ380_XAPI_OBJS := $(wildcard $(XJ380_SDK)/obj-gui/*.o) \
    $(wildcard $(XJ380_SDK)/obj-gui/*/*.o) \
    $(wildcard $(XJ380_SDK)/obj-gui/*/*/*.o) \
    $(XJ380_SDK)/obj-gui/liballoc-x86_64.a
XJ380_REQUIRED_ARCHIVE := $(XJ380_SDK)/obj-gui/liballoc-x86_64.a

XJ380_CC := clang
XJ380_CXX := clang++
XJ380_LD := ld

XJ380_INC_FLAGS := -I include \
    -I $(XJ380_SDK)/include \
    -I $(XJ380_SDK)/include/xposix
XJ380_DEF_FLAGS := -DUSE_BACKEND_XJ380 -DBAPI_LOG_ENABLED -D__XJ380_OS__ -DXJ380_OS \
    -Dcosf=cos -Dsinf=sin -Dsqrtf=sqrt \
    -Dabs=__xj380_abs


XJ380_C_FLAGS := -ffreestanding -fno-builtin -m64 -std=c11 \
    -fno-stack-protector -fshort-wchar -nostdinc \
    -O -g -fPIC $(XJ380_INC_FLAGS) $(XJ380_DEF_FLAGS)


XJ380_CXX_FLAGS := -ffreestanding -fno-builtin -m64 -std=c++11 \
    -fno-stack-protector -fno-exceptions -fshort-wchar -nostdinc \
    -O -g -fPIC $(XJ380_INC_FLAGS) $(XJ380_DEF_FLAGS)

XJ380_C_SOURCES := engine/platform/platform.c engine/master/state.c engine/master/init.c engine/master/version.c engine/render/draw.c
XJ380_C_SOURCES += engine/text/text.c engine/log/log.c engine/mouse_drawing.c
XJ380_C_SOURCES += engine/math/math.c engine/texture/texture.c engine/input/input.c engine/camera/camera.c
XJ380_C_SOURCES += engine/button/button.c engine/scene/scene.c engine/level/level.c
XJ380_C_SOURCES += engine/xml/xml_loader.c engine/audio/audio.c engine/video/video_stub.c
XJ380_CPP_SOURCES := engine/platform/xj380/platform_xj380.cpp
XJ380_C_OBJS := $(XJ380_C_SOURCES:%.c=%.xj380.o)
XJ380_CPP_OBJS := $(XJ380_CPP_SOURCES:%.cpp=%.xj380.o)
XJ380_OBJS := $(XJ380_C_OBJS) $(XJ380_CPP_OBJS)

.PHONY: check_xj380_sdk
check_xj380_sdk:
	@test -d "$(XJ380_SDK)/include" || { echo "XJ380 SDK headers not found: $(XJ380_SDK)/include"; exit 1; }
	@test -d "$(XJ380_SDK)/obj-gui" || { echo "XJ380 SDK GUI objects not found: $(XJ380_SDK)/obj-gui"; exit 1; }
	@test -n "$(wildcard $(XJ380_SDK)/obj-gui/*.o $(XJ380_SDK)/obj-gui/*/*.o $(XJ380_SDK)/obj-gui/*/*/*.o)" || { echo "XJ380 SDK GUI object files are missing under $(XJ380_SDK)/obj-gui"; exit 1; }
	@test -f "$(XJ380_REQUIRED_ARCHIVE)" || { echo "XJ380 SDK archive not found: $(XJ380_REQUIRED_ARCHIVE)"; exit 1; }

$(XJ380_OBJS): | check_xj380_sdk

%.xj380.o: %.c
	@echo "XJ380_CC $< -> $@"
	@$(XJ380_CC) $(XJ380_C_FLAGS) -c -o $@ $<

%.xj380.o: %.cpp
	@echo "XJ380_CXX $< -> $@"
	@$(XJ380_CXX) $(XJ380_CXX_FLAGS) -c -o $@ $<

# XJ380 static library
xj380_staticlib: $(XJ380_OBJS) | check_xj380_sdk
	@rm -f libbridgeengine_xj380.a
	@echo "AR libbridgeengine_xj380.a"
	@ar cr libbridgeengine_xj380.a $(XJ380_OBJS)


xj380_epf: xj380_staticlib xj380_main.xj380.o
	@echo "XJ380_LD bridgeengine_demo.epf"
	@$(XJ380_LD) -Ttext=0x200000 $(XJ380_XAPI_OBJS) xj380_main.xj380.o libbridgeengine_xj380.a -o bridgeengine_demo.epf

main: $(MAIN_OBJS)
	@echo "LINK $^ -> main$(EXE_EXT)"
	@$(CC) $(C_FLAGS) $(LD_FLAGS) $^ -o main$(EXE_EXT) $(LIBS)

# Build text example
text_example: text_example.o $(LIB_OBJS)
	@echo "LINK $^ -> text_example$(EXE_EXT)"
	@$(CC) $(C_FLAGS) $(LD_FLAGS) $^ -o text_example$(EXE_EXT) $(LIBS)

# Formatting
%.fmt: %
	@echo fmt $< ...
	@clang-format -i $<

# Tidy
%.tidy: %
	@echo tidy $< ...
	@clang-tidy $< -- $(C_FLAGS)

.PHONY: format check clean

format: $(C_SOURCES:%=%.fmt) $(HEADERS:%=%.fmt)
	@echo fmt done

check: $(C_SOURCES:%=%.tidy) $(S_SOURCES:%=%.tidy) $(HEADERS:%=%.tidy)
	@echo check done

# Clean
clean:
	@echo Removing $(LIB_OBJS) $(XJ380_OBJS) main.o main$(EXE_EXT) libbridgeengine$(LIB_EXT) libbridgeengine.a libbridgeengine_xj380.a text_example$(EXE_EXT)
ifeq ($(OS),Windows_NT)
	@del /f /q $(subst /,\,$(LIB_OBJS)) main.o main.exe libbridgeengine.so libbridgeengine.dll libbridgeengine.a text_example.exe libbridgeengine_xj380.a bridgeengine_demo.epf $(subst /,\,$(XJ380_OBJS)) xj380_main.xj380.o 2>nul
else
	@rm -f $(LIB_OBJS) $(XJ380_C_OBJS) $(XJ380_CPP_OBJS)
	@rm -f main.o main.xj380.o xj380_main.o main$(EXE_EXT) text_example$(EXE_EXT)
	@rm -f libbridgeengine$(LIB_EXT) libbridgeengine.a libbridgeengine_xj380.a bridgeengine_demo.epf
	@rm -f *.xj380.o engine/*.o
endif

# Install
install: lib
	@mkdir -p $(DESTDIR)/usr/local/lib
	@mkdir -p $(DESTDIR)/usr/local/include
	@mkdir -p $(DESTDIR)/usr/local/lib/pkgconfig
	@cp libbridgeengine.so libbridgeengine.a $(DESTDIR)/usr/local/lib/
	@cp -r include/* $(DESTDIR)/usr/local/include/
	@cp bridgeengine.pc $(DESTDIR)/usr/local/lib/pkgconfig/
	@echo BridgeEngine installed successfully!
