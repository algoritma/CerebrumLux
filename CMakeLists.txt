cmake_minimum_required(VERSION 3.20)
project(CerebrumLux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Qt6 entegrasyonu
# -----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Network)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# -----------------------------
# OpenSSL entegrasyonu (vcpkg static)
# -----------------------------
find_package(OpenSSL REQUIRED)

set(VCPKG_OPENSSL_INCLUDE_DIR "C:/vcpkg/installed/x64-mingw-static/include")
set(VCPKG_OPENSSL_LIBRARIES
    "C:/vcpkg/installed/x64-mingw-static/lib/libssl.a"
    "C:/vcpkg/installed/x64-mingw-static/lib/libcrypto.a"
)

# -----------------------------
# Core library
# -----------------------------
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
    "${PROJECT_SRC_DIR}/user/*.cpp"
    "${PROJECT_SRC_DIR}/learning/*.cpp"
    "${PROJECT_SRC_DIR}/gui/panels/CapsuleTransferPanel.cpp"
    "${PROJECT_SRC_DIR}/learning/UnicodeSanitizer.cpp"
    "${PROJECT_SRC_DIR}/learning/StegoDetector.cpp"
)

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
    "${PROJECT_SRC_DIR}/learning"
    "${PROJECT_SRC_DIR}/external"
    "${VCPKG_OPENSSL_INCLUDE_DIR}"
)

target_link_libraries(CerebrumLuxCore PUBLIC
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Charts Qt6::Network
    OpenSSL::SSL OpenSSL::Crypto
)

# -----------------------------
# GUI executable
# -----------------------------
add_executable(CerebrumLuxGUI WIN32
    "${PROJECT_SRC_DIR}/main.cpp"
)

target_link_libraries(CerebrumLuxGUI PRIVATE
    CerebrumLuxCore
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Charts Qt6::Network
    OpenSSL::SSL OpenSSL::Crypto
    ws2_32 crypt32 gdi32 version mingw32
)

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
    "${PROJECT_SRC_DIR}/external"
    "${VCPKG_OPENSSL_INCLUDE_DIR}"
)

# -----------------------------
# Test executable
# -----------------------------
file(GLOB TEST_MAIN_SOURCE "${PROJECT_TESTS_DIR}/test_response_engine.cpp")
add_executable(test_response_engine ${TEST_MAIN_SOURCE})

target_link_libraries(test_response_engine PRIVATE 
    CerebrumLuxCore 
    OpenSSL::SSL OpenSSL::Crypto
    ws2_32 crypt32 gdi32 version mingw32 stdc++ gcc winpthread
    -Wl,--enable-stdcall-fixup
)

target_include_directories(test_response_engine PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_TESTS_DIR}"
    "${VCPKG_OPENSSL_INCLUDE_DIR}"
)

# -----------------------------
# NLP trainer executable
# -----------------------------
add_executable(nlp_online_trainer "${PROJECT_SRC_DIR}/tools/nlp_online_trainer.cpp")

target_link_libraries(nlp_online_trainer PRIVATE 
    CerebrumLuxCore 
    OpenSSL::SSL OpenSSL::Crypto
    ws2_32 crypt32 gdi32 version mingw32 stdc++ gcc winpthread
    -Wl,--enable-stdcall-fixup
)

target_include_directories(nlp_online_trainer PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/communication" 
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/external"
    "${VCPKG_OPENSSL_INCLUDE_DIR}"
)
