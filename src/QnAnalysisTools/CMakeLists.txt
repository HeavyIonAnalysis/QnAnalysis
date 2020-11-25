


add_library(QnAnalysisTools INTERFACE)
target_include_directories(QnAnalysisTools INTERFACE $<BUILD_INTERFACE:${QnAnalysis_SOURCE_DIR}>)