include $(THEOS)/makefiles/common.mk

TWEAK_NAME = UnzipDylib

UnzipDylib_FILES = Tweak.xm zipcore.c
UnzipDylib_CFLAGS = -fobjc-arc
UnzipDylib_LDFLAGS = -lz
UnzipDylib_FRAMEWORKS = Foundation

include $(THEOS_MAKE_PATH)/tweak.mk
