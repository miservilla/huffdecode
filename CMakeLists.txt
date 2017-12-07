cmake_minimum_required(VERSION 3.9)
project(huffdecode C)

set(CMAKE_C_STANDARD 90)

add_executable(huffdecode main.c huffman.c)