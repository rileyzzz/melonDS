project(em_sdl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EM_SDL_SOURCES
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
find_package(PkgConfig REQUIRED)
#find_package(Iconv REQUIRED)
#pkg_check_modules(SDL2 REQUIRED sdl2)
#pkg_check_modules(SLIRP REQUIRED slirp)

#pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
#add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

add_executable(melonDS ${EM_SDL_SOURCES})


#this is a mess LOL
set(EPOXY_INCLUDE_DIRS  ../../../empoxy/include
                        ../../../empoxy/build/include
                        ../../../empoxy/src
                        ../../../empoxy/build/src)

set(EPOXY_LIBRARY_DIRS ../../../empoxy/build/src)

#emscripten options and other fun stuff
if(EMSCRIPTEN)
    message("emscripten epic stuff")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    
    # -s ALLOW_MEMORY_GROWTH=1
    #-s USE_PTHREADS=1 -pthread --shared-memory --no-check-features -Wl

    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    
    #debug
    #set(EM_FLAGS "-lidbfs.js -g -s INITIAL_MEMORY=512MB -s USE_BOOST_HEADERS=1 -s MAX_WEBGL_VERSION=2 -s USE_REGAL=1 -s FULL_ES2=1 -s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s USE_SDL=2 -s USE_PTHREADS=1 -pthread -s LLD_REPORT_UNDEFINED -s EXPORTED_FUNCTIONS='[\"_main\", \"_file_drag\", \"_sync_check\", \"_triggerFullscreen\", \"_check_required_files\", \"_onChangeAudioVolume\", \"_onChangeScreenSize\", \"_onChangeScreenRotation\", \"_onChangeScreenGap\", \"_onChangeScreenLayout\", \"_onChangeScreenSwap\", \"_onChangeScreenFiltering\", \"_onChangeScreenSizing\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' --preload-file /workspace/melonDS/firmware@/")
    #set(EM_FLAGS "-lidbfs.js -g3 -s ASSERTIONS=1 -s INITIAL_MEMORY=512MB -s USE_BOOST_HEADERS=1 -s MAX_WEBGL_VERSION=2 -s USE_REGAL=1 -s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s USE_SDL=2 -s LLD_REPORT_UNDEFINED -s EXPORTED_FUNCTIONS='[\"_main\", \"_file_drag\", \"_sync_check\", \"_triggerFullscreen\", \"_check_required_files\", \"_onChangeAudioVolume\", \"_onChangeScreenSize\", \"_onChangeScreenRotation\", \"_onChangeScreenGap\", \"_onChangeScreenLayout\", \"_onChangeScreenSwap\", \"_onChangeScreenFiltering\", \"_onChangeScreenSizing\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' --preload-file /home/riley/melonDS/melonDS/firmware@/")

    #fast
    # -s USE_REGAL=1 -s FULL_ES2=1 -s FULL_ES3=1  -fno-exceptions  -s OFFSCREEN_FRAMEBUFFER=1 
    #  --preload-file /workspace/melonDS/firmware@/
    set(EM_FLAGS "-flto -ffast-math -lidbfs.js -fno-rtti -g0 -O3 -s DISABLE_EXCEPTION_CATCHING=1 -s INITIAL_MEMORY=512MB -s USE_BOOST_HEADERS=1 -s MAX_WEBGL_VERSION=2 -s USE_REGAL=1 -s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s USE_SDL=2 -s USE_PTHREADS=1 -pthread -s LLD_REPORT_UNDEFINED -s EXPORTED_FUNCTIONS='[\"_main\", \"_file_drag\", \"_sync_check\", \"_triggerFullscreen\", \"_check_required_files\", \"_onChangeAudioVolume\", \"_onChangeScreenSize\", \"_onChangeScreenRotation\", \"_onChangeScreenGap\", \"_onChangeScreenLayout\", \"_onChangeScreenSwap\", \"_onChangeScreenFiltering\", \"_onChangeScreenSizing\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' --preload-file /home/riley/melonDS/melonDS/firmware@/")
    #set(EM_FLAGS "-flto -lidbfs.js -fno-rtti -g0 -O3 -s DISABLE_EXCEPTION_CATCHING=1 -s INITIAL_MEMORY=512MB -s USE_BOOST_HEADERS=1 -s MAX_WEBGL_VERSION=2 -s USE_REGAL=1 -s FULL_ES3=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2 -s USE_SDL=2 -s LLD_REPORT_UNDEFINED -s EXPORTED_FUNCTIONS='[\"_main\", \"_file_drag\", \"_sync_check\", \"_triggerFullscreen\", \"_check_required_files\", \"_onChangeAudioVolume\", \"_onChangeScreenSize\", \"_onChangeScreenRotation\", \"_onChangeScreenGap\", \"_onChangeScreenLayout\", \"_onChangeScreenSwap\", \"_onChangeScreenFiltering\", \"_onChangeScreenSizing\"]' -s EXPORTED_RUNTIME_METHODS='[\"ccall\",\"cwrap\"]' --preload-file /home/riley/melonDS/melonDS/firmware@/")
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EM_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${EM_FLAGS}")

    set_target_properties(melonDS PROPERTIES LINK_FLAGS "-g0 -O3 -flto -ffast-math")
    #set_target_properties(melonDS PROPERTIES LINK_FLAGS "-g3 -O3 -flto")
    set_target_properties(melonDS PROPERTIES COMPILE_FLAGS "-g0 -O3 -flto -ffast-math -s USE_BOOST_HEADERS=1")
    #set_target_properties(melonDS PROPERTIES COMPILE_FLAGS "-g3 -O3 -flto -s USE_BOOST_HEADERS=1")
endif()

#target_include_directories(melonDS PRIVATE ${SDL2_INCLUDE_DIRS} ${SLIRP_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS})
target_include_directories(melonDS PRIVATE ${SDL2_INCLUDE_DIRS} ${EPOXY_INCLUDE_DIRS})
#target_link_directories(melonDS PRIVATE ${SDL2_LIBRARY_DIRS} ${SLIRP_LIBRARY_DIRS} ${LIBARCHIVE_LIBRARY_DIRS})
target_link_directories(melonDS PRIVATE ${SDL2_LIBRARY_DIRS} ${EPOXY_LIBRARY_DIRS})
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS core)

#target_link_libraries(melonDS ${SDL2_LIBRARIES} ${SLIRP_LIBRARIES} ${LIBARCHIVE_LIBRARIES})
target_link_libraries(melonDS ${SDL2_LIBRARIES})


#install(FILES ../../../net.kuribo64.melonDS.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
#install(FILES ../../../icon/melon_16x16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_32x32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_48x48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_64x64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_128x128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME net.kuribo64.melonDS.png)
#install(FILES ../../../icon/melon_256x256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME net.kuribo64.melonDS.png)
#install(TARGETS melonDS BUNDLE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)