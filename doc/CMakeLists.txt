cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

INCLUDE(pandocology)

add_document(
    TARGET user_guide
    OUTPUT_FILE HapExporterPlugin_for_AdobeCC2018.html
    SOURCES user_guide/README.md
    RESOURCE_FILES       ../asset/hap-logo.svg ../asset/github-pandoc.css ./user_guide/chunk-counts.png ./user_guide/codec-options.png ./user_guide/format-option.png ./user_guide/media-encoder-presets.png
    PANDOC_DIRECTIVES -f markdown
                      -c github-pandoc.css
		      --standalone
)

# TODO: use variables for this
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/HapExporterPlugin_for_AdobeCC2018.html
        ../asset/hap-logo.svg ../asset/github-pandoc.css ./user_guide/chunk-counts.png ./user_guide/codec-options.png ./user_guide/format-option.png ./user_guide/media-encoder-presets.png
    DESTINATION
        doc
    COMPONENT
        user_guide
)