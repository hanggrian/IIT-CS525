cmake_minimum_required(VERSION 3.17)
project(assign1 C)

set(CMAKE_C_STANDARD 99)

add_executable(assign1
  storage_mgr.c storage_mgr.h
  dberror.c dberror.h
  test_assign1_1.c test_helper.h)
