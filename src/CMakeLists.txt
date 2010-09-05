set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O2 -I/usr/include/libxml2")
#add_definitions(-DPKGID=${Package_HG_ID})

include_directories(${RTLAB_SOURCE_DIR}/include)
include_directories(.)

add_library( rtlab SHARED
    interface.cpp
    MainTask.cpp Main.cpp Main.h
    Variable.cpp Variable.h
    Signal.cpp Signal.h
    Parameter.cpp Parameter.h
    etlproto/Server.cpp etlproto/Server.h
    etlproto/Session.cpp etlproto/Session.h
    MsrServer.cpp MsrServer.h
    MsrSessionTask.cpp MsrSession.cpp MsrSession.h
    XmlDoc.cpp XmlDoc.h
    )

target_link_libraries(rtlab ccext2 xml2 xerces-c)
