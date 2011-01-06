SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

INCLUDE_DIRECTORIES(
    ${PROJECT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/msrproto)

LINK_DIRECTORIES(${pdcomserv_BINARY_DIR}/src)

ADD_EXECUTABLE(test1 test1.cpp)
TARGET_LINK_LIBRARIES(test1 ${PROJECT_NAME})

ADD_EXECUTABLE(parser
    parser.cpp ${PROJECT_SOURCE_DIR}/src/msrproto/Inbuf.cpp)

#ADD_TEST(test1 test1)
ADD_TEST(parser parser)
