if(NOT DRIVER_CAMERA-DEVICE-OV5640_MIMX8QM6_cm4_core1_INCLUDED)
    
    set(DRIVER_CAMERA-DEVICE-OV5640_MIMX8QM6_cm4_core1_INCLUDED true CACHE BOOL "driver_camera-device-ov5640 component is included.")

    target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/fsl_ov5640.c
    )


    target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/.
    )


    include(driver_video-common_MIMX8QM6_cm4_core1)

    include(driver_camera-common_MIMX8QM6_cm4_core1)

    include(driver_camera-device-common_MIMX8QM6_cm4_core1)

    include(driver_camera-device-sccb_MIMX8QM6_cm4_core1)

endif()
