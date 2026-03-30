cmake_minimum_required(VERSION 3.14)
project(GravitySim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Release build with optimisations by default ──
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ── Find packages ──
find_package(OpenGL  REQUIRED)
find_package(glfw3   REQUIRED)
find_package(GLEW    REQUIRED)
find_package(glm     REQUIRED)

add_executable(gravity_sim gravity_sim.cpp)

target_link_libraries(gravity_sim
    PRIVATE
        OpenGL::GL
        glfw
        GLEW::GLEW
        glm::glm          # if found via cmake; otherwise include_dir
)

# GLM might not export a cmake target on older distros
if(NOT TARGET glm::glm)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp)
    target_include_directories(gravity_sim PRIVATE ${GLM_INCLUDE_DIR})
endif()

# macOS: OpenGL is a framework
if(APPLE)
    target_link_libraries(gravity_sim PRIVATE "-framework OpenGL")
endif()

# ── Install (optional) ──
install(TARGETS gravity_sim RUNTIME DESTINATION bin)
