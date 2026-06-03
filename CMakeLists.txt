cmake_minimum_required(VERSION 3.25)

project(WeeView
    VERSION 1.0.10
    DESCRIPTION "A lightweight manga viewer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent LinguistTools Widgets)
find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)
pkg_check_modules(LIBARCHIVE REQUIRED IMPORTED_TARGET libarchive)

qt_standard_project_setup()

file(GLOB_RECURSE WEEVIEW_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.h
    src/*.hpp
)

qt_add_executable(weeview ${WEEVIEW_SOURCES})
qt_add_resources(weeview "weeview_assets"
    PREFIX "/"
    FILES
        assets/arrow_up.svg
        assets/book.svg
        assets/book_closed.svg
        assets/check_circle_outside.svg
        assets/chevron_left_circle.svg
        assets/chevron_right_circle.svg
        assets/circle.svg
        assets/folder_closed.svg
        assets/home.svg
        assets/postcard.svg
        assets/settings.svg
        assets/undo_history.svg
        assets/wrap_back.svg
        assets/wrap_forward.svg
)

qt_add_translations(weeview
    TS_FILES
        translations/weeview_ja.ts
)

target_include_directories(weeview PRIVATE src)

target_link_libraries(weeview
    PRIVATE
        Qt6::Concurrent
        Qt6::Widgets
        PkgConfig::LIBARCHIVE
        PkgConfig::LIBZIP
)

if(MSVC)
    target_compile_options(weeview PRIVATE /W4)
else()
    target_compile_options(weeview PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS weeview
    RUNTIME DESTINATION bin
)

# Optional: desktop integration
install(FILES packaging/weeview.desktop
    DESTINATION share/applications
)

# Optional: icon
install(FILES assets/weeview.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME weeview.svg
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "weeview")
set(CPACK_PACKAGE_VENDOR "WeeView")
set(CPACK_PACKAGE_CONTACT "M.Nakanishi <contact114514@nakanishi.dev>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "M.Nakanishi <contact114514@nakanishi.dev>")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)
