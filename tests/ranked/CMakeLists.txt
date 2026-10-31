cmake_minimum_required(VERSION 3.16)
project(mw2_ranked_rules_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mw2_ranked_rules_tests
    ../../runtime/ranked_rules.cpp
    ranked_rules_tests.cpp
)
target_include_directories(mw2_ranked_rules_tests PRIVATE ../../runtime)

enable_testing()
add_test(NAME mw2_ranked_rules COMMAND mw2_ranked_rules_tests)
