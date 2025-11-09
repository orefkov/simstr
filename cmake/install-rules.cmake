if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/simstr-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package simstr)

install(
    DIRECTORY
    include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT simstr_Development
)

install(
    TARGETS simstr_simstr
    EXPORT simstrTargets
    RUNTIME #
    COMPONENT simstr_Runtime
    LIBRARY #
    COMPONENT simstr_Runtime
    NAMELINK_COMPONENT simstr_Development
    ARCHIVE #
    COMPONENT simstr_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    simstr_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE simstr_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(simstr_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${simstr_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT simstr_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${simstr_INSTALL_CMAKEDIR}"
    COMPONENT simstr_Development
)

install(
    EXPORT simstrTargets
    NAMESPACE simstr::
    DESTINATION "${simstr_INSTALL_CMAKEDIR}"
    COMPONENT simstr_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
