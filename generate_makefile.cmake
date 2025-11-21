# CMake script to generate a standalone Makefile from template
# This Makefile can be used without CMake installed

cmake_policy(SET CMP0053 NEW)

# Read sources from file
file(READ "${SOURCES_FILE}" SOURCES)

# Format the sources list for Makefile (with proper line continuations)
string(REPLACE ";" " \\\n\t\t\t" SOURCES_LIST "${SOURCES}")

# Read the template
file(READ "${INPUT_FILE}" MAKEFILE_CONTENT)

# Replace the placeholder with the sources list
string(REPLACE "@SOURCES_LIST@" "${SOURCES_LIST}" MAKEFILE_CONTENT_OUT "${MAKEFILE_CONTENT}")

# Write the generated Makefile
file(WRITE "${OUTPUT_FILE}" "${MAKEFILE_CONTENT_OUT}")

message(STATUS "Generated ${OUTPUT_FILE} from ${INPUT_FILE}")
