project(qt_sdl)

SET(SOURCES_QT_SDL
    main.cpp
    main_shaders.h
	CheatsDialog.cpp
    EmuSettingsDialog.cpp
    InputConfigDialog.cpp
    VideoSettingsDialog.cpp
    AudioSettingsDialog.cpp
    WifiSettingsDialog.cpp
    InterfaceSettingsDialog.cpp
    Input.cpp
    LAN_PCap.cpp
    LAN_Socket.cpp
    OSD.cpp
    OSD_shaders.h
    font.h
    Platform.cpp
    PlatformConfig.cpp
    
    ArchiveUtil.h
    ArchiveUtil.cpp

    ../Util_ROM.cpp
    ../Util_Video.cpp
    ../Util_Audio.cpp
    ../FrontendUtil.h
    ../mic_blow.h

    ../../../melon.qrc
)

if (WIN32)
    #set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
endif()

if (BUILD_STATIC AND QT5_STATIC_DIR)
    set(QT5_STATIC_BASE ${QT5_STATIC_DIR}/lib/cmake/Qt5)
    set(Qt5_DIR ${QT5_STATIC_BASE})
    set(Qt5Core_DIR ${QT5_STATIC_BASE}Core)
    set(Qt5Gui_DIR ${QT5_STATIC_BASE}Gui)
    set(Qt5Widgets_DIR ${QT5_STATIC_BASE}Widgets)
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Threads REQUIRED)
#find_package(PkgConfig REQUIRED)
#find_package(Iconv REQUIRED)
#find_package(SDL2 REQUIRED)
#pkg_check_modules(SDL2 REQUIRED sdl2)
#pkg_check_modules(SLIRP REQUIRED slirp)

if (APPLE)
    # Find libarchive on macOS, because macOS only provides the library, not the headers
    execute_process(COMMAND brew --prefix libarchive
                    OUTPUT_VARIABLE LIBARCHIVE_DIR
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND CMAKE_PREFIX_PATH "${LIBARCHIVE_DIR}")
endif()
#pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
#add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

if (WIN32 AND (CMAKE_BUILD_TYPE STREQUAL Release))
    add_executable(melonDS_Qt WIN32 ${SOURCES_QT_SDL})
else()
    add_executable(melonDS_Qt ${SOURCES_QT_SDL})
endif()

target_link_libraries(melonDS_Qt ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(melonDS_Qt PRIVATE G:/libraries/SDL2-2.0.14/include G:/libraries/glew-2.1.0/include) # ${SLIRP_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS}  
target_link_directories(melonDS_Qt PRIVATE G:/libraries/SDL2-2.0.14/lib/x64 G:/libraries/glew-2.1.0/lib/Release/x64) # ${SLIRP_LIBRARY_DIRS} ${LIBARCHIVE_LIBRARY_DIRS}  
target_include_directories(melonDS_Qt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS_Qt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(melonDS_Qt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS_Qt core)

target_link_libraries(melonDS_Qt SDL2 SDL2main glew32s) # ${SLIRP_LIBRARIES} ${LIBARCHIVE_LIBRARIES}  


if (NOT Iconv_IS_BUILT_IN)
	#target_link_libraries(melonDS_Qt iconv)
endif()

if (UNIX)
    option(PORTABLE "Make a portable build that looks for its configuration in the current directory" OFF)
    target_link_libraries(melonDS_Qt Qt5::Core Qt5::Gui Qt5::Widgets)
    if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(melonDS_Qt dl)
    endif()
elseif (WIN32)
    option(PORTABLE "Make a portable build that looks for its configuration in the current directory" ON)
    #configure_file("${CMAKE_SOURCE_DIR}/melon.rc.in" "${CMAKE_SOURCE_DIR}/melon.rc")
    #target_sources(melonDS_Qt PUBLIC "${CMAKE_SOURCE_DIR}/melon.rc")

    target_link_libraries(melonDS_Qt comctl32 d2d1 dwrite uxtheme ws2_32 iphlpapi gdi32)
    if (BUILD_STATIC)
        target_link_libraries(melonDS_Qt imm32 winmm version setupapi -static Qt5::Core Qt5::Gui Qt5::Widgets zstd)
    else()
        target_link_libraries(melonDS_Qt Qt5::Core Qt5::Gui Qt5::Widgets)
    endif()
endif()

if (PORTABLE)
    add_definitions(-DPORTABLE)
endif()

if (APPLE)
    set_target_properties(melonDS_Qt PROPERTIES
                         MACOSX_BUNDLE true
                         MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/melon.plist.in
                         OUTPUT_NAME melonDS_Qt
                         )

    # Copy icon into the bundle
    target_sources(melonDS_Qt PRIVATE "${CMAKE_SOURCE_DIR}/melon.icns")
    set_source_files_properties("${CMAKE_SOURCE_DIR}/melon.icns" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

endif()

#install(FILES ../../../net.kuribo64.melonDS_Qt.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
#install(FILES ../../../icon/melon_16x16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(FILES ../../../icon/melon_32x32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(FILES ../../../icon/melon_48x48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(FILES ../../../icon/melon_64x64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(FILES ../../../icon/melon_128x128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(FILES ../../../icon/melon_256x256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME net.kuribo64.melonDS_Qt.png)
#install(TARGETS melonDS_Qt BUNDLE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
