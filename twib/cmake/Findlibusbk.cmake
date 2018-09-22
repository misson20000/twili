if (LIBUSBK_LIBRARIES AND LIBUSBK_INCLUDE_DIRS)
  # in cache already
  set(LIBUSB_FOUND TRUE)
else (LIBUSBK_LIBRARIES AND LIBUSBK_INCLUDE_DIRS)
  find_path(LIBUSBK_INCLUDE_DIR
    NAMES
      libusbk.h
    PATHS C:/libusbK-dev-kit/includes
  )

  find_library(LIBUSBK_LIBRARY
    NAMES
      usbK
    PATHS
      C:/libusbk-dev-kit
    PATH_SUFFIXES bin/lib/static/amd64
  )

  set(LIBUSBK_INCLUDE_DIRS ${LIBUSBK_INCLUDE_DIR})
  set(LIBUSBK_LIBRARIES ${LIBUSBK_LIBRARY})

  if (LIBUSBK_INCLUDE_DIRS AND LIBUSBK_LIBRARIES)
    set(LIBUSBK_FOUND TRUE)
  endif(LIBUSBK_INCLUDE_DIRS AND LIBUSBK_LIBRARIES)

  if (LIBUSBK_FOUND)
    if (NOT libusbK_FIND_QUIETLY)
      message(STATUS "Found libusbk:")
      message(STATUS " - Includes: ${LIBUSBK_INCLUDE_DIRS}")
      message(STATUS " - Libraries: ${LIBUSBK_LIBRARIES}")
    endif(NOT libusbK_FIND_QUIETLY)
  else(LIBUSBK_FOUND)
    if (libusbK_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libusbK")
    endif(libusbK_FIND_REQUIRED)
  endif(LIBUSBK_FOUND)

  # show the LIBUSBK_INCLUDE_DIRS and LIBUSBK_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBUSBK_INCLUDE_DIRS LIBUSBK_LIBRARIES)
endif(LIBUSBK_LIBRARIES AND LIBUSBK_INCLUDE_DIRS)

