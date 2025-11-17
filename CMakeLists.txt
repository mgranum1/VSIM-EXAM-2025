cmake_minimum_required(VERSION 3.19)
project(QtVulkan LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(QtVulkan
    main.cpp
    Core/Renderer.h 
    Core/Renderer.cpp

    Core/Utility/Utilities.h
    Core/Utility/Vertex.h
    Core/Utility/gpuresourcemanager.cpp
    Core/Utility/gpuresourcemanager.h
    Core/Utility/modelloader.h 
    Core/Utility/modelloader.cpp
    Core/Utility/Modeldata.h
    Core/Utility/BblHub.h 

    Core/Camera.h
    Core/Camera.cpp
    
    Editor/MainWindow.h 
    Editor/MainWindow.cpp
    Editor/MainWindow.ui

    ECS/System.h

    ECS/Entity/Entity.h
    ECS/Entity/EntityManager.h 
    ECS/Entity/EntityManager.cpp
    ECS/Entity/SceneSerializer.h
    ECS/Entity/SceneSerializer.cpp
    ECS/Entity/SceneManager.h
    ECS/Entity/SceneManager.cpp

    ECS/Components/Components.h
    ECS/Components/Physics.cpp
    ECS/Components/Physics.h
    ECS/Components/CollisionSystem.h 
    ECS/Components/CollisionSystem.cpp
    
    SoundSystem/resourcemanager.h 
    SoundSystem/resourcemanager.cpp

    Game/GameWorld.h 
    Game/GameWorld.cpp

    Game/Terrain.h
    Game/Terrain.cpp
    
    Scripting/luamanager.cpp
    Scripting/luamanager.h 
    Assets/Scripts/test.lua

    Shaders/Shader.frag
    Shaders/Shader.vert
    Shaders/Phong.frag
    Shaders/Phong.vert


    

)

#OpenAL
# Add include directory
target_include_directories(QtVulkan PRIVATE $ENV{OPENAL_HOME}/include)
# Add lib directory
target_link_libraries(QtVulkan PRIVATE $ENV{OPENAL_HOME}/libs/Win64/OpenAL32.lib)

# Copy the DLL after build
# This makes the .exe use the correct .dll from the library we compile to
# (the "COMMAND echo" only works on Windows)
add_custom_command(TARGET QtVulkan POST_BUILD
    #COMMAND echo Copying OpenAL32.dll...
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $ENV{OPENAL_HOME}/bin/Win64/OpenAL32.dll
        $<TARGET_FILE_DIR:QtVulkan>)

#Lua scripting
# Copy the Lua scripts after build
add_custom_command(TARGET QtVulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets/Scripts"
        "$<TARGET_FILE_DIR:QtVulkan>/Assets/Scripts")


if(MSVC)
    # Required for proper C++ exception handling with MSVC
    target_compile_options(QtVulkan PRIVATE /EHsc)
    target_include_directories(QtVulkan PRIVATE
        "C:/VulkanSDK/1.4.321.1/Include"
        ${CMAKE_CURRENT_SOURCE_DIR}/External/Lua/include
    )
    target_link_libraries(QtVulkan PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::Widgets
        Qt6::Multimedia
        "C:/VulkanSDK/1.4.321.1/Lib/vulkan-1.lib"
        ${CMAKE_CURRENT_SOURCE_DIR}/External/Lua/lib/lua5.4.3-static.lib
    )
elseif(APPLE)
    # This will set NDEBUG for both debug and release build
    # It stops Validation layers from being used - needs to be fixed!!!
    add_compile_definitions(NDEBUG)

    target_include_directories(QtVulkan PRIVATE
        #Hardcoded path for Oles Macs for now:
        "/Users/ole/VulkanSDK/1.4.321.0/macOS/include"
    )
    target_link_libraries(QtVulkan PRIVATE
        Qt::Core
        Qt::Widgets
        "-framework QuartzCore"
        "-framework Metal"
        "-framework Foundation"
        "-framework IOSurface"
        "-framework CoreGraphics"
        "-framework IOKit"
        "-framework AppKit"
        #Hardcoded path for Oles Macs for now:
        "/Users/ole/VulkanSDK/1.4.321.0/macOS/lib/MoltenVK.xcframework/macos-arm64_x86_64/libMoltenVK.a"
    )
endif()

include(GNUInstallDirs)
install(TARGETS QtVulkan
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
