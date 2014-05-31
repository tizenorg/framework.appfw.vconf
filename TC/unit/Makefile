CC ?= gcc

TARGETS = utc_ApplicationFW_vconf_set_int_func \
		utc_ApplicationFW_vconf_set_bool_func \
		utc_ApplicationFW_vconf_set_str_func \
		utc_ApplicationFW_vconf_set_dbl_func \
		utc_ApplicationFW_vconf_get_bool_func \
		utc_ApplicationFW_vconf_get_dbl_func \
		utc_ApplicationFW_vconf_get_int_func \
		utc_ApplicationFW_vconf_get_str_func \
		utc_ApplicationFW_vconf_unset_func\
		utc_ApplicationFW_vconf_unset_recursive_func \
		utc_ApplicationFW_vconf_keylist_new_func \
		utc_ApplicationFW_vconf_keylist_add_bool_func \
		utc_ApplicationFW_vconf_keylist_add_dbl_func \
		utc_ApplicationFW_vconf_keylist_add_int_func \
		utc_ApplicationFW_vconf_keylist_add_null_func \
		utc_ApplicationFW_vconf_keylist_add_str_func \
		utc_ApplicationFW_vconf_get_func \
		utc_ApplicationFW_vconf_keylist_nextnode_func \
		utc_ApplicationFW_vconf_keynode_get_bool_func \
		utc_ApplicationFW_vconf_keynode_get_dbl_func \
		utc_ApplicationFW_vconf_keynode_get_int_func \
		utc_ApplicationFW_vconf_keynode_get_name_func \
		utc_ApplicationFW_vconf_keynode_get_str_func \
		utc_ApplicationFW_vconf_keynode_get_type_func \
		utc_ApplicationFW_vconf_keylist_del_func \
		utc_ApplicationFW_vconf_keylist_free_func \
		utc_ApplicationFW_vconf_keylist_lookup_func \
		utc_ApplicationFW_vconf_keylist_rewind_func \
		utc_ApplicationFW_vconf_keynode_steal_str_func \
		utc_ApplicationFW_vconf_set_func \
		utc_ApplicationFW_vconf_sync_key_func \
		utc_ApplicationFW_vconf_ignore_key_changed_func \
		utc_ApplicationFW_vconf_notify_key_changed_func \
		utc_ApplicationFW_vconf_get_ext_errno_func

PKGS = vconf glib-2.0

LDFLAGS = `pkg-config --libs $(PKGS)`
LDFLAGS += $(TET_ROOT)/lib/tet3/tcm_s.o
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -ltcm_s
LDFLAGS += -L$(TET_ROOT)/lib/tet3 -lapi_s
LDFLAGS += -pthread

CFLAGS = -I. `pkg-config --cflags $(PKGS)`
CFLAGS += -I$(TET_ROOT)/inc/tet3
CFLAGS += -Wall

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(TARGETS)
