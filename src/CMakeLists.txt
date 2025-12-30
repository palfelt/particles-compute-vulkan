add_library(app)

target_sources(app
    PRIVATE
    loggerConfig.cpp
    tApp.cpp
    tTimer.cpp
)

add_subdirectory(helpers)
add_subdirectory(sim)
add_subdirectory(engine)

target_include_directories(app
    PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app PUBLIC
    TracyClient
    helpers
    sim
    engine
)