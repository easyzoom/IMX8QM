if(NOT DRIVER_CAMERA-DEVICE-MAX9286_MIMX8QM6_cm4_core1_INCLUDED)
    
    set(DRIVER_CAMERA-DEVICE-MAX9286_MIMX8QM6_cm4_core1_INCLUDED true CACHE BOOL "driver_camera-device-max9286 component is included.")

    target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/fsl_max9286.c
    )


    target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/.
    )


    include(driver_video-common_MIMX8QM6_cm4_core1)

    include(driver_camera-common_MIMX8QM6_cm4_core1)

    include(driver_camera-device-common_MIMX8QM6_cm4_core1)

    include(driver_video-i2c_MIMX8QM6_cm4_core1)

endif()
