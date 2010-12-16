set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2 -I/usr/include/libxml2")
#add_definitions(-DPKGID=${Package_HG_ID})

include_directories(${RTLAB_SOURCE_DIR}/include)
include_directories(${RTLAB_SOURCE_DIR})

add_library( rtlab SHARED
    SessionStatistics.h
    Task.cpp Task.h
    Session.cpp Session.h
    Receiver.cpp Receiver.h
    Main.cpp Main.h
    Variable.cpp Variable.h
    Signal.cpp Signal.h
    Parameter.cpp Parameter.h

    lib/interface.cpp
    lib/ShmemDataStructures.h
    lib/Receiver.cpp lib/Receiver.h
    lib/Task.cpp lib/Task.h
    lib/Main.cpp lib/Main.h
    lib/Signal.cpp lib/Signal.h
    lib/Parameter.cpp lib/Parameter.h

    #etlproto/Server.cpp etlproto/Server.h
    #etlproto/Session.cpp etlproto/Session.h

    #msrproto/TSession.cpp msrproto/TSession.h
    msrproto/Session.cpp msrproto/Session.h
    msrproto/SubscriptionChange.cpp msrproto/SubscriptionChange.h
    msrproto/Outbuf.cpp msrproto/Outbuf.h
    msrproto/Inbuf.cpp msrproto/Inbuf.h
    msrproto/Attribute.cpp msrproto/Attribute.h
    msrproto/Server.cpp msrproto/Server.h
    msrproto/Task.cpp msrproto/Task.h
    msrproto/XmlDoc.cpp msrproto/XmlDoc.h
    )

target_link_libraries(rtlab ccext2 xml2)

IF (UNIX)
    ADD_CUSTOM_TARGET(tags etags *.cpp *.h */*.cpp */*.h)
    ADD_CUSTOM_TARGET(etags DEPENDS tags)
ENDIF (UNIX)
