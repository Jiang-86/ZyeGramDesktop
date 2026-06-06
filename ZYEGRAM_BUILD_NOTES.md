# ZyeGram Build Notes

ZyeGram v1.0 Build 2026.06 is a modified Windows x64 build derived from
AyuGram Desktop / Telegram Desktop.

## Release Package

Release artifact:

`ZyeGram-v1.0-build-2026.06-win64.zip`

SHA256:

`DA6008140E7399D13970E2E66B42FA77FD7864811C083C0D35C914D87A943783`

## License

This project is distributed under GNU GPL v3 with the OpenSSL linking
exception inherited from Telegram Desktop. Keep `LICENSE` and `LEGAL`
with binary releases and provide the corresponding source for the exact
published build.

## Submodule Build Patch

The local Windows build used two small changes in the `cmake` submodule
(`desktop-app/cmake_helpers`) to make MSVC builds more stable on this
machine.

Apply this patch inside the `cmake` submodule if you need to reproduce
the same build environment:

```diff
diff --git a/options_win.cmake b/options_win.cmake
index c2d66cf..1e5ed05 100644
--- a/options_win.cmake
+++ b/options_win.cmake
@@ -31,7 +31,8 @@ if (MSVC)
         # /Qspectre
         /utf-8
         /W4
-        /MP     # Enable multi process build.
+        /MP2    # Enable multi process build with a conservative process count.
+        /FS
         /EHsc   # Catch C++ exceptions only, extern C functions never throw a C++ exception.
         /w15038 # wrong initialization order
         /w14265 # class has virtual functions, but destructor is not virtual
@@ -64,7 +65,7 @@ if (MSVC)
     INTERFACE
         $<$<CONFIG:Debug>:/NODEFAULTLIB:LIBCMT>
         $<$<AND:$<CONFIG:Debug>,$<BOOL:${build_win64}>>:/DEBUG:FASTLINK>
-        $<$<NOT:$<AND:$<CONFIG:Debug>,$<BOOL:${build_win64}>>>:$<IF:$<STREQUAL:$<GENEX_EVAL:$<TARGET_PROPERTY:MSVC_DEBUG_INFORMATION_FORMAT>>,ProgramDatabase>,/DEBUG,/DEBUG:NONE>>
+        $<$<NOT:$<AND:$<CONFIG:Debug>,$<BOOL:${build_win64}>>>:$<IF:$<BOOL:$<GENEX_EVAL:$<TARGET_PROPERTY:MSVC_DEBUG_INFORMATION_FORMAT>>>,/DEBUG,/DEBUG:NONE>>
         $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>
         /INCREMENTAL:NO
         /DEPENDENTLOADFLAG:0x800
diff --git a/variables.cmake b/variables.cmake
index d6ac6c5..b2f492a 100644
--- a/variables.cmake
+++ b/variables.cmake
@@ -21,7 +21,9 @@ if (DESKTOP_APP_SPECIAL_TARGET STREQUAL ""
 endif()
 
 set(CMAKE_CXX_SCAN_FOR_MODULES OFF CACHE BOOL "")
-set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase" CACHE STRING "")
+set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
+    "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>$<$<CONFIG:Release,MinSizeRel>:Embedded>"
+    CACHE STRING "" FORCE)
 set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "")
 option(DESKTOP_APP_TEST_APPS "Build test apps, development only." OFF)
 option(DESKTOP_APP_LOTTIE_DISABLE_RECOLORING "Disable recoloring of lottie animations." OFF)
```
