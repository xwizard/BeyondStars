cmake_minimum_required (VERSION 3.7.1)
project (BeyondStars)
add_executable(BeyondStars alliance.cc  calc.cc camera.cc client.cc database.cc equip.cc
  error.cc frag.cc game.cc graphic.cc interface.cc os.cc planet.cc player.cc presence.cc
  SDL_rotozoom.c SDL_gfxPrimitives.c server.cc settings.cc ship.cc sockhelper.cc sound.cc sv.cc ticker.cc)

find_package (SDL 1.2.15 REQUIRED)
if (SDL_FOUND)
  include_directories(${SDL_INCLUDE_DIR})
  target_link_libraries (BeyondStars ${SDL_LIBRARY})
endif (SDL_FOUND)

find_package (SDL_net 1.2.8 REQUIRED)
if (SDL_NET_FOUND)
  include_directories(${SDL_NET_INCLUDE_DIR})
  target_link_libraries (BeyondStars ${SDL_NET_LIBRARY})
endif (SDL_NET_FOUND)