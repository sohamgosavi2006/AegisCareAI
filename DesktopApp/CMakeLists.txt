cmake_minimum_required(VERSION 3.16)
project(AegisCareAI VERSION 1.0.0 LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure Qt 6 Auto compilers
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Locate Qt6 Components
find_package(Qt6 COMPONENTS 
    Core 
    Widgets 
    Charts 
    Sql 
    Network
    SerialPort 
    REQUIRED
)
find_package(Qt6 QUIET COMPONENTS TextToSpeech)

# OpenCV enhances face detection when available. The application retains a
# Qt-only quality-analysis path so rural/offline deployments remain portable.
find_package(OpenCV QUIET COMPONENTS core imgproc objdetect)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/communication
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/components
)

# Define source groups
set(SOURCES
    src/main.cpp
    src/core/DatabaseManager.cpp
    src/core/VoiceAssistant.cpp
    src/core/CommunicationManager.cpp
    src/core/DemoReportGenerator.cpp
    src/communication/CommunicationLogger.cpp
    src/protocol/JsonLineProtocol.cpp
    src/core/CameraQualityEngine.cpp
    src/core/CameraStreamManager.cpp
    src/core/ESP32CameraProvider.cpp
    src/core/CameraSourceManager.cpp
    src/core/WorkflowEngine.cpp
    src/ui/components/DigitalTwinWidget.cpp
    src/ui/components/ModernCard.cpp
    src/ui/LoginWindow.cpp
    src/ui/MainWindow.cpp
    src/ui/ClinicalVoiceNotesPanel.cpp
    src/ui/DashboardView.cpp
    src/ui/PatientManagementView.cpp
    src/ui/DigitalTwinView.cpp
    src/ui/ScreeningView.cpp
    src/ui/PreventiveView.cpp
    src/ui/TelemedicineView.cpp
    src/ui/SettingsView.cpp
    src/ui/UiTheme.cpp
    src/resources/resources.qrc
)

set(HEADERS
    src/models/Models.h
    src/core/DatabaseManager.h
    src/core/VoiceAssistant.h
    src/core/CommunicationManager.h
    src/core/DemoReportGenerator.h
    src/communication/CommunicationLogger.h
    src/protocol/JsonLineProtocol.h
    src/core/CameraQualityEngine.h
    src/core/CameraStreamManager.h
    src/core/CameraProvider.h
    src/core/ESP32CameraProvider.h
    src/core/LaptopCameraProvider.h
    src/core/CameraSourceManager.h
    src/core/WorkflowEngine.h
    src/ui/components/DigitalTwinWidget.h
    src/ui/components/ModernCard.h
    src/ui/LoginWindow.h
    src/ui/MainWindow.h
    src/ui/ClinicalVoiceNotesPanel.h
    src/ui/DashboardView.h
    src/ui/PatientManagementView.h
    src/ui/DigitalTwinView.h
    src/ui/ScreeningView.h
    src/ui/PreventiveView.h
    src/ui/TelemedicineView.h
    src/ui/SettingsView.h
    src/ui/UiTheme.h
)

if(APPLE)
    list(APPEND SOURCES src/core/LaptopCameraProvider.mm)
    set_source_files_properties(src/core/LaptopCameraProvider.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
else()
    list(APPEND SOURCES src/core/LaptopCameraProvider.cpp)
endif()

# Application Executable
add_executable(AegisCareAI MACOSX_BUNDLE ${SOURCES} ${HEADERS})
set_target_properties(AegisCareAI PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "AegisCare AI"
    MACOSX_BUNDLE_GUI_IDENTIFIER "care.aegis.desktop"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/Info.plist.in"
)

if(APPLE)
    target_link_libraries(AegisCareAI PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo"
    )
endif()

# Link Libraries
target_link_libraries(AegisCareAI PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Qt6::Sql
    Qt6::Network
    Qt6::SerialPort
)

if(TARGET Qt6::TextToSpeech)
    target_compile_definitions(AegisCareAI PRIVATE AEGISCARE_HAS_QT_SPEECH=1)
    target_link_libraries(AegisCareAI PRIVATE Qt6::TextToSpeech)
endif()

if(OpenCV_FOUND)
    target_compile_definitions(AegisCareAI PRIVATE AEGISCARE_HAS_OPENCV=1)
    target_include_directories(AegisCareAI PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(AegisCareAI PRIVATE ${OpenCV_LIBS})
else()
    message(STATUS "OpenCV not found; using the built-in Qt camera quality engine")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(AegisCareLoginInteractionTests
        tests/LoginInteractionTests.cpp
        src/ui/LoginWindow.cpp
        src/ui/LoginWindow.h
        src/core/DatabaseManager.cpp
        src/core/DatabaseManager.h
        src/core/WorkflowEngine.cpp
        src/core/WorkflowEngine.h
        src/core/VoiceAssistant.cpp
        src/core/VoiceAssistant.h
        src/models/Models.h
    )
    target_link_libraries(AegisCareLoginInteractionTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Network)
    if(TARGET Qt6::TextToSpeech)
        target_compile_definitions(AegisCareLoginInteractionTests PRIVATE AEGISCARE_HAS_QT_SPEECH=1)
        target_link_libraries(AegisCareLoginInteractionTests PRIVATE Qt6::TextToSpeech)
    endif()
    add_test(NAME login_interaction_stability COMMAND AegisCareLoginInteractionTests)
    set_tests_properties(login_interaction_stability PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(AegisCareUiInteractionTests
        tests/UiInteractionTests.cpp
        src/ui/components/DigitalTwinWidget.cpp
        src/ui/components/DigitalTwinWidget.h
    )
    target_link_libraries(AegisCareUiInteractionTests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
    add_test(NAME ui_interaction_stability COMMAND AegisCareUiInteractionTests)
    set_tests_properties(ui_interaction_stability PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(AegisCareCommunicationTests
        tests/CommunicationProtocolTests.cpp
        src/protocol/JsonLineProtocol.cpp
        src/protocol/JsonLineProtocol.h
    )
    target_link_libraries(AegisCareCommunicationTests PRIVATE Qt6::Core)
    add_test(NAME communication_protocol COMMAND AegisCareCommunicationTests)

    add_executable(AegisCareSmokeTests
        tests/SmokeTests.cpp
        src/core/DatabaseManager.cpp
        src/core/VoiceAssistant.cpp
        src/core/WorkflowEngine.cpp
        src/core/CameraQualityEngine.cpp
        src/models/Models.h
        src/core/DatabaseManager.h
        src/core/VoiceAssistant.h
        src/core/WorkflowEngine.h
        src/core/CameraQualityEngine.h
    )
    target_link_libraries(AegisCareSmokeTests PRIVATE
        Qt6::Core Qt6::Gui Qt6::Sql Qt6::Network
    )
    if(TARGET Qt6::TextToSpeech)
        target_compile_definitions(AegisCareSmokeTests PRIVATE AEGISCARE_HAS_QT_SPEECH=1)
        target_link_libraries(AegisCareSmokeTests PRIVATE Qt6::TextToSpeech)
    endif()
    if(OpenCV_FOUND)
        target_compile_definitions(AegisCareSmokeTests PRIVATE AEGISCARE_HAS_OPENCV=1)
        target_include_directories(AegisCareSmokeTests PRIVATE ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(AegisCareSmokeTests PRIVATE ${OpenCV_LIBS})
    endif()
    add_test(NAME core_smoke COMMAND AegisCareSmokeTests)
endif()
