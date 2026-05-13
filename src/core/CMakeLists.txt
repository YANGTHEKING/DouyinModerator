set(CORE_SOURCES
    HttpRequester.cpp
    ProtobufDecoder.cpp
    MessageRouter.cpp
    ConnectionManager.cpp
    AutoResponseEngine.cpp
    TimerEngine.cpp
    BarrageSender.cpp
    LoginManager.cpp
    EmojiResolver.cpp
    AppSettings.cpp
    ${PROTO_SRCS}
)

set(CORE_HEADERS
    HttpRequester.h
    ProtobufDecoder.h
    MessageRouter.h
    ConnectionManager.h
    AutoResponseEngine.h
    TimerEngine.h
    BarrageSender.h
    LoginManager.h
    EmojiResolver.h
    AppSettings.h
    ${PROTO_HDRS}
)

add_library(core_lib STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_INCLUDE_DIR})
target_link_libraries(core_lib PUBLIC
    models_lib
    Qt6::Widgets Qt6::Network Qt6::WebSockets Qt6::WebEngineWidgets
    protobuf::libprotobuf ZLIB::ZLIB)
