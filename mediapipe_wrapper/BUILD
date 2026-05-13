load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "mediapipe_bridge",
    srcs = ["MediaPipeBridge.cpp"],
    hdrs = ["MediaPipeBridge.h"],
    visibility = ["//visibility:public"],
    deps = [
        # In a real environment, this depends on:
        # "@mediapipe//mediapipe/tasks/cc/vision/face_landmarker:face_landmarker",
        # "@mediapipe//mediapipe/framework/formats:image_frame",
        # "@mediapipe//mediapipe/framework/formats:image",
    ],
)
