cmake_minimum_required(VERSION 3.10)

# Proje adını ve C++ dilini belirt
project(CerebrumLux CXX)

# C++ standardını C++17 olarak ayarla
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Kaynak dizinlerini tanımla
set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests") # testler proje kök dizininin altında

# --- CerebrumLuxCore Statik Kütüphanesi İçin ---
# src dizinindeki tüm .cpp dosyalarını (main.cpp hariç) otomatik olarak bul
file(GLOB_RECURSE CEREBRUML_LUX_CORE_SOURCES
    "${PROJECT_SRC_DIR}/brain/*.cpp"
    "${PROJECT_SRC_DIR}/communication/*.cpp"
    "${PROJECT_SRC_DIR}/core/*.cpp"
    "${PROJECT_SRC_DIR}/data_models/*.cpp"
    "${PROJECT_SRC_DIR}/planning_execution/*.cpp"
    "${PROJECT_SRC_DIR}/sensors/*.cpp"
)

# Statik kütüphaneyi oluştur
add_library(CerebrumLuxCore STATIC ${CEREBRUML_LUX_CORE_SOURCES})

# Kütüphane için başlık dosyası yollarını belirt (PUBLIC, çünkü diğer hedefler buna bağlanacak)
target_include_directories(CerebrumLuxCore PUBLIC
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
)

# --- Ana CerebrumLux Yürütülebilir Dosyası İçin ---
# Sadece main.cpp'yi yürütülebilir dosya olarak ekle
add_executable(CerebrumLux "${PROJECT_SRC_DIR}/main.cpp")

# CerebrumLux yürütülebilir dosyasını CerebrumLuxCore kütüphanesine bağla
target_link_libraries(CerebrumLux PRIVATE CerebrumLuxCore)

# CerebrumLux yürütülebilir dosyası için başlık dosyası yollarını belirt
target_include_directories(CerebrumLux PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
)

# --- test_response_engine Yürütülebilir Dosyası İçin ---
# test_response_engine.cpp dosyasını bul
file(GLOB TEST_MAIN_SOURCE "${PROJECT_TESTS_DIR}/test_response_engine.cpp")

# Test yürütülebilir dosyasını oluştur
add_executable(test_response_engine ${TEST_MAIN_SOURCE})

# Test yürütülebilir dosyasını CerebrumLuxCore kütüphanesine bağla
target_link_libraries(test_response_engine PRIVATE CerebrumLuxCore)

# Test yürütülebilir dosyası için başlık dosyası yollarını belirt
target_include_directories(test_response_engine PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
    "${PROJECT_TESTS_DIR}" # Testlerin kendi başlıklarına erişimi için
)

# Windows için özel bağlama (eğer gerekliyse)
# if(WIN32)
#     target_link_libraries(CerebrumLux -lstdc++) # veya gerekli diğer kütüphaneler
# endif()
