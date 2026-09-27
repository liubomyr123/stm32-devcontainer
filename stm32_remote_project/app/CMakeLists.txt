file(GLOB_RECURSE APP_SOURCES "${APP_DIR}/src/*.cpp" "${APP_DIR}/*.cpp")

file(GLOB MBEDTLS_SOURCES "${APP_DIR}/third_party/mbedtls/library/*.c")
list(APPEND APP_SOURCES ${MBEDTLS_SOURCES})