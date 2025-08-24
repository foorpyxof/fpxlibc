RELEASE_FLAGS := -DNDEBUG -O3
DEBUG_FLAGS := -DDEBUG -g -O0 -DFPXLIBC_DEBUG_ENABLE

# DEBUG_FLAGS += -fsanitize=address
# ^^^ uncomment for ASAN


# some folders
BUILD_FOLDER := build
SOURCE_FOLDER := src
MODULES_DIR := modules
TEST_DIR := $(SOURCE_FOLDER)/test

INCLUDE_DIRS := include $(MODULES_DIR) $(EXTRA_INCLUDE_DIRS)

# comp/link flags
INCLUDE_FLAGS := $(foreach dir,$(INCLUDE_DIRS),-I$(dir))

CFLAGS += $(INCLUDE_FLAGS)
CPPFLAGS += $(INCLUDE_FLAGS)

# some file name definitions, for targets
RELEASE_APP := $(BUILD_FOLDER)/release-$(EXE_EXT)
DEBUG_APP := $(BUILD_FOLDER)/debug-$(EXE_EXT)


MAIN_C := $(SOURCE_FOLDER)/main.c
