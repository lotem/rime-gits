#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

EXTRA_DIST = CMakeLists.txt

INCLUDES		= -I$(top_builddir) \
			  -I$(top_srcdir)

AM_CPPFLAGS		= $(PREDEFINED_MACROS)
AM_CXXFLAGS		= $(DEFAULT_COMPILE_FLAGS)

LDADD			= $(top_builddir)/unittest/libgtest.la \
			  $(top_builddir)/ggadget/libggadget@GGL_EPOCH@.la

check_PROGRAMS		= default_options_test

default_options_test_SOURCES = default_options_test.cc

TESTS_ENVIRONMENT	= $(SHELL)
TESTS			= ${check_PROGRAMS}
