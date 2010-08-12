set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2 -I/usr/include/libxml2")
add_definitions(-DPKGID=${Package_HG_ID})

include_directories(${RTLAB_SOURCE_DIR}/include)

add_library( rtlab SHARED
    interface.cpp
    Main.cpp Main.h
    Variable.cpp Variable.h
    Signal.cpp Signal.h
    Parameter.cpp Parameter.h
    TCPServer.cpp TCPServer.h
    )
target_link_libraries(rtlab rt ccext2 xml2)
