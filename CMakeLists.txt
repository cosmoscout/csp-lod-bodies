# ------------------------------------------------------------------------------------------------ #
#                                This file is part of CosmoScout VR                                #
#       and may be used under the terms of the MIT license. See the LICENSE file for details.      #
#                         Copyright: (c) 2019 German Aerospace Center (DLR)                        #
# ------------------------------------------------------------------------------------------------ #

# build plugin -------------------------------------------------------------------------------------

file(GLOB SOURCE_FILES src/*.cpp)

find_package(Threads REQUIRED)

add_library(csp-lod-bodies SHARED 
  ${SOURCE_FILES}
)

target_link_libraries(csp-lod-bodies
  PUBLIC
    cs-core
    curl::curl
    curlpp::curlpp
    c-ares::c-ares
    Threads::Threads
)

# install plugin -----------------------------------------------------------------------------------

install(TARGETS   csp-lod-bodies DESTINATION "share/plugins")
install(DIRECTORY "shaders"      DESTINATION "share/resources")
install(DIRECTORY "colormaps"    DESTINATION "share/resources")
install(DIRECTORY "textures"     DESTINATION "share/resources")
install(DIRECTORY "gui"          DESTINATION "share/resources")
