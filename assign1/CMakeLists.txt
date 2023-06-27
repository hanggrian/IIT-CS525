cmake_minimum_required(VERSION 3.17)
project(assign1)

set(CMAKE_CXX_STANDARD 20)

add_executable(assign1
  storage_mgr.cpp storage_mgr.h
  dberror.cpp dberror.h
  test_assign1_1.cpp test_helpers.h)
