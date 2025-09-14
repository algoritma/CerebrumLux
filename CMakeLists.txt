cmake_minimum_required(VERSION 3.10)

# Proje adını ve C++ dilini belirt
project(CerebrumLux CXX)

# C++ standardını C++17 olarak ayarla (mevcut kodunuz için uygun)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Kaynak dizinlerini tanımla
set(PROJECT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PROJECT_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/tests") # testler src altında varsayılıyor


# --- Ana CerebrumLux Yürütülebilir Dosyası İçin ---
# src dizinindeki tüm .cpp dosyalarını otomatik olarak bul
file(GLOB_RECURSE CEREBRUM_LUX_SOURCES
    "${PROJECT_SRC_DIR}/*.cpp"
    "${PROJECT_SRC_DIR}/brain/*.cpp"
    "${PROJECT_SRC_DIR}/communication/*.cpp"
    "${PROJECT_SRC_DIR}/core/*.cpp"
    "${PROJECT_SRC_DIR}/data_models/*.cpp"
    "${PROJECT_SRC_DIR}/planning_execution/*.cpp"
    "${PROJECT_SRC_DIR}/sensors/*.cpp"
)

# Yürütülebilir dosyayı oluştur
add_executable(CerebrumLux ${CEREBRUM_LUX_SOURCES})

# Başlık dosyası yollarını belirt
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
# Bu test için sadece kendi kaynak dosyası ve gerekli tüm diğer proje kaynak dosyaları linklenmeli.
# Ancak bu sefer "CerebrumLux" kütüphanesine linkleyerek tüm implementasyonları alacağız.
add_executable(test_response_engine ${TEST_MAIN_SOURCE})

# Başlık dosyası yollarını belirt (test yürütülebilir dosyası için)
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

# Test yürütülebilir dosyasını ana projenin yürütülebilir dosyasına linkle
# Bu, testin ana projenin tüm implementasyonlarını (sınıf tanımları, fonksiyonlar vb.) kullanabilmesini sağlar.
# Bu, "multiple definition" hatalarını önlemek için çok önemlidir, çünkü her .cpp ayrı derlenir.
# Ancak bu, "undefined reference" hatalarını çözmek için daha modüler bir yol sunar.
target_link_libraries(test_response_engine CerebrumLux)

# Windows için özel bağlama (genellikle MinGW tarafından otomatik halledilir,
# ancak emin olmak için eklenebilir, şimdilik varsayılan C++ runtime yeterli olacaktır)
# if(WIN32)
#     target_link_libraries(CerebrumLux -lstdc++) # veya gerekli diğer kütüphaneler
# endif()