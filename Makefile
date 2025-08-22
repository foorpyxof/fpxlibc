.PHONY: all prep release debug libs test clean shaders

all: libs

# NOTE: remember to `make clean` after switching target OS
# WINDOWS := true

CC != which clang 2>/dev/null
CCPLUS != which clang++ 2>/dev/null
AR != which ar

CC_WIN32 := x86_64-w64-mingw32-gcc
CCPLUS_WIN32 := x86_64-w64-mingw32-g++
AR_WIN32 := x86_64-w64-mingw32-ar

include make/early.mak

WINDOWS_TARGET_NAME := win64
LINUX_TARGET_NAME := linux

LIB_PREFIX := libfpx_
DEBUG_SUFFIX := _debug

ifeq ($(WINDOWS),true)
	TARGET := $(WINDOWS_TARGET_NAME)
else
	TARGET := $(LINUX_TARGET_NAME)
endif

include make/*.mk

ifeq ($(TARGET),$(WINDOWS_TARGET_NAME))

	-include make/windows/*.mk

	CC := $(CC_WIN32)
	CCPLUS := $(CCPLUS_WIN32)
	AR := $(AR_WIN32)
	CFLAGS += -mwindows
	CPPFLAGS += -mwindows

	# mingw/bin/libwinpthread.dll.a import library
	LDFLAGS += -lwinpthread.dll
	
	EXE_EXT := .exe
	OBJ_EXT := .obj
	LIB_EXT := .lib

else

ifeq ($(CC),)
	CC != which cc
endif
ifeq ($(CCPLUS),)
	CC != which g++
endif

	LDFLAGS += -lglfw
	
	# EXE_EXT := .out
	OBJ_EXT := .o
	LIB_EXT := .a

endif

EXE_EXT := $(TARGET)$(EXE_EXT)

include make/variables.mak
include make/dll.mak

LIBRARY_NAMES := alloc mem c-utils cpp-utils math networking/http networking/quic networking/tcp string structures

OBJECTS_FOLDER := $(BUILD_FOLDER)/objects
LIBRARY_FOLDER := $(BUILD_FOLDER)/lib

COMPONENTS_C := $(foreach lib,$(LIBRARY_NAMES),$(patsubst $(SOURCE_FOLDER)/%.c,%,$(wildcard $(SOURCE_FOLDER)/$(lib)/*.c)))
COMPONENTS_CPP := $(foreach lib,$(LIBRARY_NAMES),$(patsubst $(SOURCE_FOLDER)/%.cpp,%,$(wildcard $(SOURCE_FOLDER)/$(lib)/*.cpp)))

OBJECTS_RELEASE_C := $(foreach c,$(COMPONENTS_C),$(OBJECTS_FOLDER)/$c$(OBJ_EXT))
OBJECTS_DEBUG_C := $(patsubst %$(OBJ_EXT),%$(DEBUG_SUFFIX)$(OBJ_EXT),$(OBJECTS_RELEASE_C))

OBJECTS_RELEASE_CPP := $(foreach c,$(COMPONENTS_CPP),$(OBJECTS_FOLDER)/$c$(OBJ_EXT))
OBJECTS_DEBUG_CPP := $(patsubst %$(OBJ_EXT),%$(DEBUG_SUFFIX)$(OBJ_EXT),$(OBJECTS_RELEASE_CPP))

TEST_CODE := $(addsuffix .cpp,$(addprefix $(TEST_DIR)/,$(LIBRARY_NAMES)))

include make/library.mak

clean:
	rm -rf ./$(BUILD_FOLDER) || true

release: $(LIBS_RELEASE)
debug: $(LIBS_DEBUG)

# individual libraries, both RELEASE and DEBUG
libs: $(LIBS_RELEASE) $(LIBS_DEBUG)

$(OBJECTS_FOLDER):
	mkdir -p $@

MKDIR_COMMAND = if ! [ -d "$(dir $@)" ]; then mkdir -p $(dir $@); fi

$(OBJECTS_RELEASE_C): $(OBJECTS_FOLDER)/%$(OBJ_EXT): $(SOURCE_FOLDER)/%.c
	$(MKDIR_COMMAND)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) -c $< -o $@

$(OBJECTS_DEBUG_C): $(OBJECTS_FOLDER)/%$(DEBUG_SUFFIX)$(OBJ_EXT): $(SOURCE_FOLDER)/%.c
	$(MKDIR_COMMAND)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

$(OBJECTS_RELEASE_CPP): $(OBJECTS_FOLDER)/%$(OBJ_EXT): $(SOURCE_FOLDER)/%.cpp
	$(MKDIR_COMMAND)
	$(CCPLUS) $(CPPFLAGS) $(RELEASE_FLAGS) -c $< -o $@

$(OBJECTS_DEBUG_CPP): $(OBJECTS_FOLDER)/%$(DEBUG_SUFFIX)$(OBJ_EXT): $(SOURCE_FOLDER)/%.cpp
	$(MKDIR_COMMAND)
	$(CCPLUS) $(CPPFLAGS) $(DEBUG_FLAGS) -c $< -o $@

include make/zip.mak
