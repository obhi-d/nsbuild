if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(nsbuild VERSION 1.0.0 LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.25)

include(FetchContent)

FetchContent_Declare(
	nsbuild
	GIT_REPOSITORY "https://github.com/obhi-d/nsbuild.git"
	GIT_TAG "origin/main"
)

FetchContent_MakeAvailable(nsbuild)
