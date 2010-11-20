set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2 -I/usr/include/libxml2")
#add_definitions(-DPKGID=${Package_HG_ID})

include_directories(${RTLAB_SOURCE_DIR}/include)
include_directories(.)

add_library( rtlab SHARED
    interface.cpp
    Task.cpp Task.h
    Session.cpp Session.h
    Main.cpp Main.h
    Variable.cpp Variable.h
    Signal.cpp Signal.h
    Parameter.cpp Parameter.h

    etlproto/Server.cpp etlproto/Server.h
    etlproto/Session.cpp etlproto/Session.h

    msrproto/Outbuf.cpp msrproto/Outbuf.h
    msrproto/Inbuf.cpp msrproto/Inbuf.h
    msrproto/Attribute.cpp msrproto/Attribute.h
    msrproto/Server.cpp msrproto/Server.h
    msrproto/Task.cpp msrproto/Task.h
    msrproto/Session.cpp msrproto/Session.h
    msrproto/XmlDoc.cpp msrproto/XmlDoc.h
    )

target_link_libraries(rtlab ccext2 xml2)

IF (UNIX)
    ADD_CUSTOM_TARGET(tags etags *.cpp *.h */*.cpp */*.h)
    ADD_CUSTOM_TARGET(etags DEPENDS tags)
ENDIF (UNIX)
