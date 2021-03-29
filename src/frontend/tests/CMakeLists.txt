project(test_sdl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TEST_SDL_SOURCES
    main.cpp
    main_shaders.h
    Platform.cpp
    PlatformConfig.cpp
    Input.cpp
    OSD.cpp
    OSD_shaders.h
    font.h
    
    ../Util_ROM.cpp
    ../Util_Video.cpp
    ../Util_Audio.cpp
    ../FrontendUtil.h)

find_package(Threads REQUIRED)
#find_package(PkgConfig REQUIRED)

#find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Boost REQUIRED date_time system)


#find_package(Iconv REQUIRED)
#pkg_check_modules(SDL2 REQUIRED sdl2)
#pkg_check_modules(SLIRP REQUIRED slirp)

#pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
#add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

add_executable(melonDS ${TEST_SDL_SOURCES})


target_include_directories(melonDS PRIVATE ${Boost_INCLUDE_DIRS} G:/libraries/SDL2-2.0.14/include G:/libraries/glew-2.1.0/include)
target_link_directories(melonDS PRIVATE ${Boost_LIBRARY_DIRS} G:/libraries/SDL2-2.0.14/lib/x64 G:/libraries/glew-2.1.0/lib/Release/x64) # ${SDL2_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS} 
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS core)


target_link_libraries(melonDS ${Boost_LIBRARIES} opengl32 SDL2 SDL2main glew32s)


#install(FILES ../../../net.kuribo64.melonDS.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
#install(FILES ../../../icon/melon_16x16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_32x32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_48x48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_64x64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_128x128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_256x256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME net.kuribo64.melonDS.png)
#install(TARGETS melonDS BUNDLE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)