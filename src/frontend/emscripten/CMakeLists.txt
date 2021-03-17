project(em_sdl)

set(EM_SDL_SOURCES
    main.cpp)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
#find_package(Iconv REQUIRED)
#pkg_check_modules(SDL2 REQUIRED sdl2)
#pkg_check_modules(SLIRP REQUIRED slirp)

#pkg_check_modules(LIBARCHIVE REQUIRED libarchive)
#add_compile_definitions(ARCHIVE_SUPPORT_ENABLED)

add_executable(melonDS ${EM_SDL_SOURCES})

#target_include_directories(melonDS PRIVATE ${SDL2_INCLUDE_DIRS} ${SLIRP_INCLUDE_DIRS} ${LIBARCHIVE_INCLUDE_DIRS})
target_include_directories(melonDS PRIVATE ${SDL2_INCLUDE_DIRS})
#target_link_directories(melonDS PRIVATE ${SDL2_LIBRARY_DIRS} ${SLIRP_LIBRARY_DIRS} ${LIBARCHIVE_LIBRARY_DIRS})
target_link_directories(melonDS PRIVATE ${SDL2_LIBRARY_DIRS})
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(melonDS PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(melonDS core)

#target_link_libraries(melonDS ${SDL2_LIBRARIES} ${SLIRP_LIBRARIES} ${LIBARCHIVE_LIBRARIES})
target_link_libraries(melonDS ${SDL2_LIBRARIES})


install(FILES ../../../net.kuribo64.melonDS.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES ../../../icon/melon_16x16.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/16x16/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../../icon/melon_32x32.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/32x32/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../../icon/melon_48x48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../../icon/melon_64x64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../../icon/melon_128x128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps RENAME net.kuribo64.melonDS.png)
install(FILES ../../../icon/melon_256x256.png DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/256x256/apps RENAME net.kuribo64.melonDS.png)
install(TARGETS melonDS BUNDLE DESTINATION ${CMAKE_BINARY_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)