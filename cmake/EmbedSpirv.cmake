function(embed_spirv OUTPUT_HEADER OUTPUT_SOURCE)
    set(HEADER_CONTENT "#pragma once\n#include <cstddef>\n#include <cstdint>\n\nnamespace EmbeddedShaders {\n")
    set(SOURCE_CONTENT "#include \"${OUTPUT_HEADER}\"\n\nnamespace EmbeddedShaders {\n")

    foreach(SPV_FILE ${ARGN})
        get_filename_component(RAW_NAME "${SPV_FILE}" NAME)
        string(REGEX REPLACE "\\.spv$" "" RAW_NAME "${RAW_NAME}")
        string(REPLACE "." "_" VAR_NAME "${RAW_NAME}")
        string(TOUPPER "${VAR_NAME}" VAR_NAME)

        file(SIZE "${SPV_FILE}" FILE_SIZE)

        file(READ "${SPV_FILE}" HEX_BYTES HEX)
        string(LENGTH "${HEX_BYTES}" HEX_LEN)

        set(WORDS "")
        set(I 0)
        while(I LESS HEX_LEN)
            math(EXPR J "${I} + 8")
            if(J GREATER HEX_LEN)
                math(EXPR J "${HEX_LEN}")
            endif()
            string(SUBSTRING "${HEX_BYTES}" ${I} 8 CHUNK)
            string(LENGTH "${CHUNK}" CHUNK_LEN)

            if(CHUNK_LEN LESS 8)
                set(PADDED "${CHUNK}")
                while(CHUNK_LEN LESS 8)
                    set(PADDED "00${PADDED}")
                    math(EXPR CHUNK_LEN "${CHUNK_LEN} + 2")
                endwhile()
                set(CHUNK "${PADDED}")
            endif()

            string(SUBSTRING "${CHUNK}" 0 2 B0)
            string(SUBSTRING "${CHUNK}" 2 2 B1)
            string(SUBSTRING "${CHUNK}" 4 2 B2)
            string(SUBSTRING "${CHUNK}" 6 2 B3)

            set(WORD "0x${B3}${B2}${B1}${B0}")

            if(J GREATER_EQUAL HEX_LEN)
                set(WORDS "${WORDS}    ${WORD}\n")
            else()
                set(WORDS "${WORDS}    ${WORD},\n")
            endif()

            math(EXPR I "${J}")
        endwhile()

        math(EXPR U32_SIZE "(${FILE_SIZE} + 3) / 4")

        set(HEADER_CONTENT "${HEADER_CONTENT}extern const uint32_t ${VAR_NAME}[${U32_SIZE}];\nextern const size_t ${VAR_NAME}_size;\n\n")
        set(SOURCE_CONTENT "${SOURCE_CONTENT}const uint32_t ${VAR_NAME}[${U32_SIZE}] = {\n${WORDS}};\nconst size_t ${VAR_NAME}_size = ${U32_SIZE};\n\n")
    endforeach()

    set(HEADER_CONTENT "${HEADER_CONTENT}} // namespace EmbeddedShaders\n")
    set(SOURCE_CONTENT "${SOURCE_CONTENT}} // namespace EmbeddedShaders\n")

    file(WRITE "${OUTPUT_HEADER}" "${HEADER_CONTENT}")
    file(WRITE "${OUTPUT_SOURCE}" "${SOURCE_CONTENT}")
endfunction()
