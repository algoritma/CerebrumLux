cmake_minimum_required(VERSION 3.20)
project(CerebrumLux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Qt6 entegrasyonu
# -----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Charts REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Projenin dizinlerini tanımla
set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# -----------------------------
# OpenSSL entegrasyonu (vcpkg static)
# -----------------------------
find_package(OpenSSL REQUIRED) # OpenSSL kütüphanelerini bul

# gumbo kütüphanesini bul (HTML5 parser)
#find_package(gumbo CONFIG REQUIRED)

# -----------------------------
# Core library
# -----------------------------
file(GLOB_RECURSE CEREBRUMLUX_CORE_SOURCES
    "${PROJECT_SRC_DIR}/brain/*.cpp"
    "${PROJECT_SRC_DIR}/communication/*.cpp"
    "${PROJECT_SRC_DIR}/core/*.cpp"
    "${PROJECT_SRC_DIR}/data_models/*.cpp"
    "${PROJECT_SRC_DIR}/gui/panels/*.cpp"
    "${PROJECT_SRC_DIR}/gui/engine_integration.cpp"
    "${PROJECT_SRC_DIR}/meta/*.cpp"
    "${PROJECT_SRC_DIR}/planning_execution/*.cpp"
    "${PROJECT_SRC_DIR}/sensors/*.cpp"
    "${PROJECT_SRC_DIR}/user/*.cpp"
    "${PROJECT_SRC_DIR}/learning/*.cpp"
    "${PROJECT_SRC_DIR}/crypto/*.cpp"
)

file(GLOB_RECURSE CEREBRUMLUX_CORE_HEADERS
    "${PROJECT_SRC_DIR}/brain/*.h"
    "${PROJECT_SRC_DIR}/communication/*.h"
    "${PROJECT_SRC_DIR}/core/*.h"
    "${PROJECT_SRC_DIR}/data_models/*.h"
    "${PROJECT_SRC_DIR}/gui/panels/*.h"
    "${PROJECT_SRC_DIR}/gui/engine_integration.h"
    "${PROJECT_SRC_DIR}/meta/*.h"
    "${PROJECT_SRC_DIR}/planning_execution/*.h"
    "${PROJECT_SRC_DIR}/sensors/*.h"
    "${PROJECT_SRC_DIR}/user/*.h"
    "${PROJECT_SRC_DIR}/learning/*.h"
    "${PROJECT_SRC_DIR}/crypto/*.h"
)

add_library(CerebrumLuxCore STATIC ${CEREBRUMLUX_CORE_SOURCES} ${CEREBRUMLUX_CORE_HEADERS})

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
    "${PROJECT_SRC_DIR}/learning"
    "${PROJECT_SRC_DIR}/crypto"
    "${PROJECT_SRC_DIR}/external"
)

target_link_libraries(CerebrumLuxCore PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts # Charts modülü
    OpenSSL::SSL # OpenSSL SSL kütüphanesi
    OpenSSL::Crypto
    gumbo # gumbo kütüphanesini doğrudan bağla
)

# gumbo kütüphanesi için bağlayıcı arama dizinini açıkça ekle
target_link_directories(CerebrumLuxCore PUBLIC
    "C:/vcpkg/installed/x64-mingw-static/lib"
)

# -----------------------------
# GUI executable
# -----------------------------
add_executable(CerebrumLuxGUI WIN32
    "${PROJECT_SRC_DIR}/main.cpp"
    "${PROJECT_SRC_DIR}/gui/MainWindow.cpp"
    "${PROJECT_SRC_DIR}/gui/MainWindow.ui"
)

target_link_options(CerebrumLuxGUI PRIVATE -Wl,--subsystem,windows)

target_link_libraries(CerebrumLuxGUI PRIVATE
    CerebrumLuxCore
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
    crypt32
    gdi32
    version
)

# CerebrumLuxGUI'nin kendi include dizinleri (gerekliyse)
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
    "${PROJECT_SRC_DIR}/learning"
    "${PROJECT_SRC_DIR}/crypto"
    "${PROJECT_SRC_DIR}/external"
    #${gumbo_INCLUDE_DIRS} # gumbo başlık dizinlerini ekle
    "C:/vcpkg/installed/x64-mingw-static/include" # gumbo başlık dizinini manuel ekle

)

# -----------------------------
# Test executable (test_response_engine)
# -----------------------------
file(GLOB TEST_RESPONSE_ENGINE_SOURCE "${PROJECT_TESTS_DIR}/test_response_engine.cpp")
add_executable(test_response_engine ${TEST_RESPONSE_ENGINE_SOURCE})

target_link_libraries(test_response_engine PRIVATE
    CerebrumLuxCore
    Qt6::Core # Bazı testler Qt Core'a ihtiyaç duyabilir
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
    crypt32
    gdi32
    version
    mingw32
    stdc++
    gcc
    winpthread
    -Wl,--enable-stdcall-fixup
)

target_include_directories(test_response_engine PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_TESTS_DIR}"
    "${PROJECT_SRC_DIR}/crypto"
)

# -----------------------------
# Test executable (test_learning_module) - YENİ EKLENDİ
# -----------------------------
file(GLOB TEST_LEARNING_MODULE_SOURCE "${PROJECT_TESTS_DIR}/test_learning_module.cpp")
add_executable(test_learning_module ${TEST_LEARNING_MODULE_SOURCE})

target_link_libraries(test_learning_module PRIVATE
    CerebrumLuxCore
    Qt6::Core # Bazı testler Qt Core'a ihtiyaç duyabilir
    OpenSSL::SSL
    OpenSSL::Crypto
    # nlohmann::json gibi kütüphaneleri de buraya eklemeniz gerekebilir.
    # eğer test_learning_module'de json kullanılıyorsa
    ws2_32
    crypt32
    gdi32
    version
    mingw32
    stdc++
    gcc
    winpthread
    -Wl,--enable-stdcall-fixup
)

target_include_directories(test_learning_module PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_TESTS_DIR}"
    "${PROJECT_SRC_DIR}/crypto"
    "${PROJECT_SRC_DIR}/external" # nlohmann/json.hpp için gerekli
)

# -----------------------------
# NLP trainer executable
# -----------------------------
add_executable(nlp_online_trainer "${PROJECT_SRC_DIR}/tools/nlp_online_trainer.cpp")

target_link_libraries(nlp_online_trainer PRIVATE
    CerebrumLuxCore
    Qt6::Core # Bazı testler Qt Core'a ihtiyaç duyabilir
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
    crypt32
    gdi32
    version
    mingw32
    stdc++
    gcc
    winpthread
    -Wl,--enable-stdcall-fixup
)

target_include_directories(nlp_online_trainer PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/external"
    "${PROJECT_SRC_DIR}/crypto"
)