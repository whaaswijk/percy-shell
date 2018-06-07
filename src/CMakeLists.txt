if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-function")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_NAUTY -DDISABLE_GLUCOSE")
endif()

add_executable(percy-shell main.cpp)
target_link_libraries(percy-shell percy alice libcryptominisat5)
