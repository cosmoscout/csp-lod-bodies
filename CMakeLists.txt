# ------------------------------------------------------------------------------------------------ #
#                                This file is part of CosmoScout VR                                #
#       and may be used under the terms of the MIT license. See the LICENSE file for details.      #
#                         Copyright: (c) 2019 German Aerospace Center (DLR)                        #
# ------------------------------------------------------------------------------------------------ #

# build plugin -------------------------------------------------------------------------------------

file(GLOB SOURCE_FILES src/*.cpp)

find_package(Threads REQUIRED)

add_library(csp-lod-planets SHARED 
  ${SOURCE_FILES}
)

target_link_libraries(csp-lod-planets
  PUBLIC
    cs-core
    cs-graphics
    cs-gui
    cs-scene
    curl::curl
    curlpp::curlpp
    c-ares::c-ares
    Threads::Threads
    tinygltf::tinygltf
)

# install plugin -----------------------------------------------------------------------------------

install(TARGETS   csp-lod-planets DESTINATION "share/plugins")
install(DIRECTORY "shaders"       DESTINATION "share/resources")
install(DIRECTORY "colormaps"     DESTINATION "share/resources")
install(DIRECTORY "textures"      DESTINATION "share/resources")
install(DIRECTORY "gui"           DESTINATION "share/resources")
