cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

include (../../tests.cmake)

project (karabiner_test)

add_executable(
  karabiner_test
  src/elecom/trackball_test.cpp
)

add_executable(
  karabiner_corsair_k95_test
  src/corsair/k95_test.cpp
)
