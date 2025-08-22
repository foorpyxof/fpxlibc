# CFLAGS := -std=c11
# CPPFLAGS := -std=c++11

BASE_FLAGS :=  -Wall -Wextra -Wpedantic -Werror -Wno-gnu-zero-variadic-macro-arguments -Wno-unknown-warning-option -Wno-variadic-macro-arguments-omitted

CFLAGS += $(BASE_FLAGS)
CPPFLAGS += $(BASE_FLAGS)

ROOT_DIR != pwd
ROOT_DIR_NAME != basename $(ROOT_DIR)
