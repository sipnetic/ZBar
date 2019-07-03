LOCAL_PATH := $(call my-dir)
LOCAL_SHORT_COMMANDS := true

include $(CLEAR_VARS)

PROJ_REL := ../zbar
PROJ_DIR := $(LOCAL_PATH)/$(PROJ_REL)

LOCAL_MODULE := zbar

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -Wall -fno-strict-aliasing \
 -Wno-sign-compare -Wno-shift-op-parentheses -Wno-logical-op-parentheses -Wno-bitwise-op-parentheses \
 -DHAVE_INTTYPES_H -DHAVE_ERRNO_H -DHAVE_SYS_TIME_H \
 -DNO_STATS -DZNO_MESSAGES

DEBUG_CFLAGS    := -D_DEBUG -g
DEBUG_LDFLAGS   := -g

RELEASE_CFLAGS  := -DNDEBUG -fvisibility=hidden
RELEASE_LDFLAGS :=

LOCAL_C_INCLUDES += $(PROJ_DIR) $(PROJ_DIR)/../include

SRC_FILES := \
 config.c \
 decoder.c \
 error.c \
 image.c \
 img_scanner.c \
 refcnt.c \
 scanner.c \
 symbol.c

SRC_FILES_QRCODE := \
 qrcode/bch15_5.c \
 qrcode/binarize.c \
 qrcode/isaac.c \
 qrcode/qrdec.c \
 qrcode/qrdectxt.c \
 qrcode/rs.c \
 qrcode/util.c \
 decoder/qr_finder.c \
 text/charsets.c \
 text/text_conv.c

SRC_FILES_EAN     := decoder/ean.c
SRC_FILES_I25     := decoder/i25.c
SRC_FILES_DATABAR := decoder/databar.c
SRC_FILES_CODABAR := decoder/codabar.c
SRC_FILES_CODE39  := decoder/code39.c
SRC_FILES_CODE93  := decoder/code93.c
SRC_FILES_CODE128 := decoder/code128.c
SRC_FILES_PDF417  := decoder/pdf417.c

ifeq ($(ENABLE_QRCODE),1)
 LOCAL_CFLAGS += -DENABLE_QRCODE
 SRC_FILES += $(SRC_FILES_QRCODE)
endif

ifeq ($(ENABLE_EAN),1)
 LOCAL_CFLAGS += -DENABLE_EAN
 SRC_FILES += $(SRC_FILES_EAN)
endif

ifeq ($(ENABLE_I25),1)
 LOCAL_CFLAGS += -DENABLE_I25
 SRC_FILES += $(SRC_FILES_I25)
endif

ifeq ($(ENABLE_DATABAR),1)
 LOCAL_CFLAGS += -DENABLE_DATABAR
 SRC_FILES += $(SRC_FILES_DATABAR)
endif

ifeq ($(ENABLE_CODABAR),1)
 LOCAL_CFLAGS += -DENABLE_CODABAR
 SRC_FILES += $(SRC_FILES_CODABAR)
endif

ifeq ($(ENABLE_CODE39),1)
 LOCAL_CFLAGS += -DENABLE_CODE39
 SRC_FILES += $(SRC_FILES_CODE39)
endif

ifeq ($(ENABLE_CODE93),1)
 LOCAL_CFLAGS += -DENABLE_CODE93
 SRC_FILES += $(SRC_FILES_CODE93)
endif

ifeq ($(ENABLE_CODE128),1)
 LOCAL_CFLAGS += -DENABLE_CODE128
 SRC_FILES += $(SRC_FILES_CODE128)
endif

ifeq ($(ENABLE_PDF417),1)
 LOCAL_CFLAGS += -DENABLE_PDF417
 SRC_FILES += $(SRC_FILES_PDF417)
endif

ifeq ($(ENABLE_CHARSET_ISO8859),1)
 LOCAL_CFLAGS += -DENABLE_CHARSET_ISO8859
endif

ifeq ($(ENABLE_CHARSET_WINDOWS),1)
 LOCAL_CFLAGS += -DENABLE_CHARSET_WINDOWS
endif

LOCAL_SRC_FILES += $(addprefix $(PROJ_REL)/, $(SRC_FILES))

APP_DEBUG := $(strip $(NDK_DEBUG))
ifeq ($(APP_DEBUG),1)
 LOCAL_CPPFLAGS += $(DEBUG_CFLAGS)
 LOCAL_CFLAGS   += $(DEBUG_CFLAGS)
 LOCAL_LDLIBS   += $(DEBUG_LDFLAGS)
else
 LOCAL_CPPFLAGS += $(RELEASE_CFLAGS)
 LOCAL_CFLAGS   += $(RELEASE_CFLAGS)
 LOCAL_LDLIBS   += $(RELEASE_LDFLAGS)
endif

include $(BUILD_SHARED_LIBRARY)
