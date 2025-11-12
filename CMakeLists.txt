cmake_minimum_required(VERSION 3.20)
project(CerebrumLux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Qt6 entegrasyonu
# -----------------------------
find_package(Qt6 COMPONENTS Core Gui Widgets Network Charts REQUIRED)

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

# --- Yeni Kütüphaneler: Eigen3 (vcpkg üzerinden) ---
find_package(Eigen3 CONFIG REQUIRED)          # Eigen3 için vcpkg tarafından sağlanan hedef
find_package(hnswlib CONFIG REQUIRED) # hnswlib için vcpkg tarafından sağlanan hedef

# gumbo kütüphanesini bul (HTML5 parser)
# find_package(gumbo CONFIG REQUIRED) # Gumbo manuel olarak bağlanıyor

# -----------------------------
# Core library
# -----------------------------
file(GLOB_RECURSE CEREBRUMLUX_CORE_SOURCES
    "${PROJECT_SRC_DIR}/brain/*.cpp"
    "${PROJECT_SRC_DIR}/communication/*.cpp"
    "${PROJECT_SRC_DIR}/core/*.cpp"
    "${PROJECT_SRC_DIR}/data_models/*.cpp"
    #"${PROJECT_SRC_DIR}/gui/panels/*.cpp"
    #"${PROJECT_SRC_DIR}/gui/*.cpp"
    "${PROJECT_SRC_DIR}/meta/*.cpp"
    "${PROJECT_SRC_DIR}/planning_execution/*.cpp"
    "${PROJECT_SRC_DIR}/sensors/*.cpp"
    "${PROJECT_SRC_DIR}/user/*.cpp"
    "${PROJECT_SRC_DIR}/learning/*.cpp"
    "${PROJECT_SRC_DIR}/crypto/*.cpp"
    "${PROJECT_SRC_DIR}/swarm_vectordb/*.cpp" # Yeni Swarm-VectorDB modülü eklendi
    "${PROJECT_SRC_DIR}/hnswlib_wrapper.cpp"
)

file(GLOB_RECURSE CEREBRUMLUX_CORE_HEADERS
    "${PROJECT_SRC_DIR}/brain/*.h"
    "${PROJECT_SRC_DIR}/communication/*.h"
    "${PROJECT_SRC_DIR}/core/*.h"
    "${PROJECT_SRC_DIR}/data_models/*.h"
    #"${PROJECT_SRC_DIR}/gui/panels/*.h"
    #"${PROJECT_SRC_DIR}/gui/*.h"
    "${PROJECT_SRC_DIR}/meta/*.h"
    "${PROJECT_SRC_DIR}/planning_execution/*.h"
    "${PROJECT_SRC_DIR}/sensors/*.h"
    "${PROJECT_SRC_DIR}/user/*.h"
    "${PROJECT_SRC_DIR}/learning/*.h"
    "${PROJECT_SRC_DIR}/crypto/*.h"
    "${PROJECT_SRC_DIR}/swarm_vectordb/*.h" # Yeni Swarm-VectorDB modülü eklendi
    "${PROJECT_SRC_DIR}/hnswlib_wrapper.h"
)

add_library(CerebrumLuxCore STATIC ${CEREBRUMLUX_CORE_SOURCES} ${CEREBRUMLUX_CORE_HEADERS})

# --------------------------------------------------
# FastText Entegrasyonu
# --------------------------------------------------
add_library(fasttext STATIC IMPORTED GLOBAL)
set_target_properties(fasttext PROPERTIES
    IMPORTED_LOCATION
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/fasttext/lib/libfasttext.a"
    INTERFACE_INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/fasttext/include"
)

target_include_directories(CerebrumLuxCore PUBLIC
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/brain"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/data_models"
    #"${PROJECT_SRC_DIR}/gui"
    #"${PROJECT_SRC_DIR}/gui/panels"
    # GUI include dizinleri buradan kaldırıldı (Core GUI'den bağımsız olmalı)
    "${PROJECT_SRC_DIR}/meta"
    "${PROJECT_SRC_DIR}/planning_execution"
    "${PROJECT_SRC_DIR}/sensors"
    "${PROJECT_SRC_DIR}/tools"
    "${PROJECT_SRC_DIR}/user"
    "${PROJECT_SRC_DIR}/learning"
    "${PROJECT_SRC_DIR}/crypto"
    "${PROJECT_SRC_DIR}/external" # nlohmann/json.hpp için
    "${PROJECT_SRC_DIR}/swarm_vectordb" # Yeni Swarm-VectorDB modülü include dizini eklendi
    "C:/vcpkg/installed/x64-mingw-static/include" # Gumbo, LMDB ve Eigen için vcpkg include dizini
    "C:/vcpkg/installed/x64-mingw-static/include/hnswlib" # hnswlib başlık dizini
    ${Eigen3_INCLUDE_DIRS} # Eigen3 başlık dizinleri (find_package ile gelir)
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/fasttext/include"
)

target_link_libraries(CerebrumLuxCore PUBLIC
    # WebFetcher'ın QObject, QNetworkAccessManager, QTextDocument gibi bağımlılıkları için gereklidir.
    Qt6::Core
    Qt6::Network
    Qt6::Gui
    OpenSSL::SSL # OpenSSL SSL kütüphanesi
    OpenSSL::Crypto
    gumbo # gumbo kütüphanesini doğrudan bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini tam yolla bağla
    Eigen3::Eigen # Eigen3 kütüphanesi (vcpkg tarafından sağlanan hedef)
    "C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib kütüphanesini CerebrumLuxCore'a statik olarak bağla
    hnswlib::hnswlib # hnswlib kütüphanesini vcpkg hedefi ile bağla (doğrudan dosya yolu kaldırıldı)
    fasttext
    Threads::Threads
    winpthread # winpthread kütüphanesini CerebrumLuxCore'a bağla
    ws2_32 # Windows Sockets
    advapi32 # Advanced API Services
    winmm # Windows Multimedia
)

# gumbo kütüphanesi için bağlayıcı arama dizinini açıkça ekle
target_link_directories(CerebrumLuxCore PUBLIC
    "C:/vcpkg/installed/x64-mingw-static/lib"
    "C:/msys64/mingw64/lib" # MSYS2'den zlib ve winpthread için lib dizini (varsa)
)

# -----------------------------
# GUI executable
# -----------------------------
# Debug aşamasında konsollu çalışsın, Release'te sessiz GUI olsun
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_executable(CerebrumLuxGUI WIN32
        # Ana GUI dosyaları
        "${PROJECT_SRC_DIR}/main.cpp"
        "${PROJECT_SRC_DIR}/gui/MainWindow.cpp"
        "${PROJECT_SRC_DIR}/gui/MainWindow.ui"
        # GUI panel
        "${PROJECT_SRC_DIR}/gui/engine_integration.cpp" # GUI kaynağı buraya taşındı
        "${PROJECT_SRC_DIR}/gui/panels/CapsuleTransferPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/GraphPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/KnowledgeBasePanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/LogPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/SimulationPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/KnowledgeBaseWorker.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/QTablePanel.cpp" # QTablePanel implementasyonu
        "${PROJECT_SRC_DIR}/gui/panels/QTableWorker.cpp"
    )
    target_link_options(CerebrumLuxGUI PRIVATE -Wl,--subsystem,windows)
else()
    add_executable(CerebrumLuxGUI
        # Ana GUI dosyaları
        "${PROJECT_SRC_DIR}/main.cpp"
        "${PROJECT_SRC_DIR}/gui/MainWindow.cpp"
        "${PROJECT_SRC_DIR}/gui/MainWindow.ui"
        # GUI panel
        "${PROJECT_SRC_DIR}/gui/engine_integration.cpp" # GUI kaynağı buraya taşındı
        "${PROJECT_SRC_DIR}/gui/panels/CapsuleTransferPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/GraphPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/KnowledgeBasePanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/LogPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/SimulationPanel.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/KnowledgeBaseWorker.cpp"
        "${PROJECT_SRC_DIR}/gui/panels/QTablePanel.cpp" # QTablePanel implementasyonu
        "${PROJECT_SRC_DIR}/gui/panels/QTableWorker.cpp"
    )
endif()

target_link_libraries(CerebrumLuxGUI PRIVATE
    CerebrumLuxCore
    Qt6::Core
    Qt6::Gui
    #Qt6::Widgets
    Qt6::Network
    Qt6::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
    ws2_32
    crypt32
    gdi32
    version
    "C:/vcpkg/installed/x64-mingw-static/lib/libgumbo.a" # Gumbo'yu GUI'ye bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini GUI'ye bağla
    Eigen3::Eigen # Eigen3 kütüphanesini GUI'ye bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib'i GUI'ye statik olarak bağla
    Threads::Threads
    winpthread # winpthread'i GUI'ye bağla
    advapi32
    winmm
    -Wl,--stack,16777216 # Stack boyutu artışı (16MB) - GUI uygulamaları için faydalı
)

# Qt platform pluginlerini GUI build dizinine kopyala (GUI'nin açılmama sebebini önler)
add_custom_command(TARGET CerebrumLuxGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${Qt6_DIR}/../../../plugins/platforms"
        "$<TARGET_FILE_DIR:CerebrumLuxGUI>/platforms"
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
    "${PROJECT_SRC_DIR}/external" # nlohmann/json.hpp için
    "${PROJECT_SRC_DIR}/swarm_vectordb" # Yeni Swarm-VectorDB modülü include dizini eklendi
    "C:/vcpkg/installed/x64-mingw-static/include" # Gumbo, LMDB ve Eigen için vcpkg include dizini
    "C:/vcpkg/installed/x64-mingw-static/include/hnswlib" # hnswlib başlık dizini
    ${Eigen3_INCLUDE_DIRS} # Eigen3 başlık dizinleri
    "${CMAKE_CURRENT_SOURCE_DIR}/src/external/fasttext/include"
)

# --------------------------------------------------
# Runtime DLL Kopyalama
# --------------------------------------------------
add_custom_command(TARGET CerebrumLuxGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/external/fasttext/lib/libfasttext.dll"
        $<TARGET_FILE_DIR:CerebrumLuxGUI>
    COMMENT "Copying FastText DLL"
)
# --------------------------------------------------
# Thread Desteği
# --------------------------------------------------
find_package(Threads REQUIRED)
# --------------------------------------------------

# -----------------------------
# Importer CLI executable
# -----------------------------
add_executable(knowledge_importer "${PROJECT_SRC_DIR}/tools/KnowledgeImporter.cpp")
# knowledge_importer bir konsol uygulamasıdır, GUI değildir:
set_target_properties(knowledge_importer PROPERTIES WIN32_EXECUTABLE FALSE)
target_link_options(knowledge_importer PRIVATE -mconsole)
target_link_libraries(knowledge_importer PRIVATE
    CerebrumLuxCore # knowledge_importer'ın CerebrumLuxCore'a bağlı olduğunu belirt
    Qt6::Core # KnowledgeImporter'ın kendisi Qt Core kullanıyorsa (Logger için)
    OpenSSL::SSL
    OpenSSL::Crypto
    "C:/vcpkg/installed/x64-mingw-static/lib/libgumbo.a" # Gumbo'yu importer'a bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini importer'a bağla
    Eigen3::Eigen # Eigen3 kütüphanesini importer'a bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib'i importer'a statik olarak bağla
    hnswlib::hnswlib # hnswlib kütüphanesini vcpkg hedefi ile bağla (doğrudan dosya yolu kaldırıldı)
    winpthread # winpthread'i importer'a bağla
    ws2_32
    crypt32
    gdi32
    version
    advapi32
    winmm
)

target_include_directories(knowledge_importer PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/crypto"
    "${PROJECT_SRC_DIR}/external" # nlohmann/json.hpp için gerekli
    "${PROJECT_SRC_DIR}/learning" # Capsule için
    "${PROJECT_SRC_DIR}/swarm_vectordb" # SwarmVectorDB DataModels için
    "C:/vcpkg/installed/x64-mingw-static/include" # Gumbo, LMDB ve Eigen için vcpkg include dizini
    "C:/vcpkg/installed/x64-mingw-static/include/hnswlib" # hnswlib başlık dizini
    ${Eigen3_INCLUDE_DIRS} # Eigen3 başlık dizinleri
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
    winpthread
    "C:/vcpkg/installed/x64-mingw-static/lib/libgumbo.a" # Gumbo'yu teste bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini teste bağla
    Eigen3::Eigen # Eigen3 kütüphanesini teste bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib'i teste statik olarak bağla
    #hnswlib::hnswlib # hnswlib kütüphanesini vcpkg hedefi ile bağla
    advapi32
    winmm
    # PROBLEMLİ BAYRAKLAR KALDIRILDI: mingw32, stdc++, gcc, -Wl,--enable-stdcall-fixup
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
    "C:/vcpkg/installed/x64-mingw-static/lib/libgumbo.a" # Gumbo'yu teste bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini teste bağla
    Eigen3::Eigen # Eigen3 kütüphanesini teste bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib'i teste statik olarak bağla
    hnswlib::hnswlib # hnswlib kütüphanesini vcpkg hedefi ile bağla
    ws2_32
    crypt32
    gdi32
    version
    winpthread
    advapi32
    winmm
    # PROBLEMLİ BAYRAKLAR KALDIRILDI: mingw32, stdc++, gcc, -Wl,--enable-stdcall-fixup
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
    "C:/vcpkg/installed/x64-mingw-static/lib/libgumbo.a" # Gumbo'yu eğiticisine bağla
    "C:/vcpkg/installed/x64-mingw-static/lib/liblmdb.a" # LMDB kütüphanesini eğiticisine bağla
    Eigen3::Eigen # Eigen3 kütüphanesini eğiticisine bağla
    #"C:/vcpkg/installed/x64-mingw-static/lib/libzlib.a" # Zlib'i eğiticisine statik olarak bağla
    hnswlib::hnswlib # hnswlib kütüphanesini vcpkg hedefi ile bağla
    ws2_32
    crypt32
    gdi32
    version
    winpthread
    advapi32
    winmm
    # PROBLEMLİ BAYRAKLAR KALDIRILDI: mingw32, stdc++, gcc, -Wl,--enable-stdcall-fixup
)

target_include_directories(nlp_online_trainer PRIVATE
    "${PROJECT_SRC_DIR}"
    "${PROJECT_SRC_DIR}/communication"
    "${PROJECT_SRC_DIR}/core"
    "${PROJECT_SRC_DIR}/external"
    "${PROJECT_SRC_DIR}/crypto"
)