cmake_minimum_required(VERSION 3.16)
project(QK4FrequencyEntryTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

qt_add_executable(test_frequencyentryparser
    test_frequencyentryparser.cpp
    ../src/ui/frequencyentryparser.cpp
    ../src/ui/frequencyentryparser.h
)
target_include_directories(test_frequencyentryparser PRIVATE ../src)
target_link_libraries(test_frequencyentryparser PRIVATE Qt6::Core Qt6::Test)

enable_testing()
add_test(NAME frequencyentryparser COMMAND test_frequencyentryparser)

qt_add_executable(test_frequencydisplaywidget
    test_frequencydisplaywidget.cpp
    ../src/ui/frequencydisplaywidget.cpp
    ../src/ui/frequencydisplaywidget.h
    ../src/ui/wheelaccumulator.cpp
    ../src/ui/wheelaccumulator.h
    ../src/ui/k4styles.cpp
    ../src/ui/k4styles.h
)
target_include_directories(test_frequencydisplaywidget PRIVATE ../src)
target_link_libraries(test_frequencydisplaywidget PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME frequencydisplaywidget COMMAND test_frequencydisplaywidget)

cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH compiler_runtime_dir)
set_tests_properties(frequencyentryparser frequencydisplaywidget PROPERTIES
    ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
        "PATH=path_list_prepend:${compiler_runtime_dir}"
        "QT_QPA_PLATFORM=set:offscreen"
)
