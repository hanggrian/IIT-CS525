cmake_minimum_required(VERSION 3.17)
project(assign3 C)

set(CMAKE_C_STANDARD 99)

add_executable(assign3
  buffer_mgr_stat.c buffer_mgr_stat.h
  buffer_mgr.c buffer_mgr.h
  dberror.c dberror.h
  dt.h
  expr.h expr.c
  record_mgr.c record_mgr.h
  rm_serializer.c rm_serializer.h
  storage_mgr.c storage_mgr.h
  tables.h
  interactive.c test_helper.h) # or test_expr.c
