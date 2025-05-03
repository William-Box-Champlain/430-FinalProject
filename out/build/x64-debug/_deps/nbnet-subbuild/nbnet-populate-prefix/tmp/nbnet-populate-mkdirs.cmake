# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-src"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-build"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/tmp"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/src/nbnet-populate-stamp"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/src"
  "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/src/nbnet-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/src/nbnet-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Will/source/repos/430-FinalProject/out/build/x64-debug/_deps/nbnet-subbuild/nbnet-populate-prefix/src/nbnet-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
