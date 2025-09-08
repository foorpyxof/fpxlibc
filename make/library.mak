$(foreach lib,$(LIBRARY_NAMES),$(eval $(lib)_OBJ_REL := $(filter $(OBJECTS_FOLDER)/$(lib)/%,$(OBJECTS_RELEASE_C) $(OBJECTS_RELEASE_CPP))))
$(foreach lib,$(LIBRARY_NAMES),$(eval $(lib)_OBJ_DBG := $(filter $(OBJECTS_FOLDER)/$(lib)/%,$(OBJECTS_DEBUG_C) $(OBJECTS_DEBUG_CPP))))

define new-lib-target
$(1)_lib_dir := $(LIBRARY_FOLDER)/$(patsubst ./%,%,$(dir $(1)))
$(1)_lib_name := $(LIB_PREFIX)$(notdir $(1))$(LIB_EXT)
$(1)_lib_debug_name := $(LIB_PREFIX)$(notdir $(1))$(DEBUG_SUFFIX)$(LIB_EXT)


LIBS_RELEASE += $$($(1)_lib_dir)$$($(1)_lib_name)
$$($(1)_lib_dir)$$($(1)_lib_name): $($(1)_OBJ_REL)
	-if ! [ -d $$($(1)_lib_dir) ]; then mkdir -p $$($(1)_lib_dir); fi
	-if [ -f $$@ ]; then rm $$@; fi
	$(AR) cr --thin $$@ $$? && echo -e 'create $$@\naddlib $$@\nsave\nend' | ar -M

LIBS_DEBUG += $$($(1)_lib_dir)$$($(1)_lib_debug_name)
$$($(1)_lib_dir)$$($(1)_lib_debug_name): $($(1)_OBJ_DBG)
	-if ! [ -d $$($(1)_lib_dir) ]; then mkdir -p $$($(1)_lib_dir); fi
	-if [ -f $$@ ]; then rm $$@; fi
	$(AR) cr --thin $$@ $$? && echo -e 'create $$@\naddlib $$@\nsave\nend' | ar -M

endef

$(foreach lib,$(LIBRARY_NAMES),$(eval $(call new-lib-target,$(lib))))
