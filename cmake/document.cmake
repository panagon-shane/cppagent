find_package(Doxygen OPTIONAL_COMPONENTS dot)
if (DOXYGEN_FOUND)
  set(DOXYGEN_PROJECT_NAME "MTConnect C++ Agent")
  set(DOXYGEN_PROJECT_BRIEF "MTConnect Reference C++ Agent")
  set(DOXYGEN_PROJECT_LOGO "${PROJECT_SOURCE_DIR}/styles/favicon.ico")
  set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Documentation")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_DOCSET_BUNDLE_ID org.mtconnect.agent)
  set(DOXYGEN_DOCSET_PUBLISHER_ID org.amtonline)
  set(DOXYGEN_DOCSET_PUBLISHER_NAME "Association for Manufacturing Technology")
  set(DOXYGEN_TAB_SIZE 2)
  set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
  set(DOXYGEN_UML_LOOK NO)
  set(DOXYGEN_DOT_IMAGE_FORMAT svg)
  set(DOXYGEN_INTERACTIVE_SVG YES)
  set(DOXYGEN_INCLUDE_PATH "${CONAN_INCLUDE_DIRS}")
  set(DOXYGEN_HTML_COLORSTYLE "DARK")
endif()
