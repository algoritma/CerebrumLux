# ai_tutor/CMakeLists.txt
add_library(ai_tutor
    curriculum.h
    curriculum.cpp
    llama_adapter.cpp
    teacher_evaluator.cpp
    tutor_broker_adapter.cpp
    tutor_gui_bridge.cpp
    teacher_ai.cpp
    student_ai.cpp
    aitutor_loop.cpp
    student_level_evaluator.cpp # Yeni eklenen sınıf
    enums.h # Yeni eklenen enum dosyası
)

target_include_directories(ai_tutor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../external") # For nlohmann/json.hpp
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../brain") # For IntentLearnerQtBridge.h
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../communication") # For TutorBrokerAdapter.h
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../learning") # StrategyOutcome.h için
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..") # For headers in sibling directories like 'core'
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../swarm_vectordb") # DataModels.h için
target_include_directories(ai_tutor PUBLIC "C:/vcpkg/installed/x64-mingw-static/include") # vcpkg global include (Eigen vb. için)
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../core") # For core headers like IVectorDatabase.h
target_include_directories(ai_tutor PUBLIC ${Eigen3_INCLUDE_DIRS}) # YENİ: Eigen3 için include dizinleri
target_include_directories(ai_tutor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../hnswlib_wrapper") # hnswlib_wrapper.h için

# If using Qt GUI parts, link Qt6::Widgets, Qt6::Charts
set_target_properties(ai_tutor PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
target_link_libraries(ai_tutor PRIVATE Qt6::Widgets) # Qt6::Charts commented out as not explicitly used yet

target_include_directories(ai_tutor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/brain
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/learning # LearningStore.h için
    ${CMAKE_SOURCE_DIR}/src/communication # TutorBrokerAdapter.h için
    ${CMAKE_SOURCE_DIR}/src/swarm_vectordb # DataModels.h için
    ${CMAKE_SOURCE_DIR}/src/hnswlib_wrapper # hnswlib_wrapper.h için
)


