cmake_minimum_required(VERSION 3.20)
project(ModeratedLMS VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)
find_package(OpenSSL REQUIRED)
add_executable(moderated_lms
    src/main.cpp
    src/database.cpp
    src/login_dialog.cpp
    src/main_window.cpp
    src/ui/admin_dashboard.cpp
    src/ui/lecturer_dashboard.cpp
    src/ui/hod_dashboard.cpp
    src/ui/student_dashboard.cpp
)
target_link_libraries(moderated_lms PRIVATE Qt6::Widgets Qt6::Sql OpenSSL::Crypto)
target_include_directories(moderated_lms PRIVATE include)
target_include_directories(moderated_lms PRIVATE ${OPENSSL_INCLUDE_DIR})


include(CTest)

if(BUILD_TESTING)

    find_package(GTest REQUIRED)

    add_executable(lms_tests
        tests/test_main.cpp
        tests/domain_tests.cpp
        tests/database_tests.cpp
        tests/submission_tests.cpp
        tests/moderation_tests.cpp
    )

    target_include_directories(lms_tests PRIVATE
        include
    )

    target_link_libraries(lms_tests PRIVATE
        GTest::gtest
        Qt6::Core
        Qt6::Sql
    )

    add_test(
        NAME lms_tests
        COMMAND lms_tests
    )

endif()

