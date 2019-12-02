set(MODEL_TOOLS_PROJECT_NAME "model-tools")
unset(MODEL_TOOLS_ROOT)
find_path(MODEL_TOOLS_ROOT NAMES ${MODEL_TOOLS_PROJECT_NAME} HINTS ${BOLT_ROOT} $ENV{BOLT_ROOT})
set(MODEL_TOOLS_ROOT "${MODEL_TOOLS_ROOT}/${MODEL_TOOLS_PROJECT_NAME}")

if (USE_DYNAMIC_LIBRARY)
    set(MODEL_TOOLS_ONNX_LIBRARY "${MODEL_TOOLS_ROOT}/lib/lib${MODEL_TOOLS_PROJECT_NAME}_onnx.so")
else (USE_DYNAMIC_LIBRARY)
    set(MODEL_TOOLS_ONNX_LIBRARY "${MODEL_TOOLS_ROOT}/lib/lib${MODEL_TOOLS_PROJECT_NAME}_onnx.a")
endif (USE_DYNAMIC_LIBRARY)

if (MODEL_TOOLS_ONNX_LIBRARY)
    set(MODEL_TOOLS_ONNX_FOUND true)
endif (MODEL_TOOLS_ONNX_LIBRARY)

find_package(Protobuf)

if (MODEL_TOOLS_ONNX_FOUND)
    set(MODEL_TOOLS_ONNX_LIBRARIES "${MODEL_TOOLS_ONNX_LIBRARY};${Protobuf_LIBRARY};-lpthread")
    include_directories(include ${Protobuf_INCLUDE_DIR})
    message(STATUS "Found ${MODEL_TOOLS_PROJECT_NAME}_onnx: ${MODEL_TOOLS_ONNX_LIBRARY}")
else (MODEL_TOOLS_ONNX_FOUND)
    message(FATAL_ERROR "Could not find ${MODEL_TOOLS_PROJECT_NAME}_onnx library")
endif (MODEL_TOOLS_ONNX_FOUND)