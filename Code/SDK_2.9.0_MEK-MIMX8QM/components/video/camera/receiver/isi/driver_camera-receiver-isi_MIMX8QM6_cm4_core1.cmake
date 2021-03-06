if(NOT DRIVER_CAMERA-RECEIVER-ISI_MIMX8QM6_cm4_core1_INCLUDED)
    
    set(DRIVER_CAMERA-RECEIVER-ISI_MIMX8QM6_cm4_core1_INCLUDED true CACHE BOOL "driver_camera-receiver-isi component is included.")

    target_sources(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/fsl_isi_camera_adapter.c
    )


    target_include_directories(${MCUX_SDK_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/.
    )


    include(driver_isi_MIMX8QM6_cm4_core1)

    include(driver_camera-receiver-common_MIMX8QM6_cm4_core1)

    include(driver_camera-common_MIMX8QM6_cm4_core1)

    include(driver_video-common_MIMX8QM6_cm4_core1)

endif()
