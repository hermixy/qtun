
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)

SET(PROJECT_NAME transproxy)

PROJECT(${PROJECT_NAME})

SET(SOURCE
    main.h
    main.c
    network.h
    network.c
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCE})
