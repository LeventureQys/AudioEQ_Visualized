function(embed_font OUTPUT_HEADER OUTPUT_SOURCE FONT_FILE VAR_NAME)

    file(SIZE "${FONT_FILE}" FILE_SIZE)
    file(READ "${FONT_FILE}" HEX_DATA HEX)

    string(REGEX REPLACE "(..)" "0x\\1,\n" BYTE_DATA "${HEX_DATA}")
    string(REGEX REPLACE ",\n$" "" BYTE_DATA "${BYTE_DATA}")

    set(HEADER_CONTENT "#pragma once\n#include <cstddef>\n\nnamespace EmbeddedFont {\nextern const unsigned char ${VAR_NAME}[${FILE_SIZE}];\nextern const size_t ${VAR_NAME}_size;\n}\n")
    set(SOURCE_CONTENT "#include \"${OUTPUT_HEADER}\"\n\nnamespace EmbeddedFont {\nconst unsigned char ${VAR_NAME}[${FILE_SIZE}] = {\n${BYTE_DATA}\n};\nconst size_t ${VAR_NAME}_size = ${FILE_SIZE};\n}\n")

    file(WRITE "${OUTPUT_HEADER}" "${HEADER_CONTENT}")
    file(WRITE "${OUTPUT_SOURCE}" "${SOURCE_CONTENT}")
endfunction()
