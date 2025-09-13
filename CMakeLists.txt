cmake_minimum_required(VERSION 3.10)

# Proje adını ve C++ dilini belirt
project(CerebrumLux CXX)

# C++ standardını C++17 olarak ayarla (mevcut kodunuz için uygun)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Tüm kaynak dosyalarını tanımla (ana yürütülebilir dosya için)
set(SOURCE_FILES
    src/main.cpp
    src/brain/autoencoder.cpp
    src/brain/cryptofig_processor.cpp
    src/brain/intent_analyzer.cpp
    src/brain/intent_learner.cpp
    src/brain/intent_template.cpp
    src/brain/prediction_engine.cpp
    src/communication/ai_insights_engine.cpp
    src/communication/response_engine.cpp
    src/communication/suggestion_engine.cpp
    src/core/utils.cpp
    src/core/logger.cpp
    src/data_models/dynamic_sequence.cpp
    src/data_models/sequence_manager.cpp
    src/planning_execution/goal_manager.cpp
    src/planning_execution/planner.cpp
    src/sensors/atomic_signal.cpp
    src/sensors/simulated_processor.cpp
)

# Yürütülebilir dosyayı oluştur
add_executable(CerebrumLux ${SOURCE_FILES})

# Başlık dosyası yollarını belirt (ana yürütülebilir dosya için)
target_include_directories(CerebrumLux PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/brain
    ${CMAKE_CURRENT_SOURCE_DIR}/src/communication
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/planning_execution
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sensors
)

# Test yürütülebilir dosyası için kaynak dosyası
set(TEST_SOURCE_FILES
    tests/test_response_engine.cpp
)

# Test yürütülebilir dosyasını oluştur
add_executable(test_response_engine ${TEST_SOURCE_FILES} ${SOURCE_FILES})

# Başlık dosyası yollarını belirt (test yürütülebilir dosyası için)
target_include_directories(test_response_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/brain
    ${CMAKE_CURRENT_SOURCE_DIR}/src/communication
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data_models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/planning_execution
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sensors
    ${CMAKE_CURRENT_SOURCE_DIR}/tests # Include the tests directory itself for test-specific headers if any
)

# Test yürütülebilir dosyasını ana projenin kütüphanelerine bağla
# Bu, testin ana projenin sınıflarını ve fonksiyonlarını kullanabilmesini sağlar.
# target_link_libraries(test_response_engine CerebrumLux) # REMOVED

# Windows için özel bağlama (genellikle MinGW tarafından otomatik halledilir,
# ancak emin olmak için eklenebilir, şimdilik varsayılan C++ runtime yeterli olacaktır)
# if(WIN32)
#     target_link_libraries(CerebrumLux -lstdc++) # veya gerekli diğer kütüphaneler
# endif()