cmake_minimum_required(VERSION 3.17)
project(assign2 C)

set(CMAKE_C_STANDARD 99)

add_executable(assign2
  buffer_mgr.c buffer_mgr.h dt.h
  buffer_mgr_stat.c buffer_mgr_stat.h
  storage_mgr.c storage_mgr.h
  dberror.c dberror.h
  test_assign2_1.c test_helper.h) # or test_assign2_2.c
