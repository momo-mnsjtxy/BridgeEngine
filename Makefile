ifeq ($(OS),Windows_NT)
    ifndef VCPKG_ROOT
        VCPKG_FROM_PATH := $(shell where.exe vcpkg)
        ifneq ($(VCPKG_FROM_PATH),)
            VCPKG_ROOT := $(shell ./x_dirname.bat "$(VCPKG_FROM_PATH)")
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

# Source files
C_SOURCES := engine/platform/platform.c engine/platform/sdl3/platform_sdl3.c engine/master/init.c engine/render/create.c engine/render/draw.c engine/mouse_drawing.c engine/text.c engine/version.c engine/log.c engine/button/button.c engine/scene/scene.c engine/level/level.c engine/xml/xml_loader.c engine/audio/audio.c engine/video/video.c
LIB_OBJS := $(C_SOURCES:%.c=%.o)
MAIN_OBJS := $(LIB_OBJS) main.o

all: lib main

# Compile rules
%.o: %.c
	@echo CC $< -> $@
	@$(CC) $(C_FLAGS) -c -o $@ $<

# Build shared library
lib: $(LIB_OBJS)
	@echo LIB $^ -> libbridgeengine$(LIB_EXT)
	@$(CC) $(C_FLAGS) $(LD_FLAGS) -shared -fPIC $^ -o libbridgeengine$(LIB_EXT) $(LIBS)

# Build static library
staticlib: engine/platform/platform.o engine/platform/sdl3/platform_sdl3.o engine/master/init.o engine/render/create.o engine/render/draw.o engine/mouse_drawing.o engine/text.o engine/version.o engine/log.o engine/button/button.o engine/scene/scene.o engine/level/level.o engine/xml/xml_loader.o engine/audio/audio.o engine/video/video.o
	@ar cr libbridgeengine.a $^



XJ380_SDK := $(CURDIR)/XJ380_XACT_2026v4_xj380/depend

XJ380_XAPI_OBJS := $(wildcard $(XJ380_SDK)/obj-gui/*.o) \
    $(wildcard $(XJ380_SDK)/obj-gui/*/*.o) \
    $(wildcard $(XJ380_SDK)/obj-gui/*/*/*.o) \
    $(XJ380_SDK)/obj-gui/liballoc-x86_64.a

XJ380_CC := clang
XJ380_CXX := clang++
XJ380_LD := ld

XJ380_INC_FLAGS := -I include \
    -I $(XJ380_SDK)/include \
    -I $(XJ380_SDK)/include/xposix
XJ380_DEF_FLAGS := -DUSE_BACKEND_XJ380 -DBAPI_LOG_ENABLED -D__XJ380_OS__ -DXJ380_OS \
    -DM_PI=3.14159265358979323846 -Dcosf=cos -Dsinf=sin -Dsqrtf=sqrt \
    -Dabs=__xj380_abs


XJ380_C_FLAGS := -ffreestanding -fno-builtin -m64 -std=c11 \
    -fno-stack-protector -fshort-wchar -nostdinc \
    -O -g -fPIC $(XJ380_INC_FLAGS) $(XJ380_DEF_FLAGS)


XJ380_CXX_FLAGS := -ffreestanding -fno-builtin -m64 -std=c++11 \
    -fno-stack-protector -fno-exceptions -fshort-wchar -nostdinc \
    -O -g -fPIC $(XJ380_INC_FLAGS) $(XJ380_DEF_FLAGS)

XJ380_C_SOURCES := engine/platform/platform.c engine/master/init.c engine/render/create.c engine/render/draw.c engine/mouse_drawing.c engine/text.c engine/version.c engine/log.c engine/button/button.c engine/scene/scene.c engine/level/level.c engine/xml/xml_loader.c engine/audio/audio.c engine/video/video_stub.c
XJ380_CPP_SOURCES := engine/platform/xj380/platform_xj380.cpp
XJ380_C_OBJS := $(XJ380_C_SOURCES:%.c=%.xj380.o)
XJ380_CPP_OBJS := $(XJ380_CPP_SOURCES:%.cpp=%.xj380.o)
XJ380_OBJS := $(XJ380_C_OBJS) $(XJ380_CPP_OBJS)


%.xj380.o: %.c
	@echo XJ380_CC $< -> $@
	@$(XJ380_CC) $(XJ380_C_FLAGS) -c -o $@ $<

%.xj380.o: %.cpp
	@echo XJ380_CXX $< -> $@
	@$(XJ380_CXX) $(XJ380_CXX_FLAGS) -c -o $@ $<

# XJ380 static library
xj380_staticlib: $(XJ380_OBJS)
	@rm -f libbridgeengine_xj380.a
	@echo "AR libbridgeengine_xj380.a"
	@ar cr libbridgeengine_xj380.a $^


xj380_epf: xj380_staticlib xj380_main.xj380.o
	@echo "XJ380_LD bridgeengine_demo.epf"
	@$(XJ380_LD) -Ttext=0x200000 $(XJ380_XAPI_OBJS) xj380_main.xj380.o libbridgeengine_xj380.a -o bridgeengine_demo.epf

main: $(MAIN_OBJS)
	@echo LINK $^ -> main$(EXE_EXT)
	@$(CC) $(C_FLAGS) $(LD_FLAGS) $^ -o main$(EXE_EXT) $(LIBS)

# Build text example
text_example: text_example.o engine/master/init.o engine/render/create.o engine/text.o
	@echo LINK $^ -> text_example$(EXE_EXT)
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
	@echo Removing $(LIB_OBJS) main.o main$(EXE_EXT) libbridgeengine$(LIB_EXT) libbridgeengine.a text_example$(EXE_EXT)
ifeq ($(OS),Windows_NT)
	@del /f /q engine\platform\platform.o engine\platform\sdl3\platform_sdl3.o engine\master\init.o engine\render\create.o engine\render\draw.o engine\mouse_drawing.o engine\text.o engine\version.o main.o main.exe libbridgeengine.so libbridgeengine.dll libbridgeengine.a text_example.exe engine\scene\scene.o engine\level\level.o engine\xml\xml_loader.o engine\log.o engine\button\button.o engine\audio\audio.o engine\video\video.o 2>nul
else
	@rm -f engine/platform/platform.o engine/platform/sdl3/platform_sdl3.o engine/master/init.o engine/render/create.o engine/render/draw.o engine/mouse_drawing.o engine/text.o engine/version.o main.o main libbridgeengine.so libbridgeengine.dll libbridgeengine.a text_example engine/scene/scene.o engine/level/level.o engine/xml/xml_loader.o engine/log.o engine/button/button.o engine/audio/audio.o engine/video/video.o engine/video/video_stub.o engine/platform/xj380/platform_xj380.o xj380_main.o libbridgeengine_xj380.a bridgeengine_demo.epf *.xj380.o
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