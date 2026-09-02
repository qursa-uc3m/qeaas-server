cmake_minimum_required(VERSION 3.16)

project(eaas-quantis-qrng-api VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_OS_XOR "Allow XORing QRNG output with /dev/urandom" ON)
set(QUANTIS_ROOT "/opt/quantis" CACHE PATH "Quantis installation prefix")

find_library(DROGON_LIBRARY drogon REQUIRED)
if(DROGON_LIBRARY MATCHES "\\.a$")
  find_package(Drogon CONFIG REQUIRED)
  set(DROGON_LIBRARIES Drogon::Drogon)
else()
  find_path(DROGON_INCLUDE_DIR drogon/drogon.h REQUIRED)
  find_path(JSONCPP_INCLUDE_DIR json/json.h PATH_SUFFIXES jsoncpp REQUIRED)
  find_library(JSONCPP_LIBRARY jsoncpp REQUIRED)
  find_library(TRANTOR_LIBRARY trantor REQUIRED)
  set(DROGON_LIBRARIES ${DROGON_LIBRARY} ${TRANTOR_LIBRARY} ${JSONCPP_LIBRARY})
endif()

find_path(QUANTIS_INCLUDE_DIR Quantis.h
  HINTS ${QUANTIS_ROOT}
  PATH_SUFFIXES include Libs-Apps/Quantis
  REQUIRED
)
find_path(QUANTIS_EXTENSIONS_INCLUDE_DIR QuantisExtractor.h
  HINTS ${QUANTIS_ROOT}
  PATH_SUFFIXES include Libs-Apps/QuantisExtensions
  REQUIRED
)
find_library(QUANTIS_LIBRARY Quantis
  HINTS ${QUANTIS_ROOT}
  PATH_SUFFIXES lib lib64 Libs-Apps/build/Quantis
  REQUIRED
)
find_library(QUANTIS_EXTENSIONS_LIBRARY Quantis_Extensions
  HINTS ${QUANTIS_ROOT}
  PATH_SUFFIXES lib lib64 Libs-Apps/build/QuantisExtensions
  REQUIRED
)

add_executable(${PROJECT_NAME}
  main.cc
  RandomSource.cc
  controllers/QuantumRandomNumberController.cc
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/controllers
  ${DROGON_INCLUDE_DIR}
  ${JSONCPP_INCLUDE_DIR}
  ${QUANTIS_INCLUDE_DIR}
  ${QUANTIS_EXTENSIONS_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${DROGON_LIBRARIES}
  ${QUANTIS_EXTENSIONS_LIBRARY}
  ${QUANTIS_LIBRARY}
)

if(ENABLE_OS_XOR)
  target_compile_definitions(${PROJECT_NAME} PRIVATE QRNG_ENABLE_OS_XOR=1)
endif()

get_filename_component(QUANTIS_LIBRARY_DIR ${QUANTIS_LIBRARY} DIRECTORY)
get_filename_component(QUANTIS_EXTENSIONS_LIBRARY_DIR ${QUANTIS_EXTENSIONS_LIBRARY} DIRECTORY)
set_target_properties(${PROJECT_NAME} PROPERTIES
  BUILD_RPATH "${QUANTIS_LIBRARY_DIR};${QUANTIS_EXTENSIONS_LIBRARY_DIR}"
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
