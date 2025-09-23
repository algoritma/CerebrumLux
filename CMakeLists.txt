cmake_minimum_required(VERSION 3.20)

# Düzeltme: Derleyici yolları doğrudan burada ayarlandı, emin olmak için
set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")

project(CerebrumLux LANGUAGES CXX)

# -----------------------------
# Qt6 config dosyalarının yolu
# -----------------------------
# Düzeltme: CMAKE_PREFIX_PATH'i Qt'nin lib/cmake dizinine ayarla
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/mingw_64/lib/cmake" CACHE PATH "Path to Qt6 CMake configuration files")

# -----------------------------
# C++ standardı
# -----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Qt6 modüllerini bul
# -----------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# -----------------------------
# Proje kaynak dizinleri
# -----------------------------
set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")

# -----------------------------
# OpenSSL entegrasyonu (Lokal Kurulum)
# -----------------------------
# Düzeltme: OpenSSL kök dizini belirtildi. Include ve Library yolları otomatik bulunacak.
# Kendi OpenSSL kurulumunuzdaki "OpenSSL" klasörünün kök dizini (C:/OpenSSL-Win64 gibi)
set(OPENSSL_ROOT_DIR "C:/msys64/mingw64" CACHE PATH "Path to OpenSSL installation root")

# find_package OpenSSL'i bulmasını sağlayın
find_package(OpenSSL REQUIRED)
# OpenSSL_INCLUDE_DIR ve OpenSSL_LIBRARIES otomatik olarak bulunacaktır.

# -----------------------------
# Core kütüphane kaynakları
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
    "${OpenSSL_INCLUDE_DIR}" # find_package ile bulunan include yolu
)

target_link_libraries(CerebrumLuxCore PUBLIC
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Charts
    OpenSSL::SSL OpenSSL::Crypto # find_package ile bulunan OpenSSL hedefleri
)

# -----------------------------
# GUI uygulaması (WIN32 = konsolsuz)
# -----------------------------
add_executable(CerebrumLuxGUI WIN32 "${PROJECT_SRC_DIR}/main.cpp")

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
    "${OpenSSL_INCLUDE_DIR}"
)

# Düzeltme: CerebrumLuxGUI için linkleme listesi ve flag'leri
target_link_libraries(CerebrumLuxGUI PRIVATE
    CerebrumLuxCore
    Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Charts # Qt kütüphaneleri
    OpenSSL::SSL OpenSSL::Crypto               # OpenSSL kütüphaneleri
)

# --- MinGW özel linker flags (WIN32 executable için) ---
# Düzeltme: Bağlantı seçenekleri target_link_options ile ekleniyor.
# Bu, __imp___argc sorununu çözmek için en uygun yöntemdir.
if(MINGW)
    target_link_options(CerebrumLuxGUI PRIVATE
        "-Wl,--subsystem,windows" # Uygulamanın bir GUI uygulaması olduğunu açıkça belirtir
        "-lmingw32"               # MinGW runtime kütüphanesi (kritik)
        "-lstdc++"                # C++ standart kütüphanesi
        "-lgcc"                   # GCC destek kütüphanesi
        "-lwinpthread"            # pthreads desteği
    )
endif()

# -----------------------------
# Test executable
# -----------------------------
file(GLOB TEST_MAIN_SOURCE "${PROJECT_TESTS_DIR}/test_response_engine.cpp")
add_executable(test_response_engine ${TEST_MAIN_SOURCE})
target_link_libraries(test_response_engine PRIVATE CerebrumLuxCore OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(test_response_engine PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_TESTS_DIR}"
)

# -----------------------------
# NLP Online Trainer
# -----------------------------
add_executable(nlp_online_trainer "${PROJECT_SRC_DIR}/tools/nlp_online_trainer.cpp")
target_link_libraries(nlp_online_trainer PRIVATE CerebrumLuxCore OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(nlp_online_trainer PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/communication" 
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/external"
    "${OpenSSL_INCLUDE_DIR}"
)