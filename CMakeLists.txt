cmake_minimum_required(VERSION 3.0)
project(zbar)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")

set(ENABLE_EAN     ON  CACHE BOOL "Enable EAN/UPC support")
set(ENABLE_I25     ON  CACHE BOOL "Enable IA2/5 support")
set(ENABLE_DATABAR ON  CACHE BOOL "Enable DataBar support")
set(ENABLE_CODABAR ON  CACHE BOOL "Enable Codabar support")
set(ENABLE_CODE39  ON  CACHE BOOL "Enable CODE-39 support")
set(ENABLE_CODE93  ON  CACHE BOOL "Enable CODE-93 support")
set(ENABLE_CODE128 ON  CACHE BOOL "Enable CODE-128 support")
set(ENABLE_PDF417  ON  CACHE BOOL "Enable PDF-417 support")
set(ENABLE_QRCODE  ON  CACHE BOOL "Enable QRCode support")
set(ZNO_MESSAGES   ON  CACHE BOOL "Disable diagnostic messages")
set(NO_STATS       ON  CACHE BOOL "Disable image scanner's stats")
set(DEBUG_SVG      OFF CACHE BOOL "Create debug output")
set(MSVC_STATIC_BUILD ON CACHE BOOL "Link standard library statically (MSVC only)")
set(ENABLE_CHARSET_ISO8859 ON CACHE BOOL "Enable ISO8859 charsets")
set(ENABLE_CHARSET_WINDOWS ON CACHE BOOL "Enable charsets CP1250, CP1251, CP1252, CP1256")

include(CheckIncludeFile)

check_include_file("inttypes.h" HAVE_INTTYPES_H)
if (HAVE_INTTYPES_H)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} HAVE_INTTYPES_H)
endif()

check_include_file("errno.h" HAVE_ERRNO_H)
if (HAVE_ERRNO_H)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} HAVE_ERRNO_H)
endif()

check_include_file("sys/time.h" HAVE_SYS_TIME_H)
if (HAVE_SYS_TIME_H)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} HAVE_SYS_TIME_H)
endif()

if (ZNO_MESSAGES)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ZNO_MESSAGES)
endif()

if (NO_STATS)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} NO_STATS)
endif()

if (DEBUG_SVG)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} DEBUG_SVG)
endif()

set (CFILES
  zbar/config.c
  zbar/decoder.c
  zbar/error.c
  zbar/image.c
  zbar/img_scanner.c
  zbar/refcnt.c
  zbar/scanner.c
  zbar/symbol.c
)

if (ENABLE_QRCODE)
  set (ZBAR_QRCODE ${ZBAR_QRCODE}
    zbar/qrcode/bch15_5.c
    zbar/qrcode/binarize.c
    zbar/qrcode/isaac.c
    zbar/qrcode/qrdec.c
    zbar/qrcode/qrdectxt.c
    zbar/qrcode/rs.c
    zbar/qrcode/util.c
  )
  set (ZBAR_DECODER ${ZBAR_DECODER}
    zbar/decoder/qr_finder.c
  )
  set (ZBAR_TEXT ${ZBAR_TEXT}
    zbar/text/charsets.c
    zbar/text/text_conv.c
  )
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_QRCODE)
endif()

if (ENABLE_EAN)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/ean.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_EAN)
endif()

if (ENABLE_I25)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/i25.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_I25)
endif()

if (ENABLE_DATABAR)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/databar.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_DATABAR)
endif()

if (ENABLE_CODABAR)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/codabar.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CODABAR)
endif()

if (ENABLE_CODE39)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/code39.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CODE39)
endif()

if (ENABLE_CODE93)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/code93.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CODE93)
endif()

if (ENABLE_CODE128)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/code128.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CODE128)
endif()

if (ENABLE_PDF417)
  set (ZBAR_DECODER ${ZBAR_DECODER} zbar/decoder/pdf417.c)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_PDF417)
endif()

if (ENABLE_CHARSET_ISO8859)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CHARSET_ISO8859)
endif()

if (ENABLE_CHARSET_WINDOWS)
  set (ZBAR_DEFINES ${ZBAR_DEFINES} ENABLE_CHARSET_WINDOWS)
endif()

set (CFILES ${CFILES}
  ${ZBAR_QRCODE}
  ${ZBAR_DECODER}
  ${ZBAR_TEXT}
)

source_group("Source Files\\decoder" FILES ${ZBAR_DECODER})
source_group("Source Files\\qrcode"  FILES ${ZBAR_QRCODE})
source_group("Source Files\\text"    FILES ${ZBAR_TEXT})

add_library(libzbar ${CFILES})

if (DEBUG_SVG)
  find_package(PNG REQUIRED)
  target_link_libraries(libzbar PNG::PNG)
endif()

target_include_directories(libzbar PUBLIC  include)
target_include_directories(libzbar PRIVATE zbar)
target_compile_definitions(libzbar PRIVATE ${ZBAR_DEFINES})

set (CFILES examples/scan_image.c)

add_executable(scan_image ${CFILES})

target_link_libraries(scan_image libzbar)

find_package(PNG REQUIRED)
target_link_libraries(scan_image PNG::PNG)

if (WIN32)
  target_link_libraries(scan_image winmm.lib)
endif()

if (MSVC)
  set (MSVC_DISABLE_WARNINGS "/wd4996 /wd4554")
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Oi /GS- /GF ${MSVC_DISABLE_WARNINGS}")
  set (CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} ${MSVC_DISABLE_WARNINGS}")
  if (MSVC_STATIC_BUILD)
    set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
  endif()
endif()
