cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

INCLUDE(pandocology)

add_document(
    TARGET user_guide
    OUTPUT_FILE HapExporterPlugin_for_AdobeCC2018.html
    SOURCES user_guide/README.md
    RESOURCE_FILES       ../asset/hap-logo.svg ../asset/github-pandoc.css
    PANDOC_DIRECTIVES -f markdown
                      -c github-pandoc.css
		      --standalone
)
