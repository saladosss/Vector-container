# Add source to this project's executable.
add_executable (Vector  "vector.h" "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Vector PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
