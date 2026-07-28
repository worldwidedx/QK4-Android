set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # host tools (moc, qsb)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)    # arm64 libs only
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)    # arm64 headers only
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)    # arm64 cmake configs only

# Qt6 host tool discovery
set(QT_HOST_PATH "/usr" CACHE PATH "")
set(QT_HOST_PATH_CMAKE_DIR "/usr/lib/x86_64-linux-gnu/cmake" CACHE PATH "")

# Debian Bug #1010049: explicit host tool package dirs
set(Qt6HostInfo_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6HostInfo" CACHE PATH "")
set(Qt6CoreTools_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6CoreTools" CACHE PATH "")
set(Qt6GuiTools_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6GuiTools" CACHE PATH "")
set(Qt6WidgetsTools_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6WidgetsTools" CACHE PATH "")
set(Qt6DBusTools_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6DBusTools" CACHE PATH "")
set(Qt6ShaderToolsTools_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6ShaderToolsTools" CACHE PATH "")
