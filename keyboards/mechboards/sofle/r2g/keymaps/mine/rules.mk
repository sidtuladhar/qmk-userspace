ENCODER_MAP_ENABLE = yes
VIA_ENABLE = yes
VIAL_ENABLE = yes
VIALRGB_ENABLE = yes
AUTOCORRECT_ENABLE = yes # dictionary: autocorrect_dictionary.txt
DEBOUNCE_TYPE = sym_defer_pk # per-key; DEBOUNCE ms set in config.h
SRC += settings_menu.c   # right-screen settings menu
SRC += debug_perf.c      # per-stage timing for the menu's Debug page
OPT_DEFS += -DDEBUG_PERF_ENABLE
include $(dir $(lastword $(MAKEFILE_LIST)))pika_sources.mk

include $(dir $(lastword $(MAKEFILE_LIST)))keyglow.mk # keyglow
