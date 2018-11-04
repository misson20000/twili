if (SETUPAPI_LIBRARIES)
  # in cache already
  set(SETUPAPI_FOUND TRUE)
else()
  find_library(SETUPAPI_LIBRARY NAMES SetupAPI PATHS "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.17134.0/um/x64")

  set(SETUPAPI_LIBRARIES ${SETUPAPI_LIBRARY})

  if(SETUPAPI_LIBRARIES)
    set(SETUPAPI_FOUND TRUE)
  endif()

  if(SETUPAPI_FOUND)
    if (NOT SETUPAPI_FIND_QUIETLY)
      message(STATUS "Found SetupAPI:")
      message(STATUS " - Libraries: ${SETUPAPI_LIBRARIES}")
    endif()
  else()
    if (SETUPAPI_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find SetupAPI")
    endif()
  endif()

  # show only in the advanced view
  mark_as_advanced(SETUPAPI_LIBRARIES)
endif()
