cmake_minimum_required(VERSION 3.20)
project(CerebrumLux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 bul ve gerekli modülleri dahil et
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

# Otomatik MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Kaynak ve header dosyalarını tanımla
set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

file(GLOB_RECURSE CEREBRUML_LUX_CORE_SOURCES
    "${PROJECT_SRC_DIR}/brain/*.cpp"
    "${PROJECT_SRC_DIR}/communication/*.cpp"
    "${PROJECT_SRC_DIR}/core/*.cpp"
    "${PROJECT_SRC_DIR}/data_models/*.cpp"
    "${PROJECT_SRC_DIR}/gui/*.cpp"
    "${PROJECT_SRC_DIR}/gui/panels/*.cpp"
    "${PROJECT_SRC_DIR}/meta/*.cpp"
    "${PROJECT_SRC_DIR}/planning_execution/*.cpp"
    "${PROJECT_SRC_DIR}/sensors/*.cpp"
    "${PROJECT_SRC_DIR}/tools/*.cpp"
    "${PROJECT_SRC_DIR}/user/*.cpp"
    "${PROJECT_SRC_DIR}/learning/*.cpp" # YENİ: LearningModule ve KnowledgeBase için
)

# CerebrumLuxCore kütüphanesini oluştur
add_library(CerebrumLuxCore STATIC ${CEREBRUML_LUX_CORE_SOURCES})

target_include_directories(CerebrumLuxCore PUBLIC
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/gui"
    "${PROJECT_SRC_DIR}/gui/panels"
    "${PROJECT_SRC_DIR}/meta"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
    "${PROJECT_SRC_DIR}/tools"
    "${PROJECT_SRC_DIR}/user"
    "${PROJECT_SRC_DIR}/learning"  # YENİ: LearningModule ve KnowledgeBase için
    "${PROJECT_SRC_DIR}/external" # YENİ: nlohmann/json.hpp için
)

# Qt6 ve Charts ile linkle
target_link_libraries(CerebrumLuxCore PUBLIC Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Charts)

# Ana yürütülebilir dosya
add_executable(CerebrumLuxGUI "${PROJECT_SRC_DIR}/main.cpp")
target_link_libraries(CerebrumLuxGUI PRIVATE CerebrumLuxCore)

target_include_directories(CerebrumLuxGUI PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/gui"
    "${PROJECT_SRC_DIR}/gui/panels"
    "${PROJECT_SRC_DIR}/meta"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
    "${PROJECT_SRC_DIR}/tools"
    "${PROJECT_SRC_DIR}/user"
    "${PROJECT_SRC_DIR}/learning"  # YENİ: LearningModule ve KnowledgeBase için
    "${PROJECT_SRC_DIR}/external" # YENİ: nlohmann/json.hpp için
)

# Test yürütülebilir dosyası
file(GLOB TEST_MAIN_SOURCE "${PROJECT_TESTS_DIR}/test_response_engine.cpp")
add_executable(test_response_engine ${TEST_MAIN_SOURCE})
target_link_libraries(test_response_engine PRIVATE CerebrumLuxCore)

target_include_directories(test_response_engine PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/gui"
    "${PROJECT_SRC_DIR}/gui/panels"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
    "${PROJECT_SRC_DIR}/tools"
    "${PROJECT_SRC_DIR}/user"
    "${PROJECT_TESTS_DIR}"
    "${PROJECT_SRC_DIR}/learning"  # YENİ: LearningModule ve KnowledgeBase için
    "${PROJECT_SRC_DIR}/external" # YENİ: nlohmann/json.hpp için
)