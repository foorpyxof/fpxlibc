MINGW_DLLS := libwinpthread-1.dll

REQUIRED_DLLS := $(MINGW_DLLS)

find_pipe := head -n 1 | xargs -I found ln -s found .

$(MINGW_DLLS):
	find $(MINGW_DIRECTORY) -name "$@" | $(find_pipe)
