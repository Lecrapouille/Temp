##=====================================================================
## OpenGLCppWrapper: A C++20 OpenGL wrapper.
## Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of OpenGLCppWrapper.
##
## OpenGLCppWrapper is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## OpenGLCppWrapper is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
include $(M)/project/Makefile

###################################################
# Compile shared and static libraries
#
# Exactly one GPU backend is compiled. Selecting another one is a matter of
# changing GPU_BACKEND in Makefile.common.
#
LIB_FILES := $(filter-out src/GPU/Backends/%,$(call rwildcard,src,*.cpp))
LIB_FILES += $(call rwildcard,src/GPU/Backends/$(GPU_BACKEND),*.cpp)
DEFINES += -DGPU_BACKEND_$(GPU_BACKEND)
INCLUDES := $(P)/include $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/units/include
INCLUDES += $(THIRD_PARTIES_DIR)/stb
INCLUDES += $(THIRD_PARTIES_DIR)/cgltf
INCLUDES += $(THIRD_PARTIES_DIR)/json/include
INCLUDES += $(P)/src/GPU/Backends/$(GPU_BACKEND)/glad/include
VPATH := $(P)/src

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
post-build:: build-examples

.PHONY: build-examples
build-examples: $(TARGET_STATIC_LIB_NAME)
	$(Q)$(MAKE) --no-print-directory --directory=examples all
