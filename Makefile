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

PREFIX := fpx_
DEBUG_SUFFIX := _debug

ifeq ($(WINDOWS),true)
	LIB_PREFIX := $(PREFIX)
	TARGET := $(WINDOWS_TARGET_NAME)
else
	LIB_PREFIX := lib$(PREFIX)
	TARGET := $(LINUX_TARGET_NAME)
endif

-include make/*.mk

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

	# EXE_EXT := .out
	OBJ_EXT := .o
	LIB_EXT := .a

endif

EXE_EXT := $(TARGET)$(EXE_EXT)

include make/variables.mak
include make/dll.mak

LIBRARY_NAMES := alloc mem c-utils cpp-utils math networking/http networking/quic networking/tcp string structures serialize

OBJECTS_FOLDER := $(BUILD_FOLDER)/objects
LIBRARY_FOLDER := $(BUILD_FOLDER)/lib

COMPONENTS_C := $(strip $(foreach lib,$(LIBRARY_NAMES),$(patsubst $(SOURCE_FOLDER)/$(lib)/%.c,$(lib)/$(PREFIX)$(subst /,_,$(lib))_%,$(filter-out %/test.c,$(wildcard $(SOURCE_FOLDER)/$(lib)/*.c)))))

COMPONENTS_CPP := $(strip $(foreach lib,$(LIBRARY_NAMES),$(patsubst $(SOURCE_FOLDER)/$(lib)/%.cpp,$(lib)/$(PREFIX)$(subst /,_,$(lib))_%,$(filter-out %/test.cpp,$(wildcard $(SOURCE_FOLDER)/$(lib)/*.cpp)))))

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

# $(1) is lib
define new-obj-target
ifeq ($(1),alloc)
	$(1)_new_rel_flags := $(filter-out -O3,$(RELEASE_FLAGS))
else
	$(1)_new_rel_flags := $(RELEASE_FLAGS)
endif

$(filter $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%,$(OBJECTS_RELEASE_C)): $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%$(OBJ_EXT): $(SOURCE_FOLDER)/$(1)/%.c
	$$(MKDIR_COMMAND)
	$(CC) $(CFLAGS) $$(alloc_new_rel_flags) -g -c $$< -o $$@

$(filter $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%,$(OBJECTS_DEBUG_C)): $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%$(DEBUG_SUFFIX)$(OBJ_EXT): $(SOURCE_FOLDER)/$(1)/%.c
	$$(MKDIR_COMMAND)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $$< -o $$@

$(filter $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%,$(OBJECTS_RELEASE_CPP)): $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%$(OBJ_EXT): $(SOURCE_FOLDER)/$(1)/%.cpp
	$$(MKDIR_COMMAND)
	$(CCPLUS) $(CFLAGS) $$(alloc_new_rel_flags) -c $$< -o $$@

$(filter $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%,$(OBJECTS_DEBUG_CPP)): $(OBJECTS_FOLDER)/$(1)/$(PREFIX)$(subst /,_,$(1))_%$(DEBUG_SUFFIX)$(OBJ_EXT): $(SOURCE_FOLDER)/$(1)/%.cpp
	$$(MKDIR_COMMAND)
	$(CCPLUS) $(CFLAGS) $(DEBUG_FLAGS) -c $$< -o $$@
endef

$(foreach lib,$(LIBRARY_NAMES),$(eval $(call new-obj-target,$(lib))))


include make/zip.mak
