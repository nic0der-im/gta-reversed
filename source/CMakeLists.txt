cmake_minimum_required(VERSION 4.2.0 FATAL_ERROR)

include(FetchContent)

#--------------------------------------------------------------------
# Game features
#--------------------------------------------------------------------
option(${RE_PROJECT}_WITH_OPENAL "Use OpenAL" OFF)
option(${RE_PROJECT}_USE_SDL3 "Use SDL3" OFF)
option(${RE_PROJECT}_STANDALONE "Build standalone executable" OFF)
option(${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY "Dump hooks only" OFF)
option(${RE_PROJECT}_NATIVE "Build a native (host OS, no retail exe) executable" OFF)

if (${RE_PROJECT}_USE_SDL3)
    add_compile_definitions(NOTSA_USE_SDL3)
endif()
if (${RE_PROJECT}_NATIVE AND NOT ${RE_PROJECT}_USE_SDL3)
    message(FATAL_ERROR "${RE_PROJECT}_NATIVE requires ${RE_PROJECT}_USE_SDL3=ON")
endif()
if (${RE_PROJECT}_STANDALONE)
    add_compile_definitions(NOTSA_STANDALONE)
endif()
if (${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY)
    if (NOT ${RE_PROJECT}_STANDALONE)
        message(FATAL_ERROR "${RE_PROJECT}_STANDALONE_DUMP_HOOKS_ONLY requires ${RE_PROJECT}_STANDALONE=ON")
    endif()
    add_compile_definitions(NOTSA_STANDALONE_DUMP_HOOKS_ONLY)
endif()

#--------------------------------------------------------------------
# Build options
#--------------------------------------------------------------------
option(${RE_PROJECT}_UNITY_BUILD "Use Unity/Jumbo build" OFF)
option(${RE_PROJECT}_WITH_SANITIZERS "Use Sanitizers (better crash log)" OFF)
option(${RE_PROJECT}_WITH_LTO "Use Link Time Optimization" OFF)
option(${RE_PROJECT}_WITH_CLEO_COMMANDS "Enable CLEO script commands" OFF)
option(${RE_PROJECT}_WITH_SCRIPT_COMMAND_HOOKS "Create script command hooks in the UI (Command hooks are installed regardless)" OFF)

if(${RE_PROJECT}_WITH_SCRIPT_COMMAND_HOOKS)
    add_compile_definitions(NOTSA_WITH_SCRIPT_COMMAND_HOOKS)
endif()

if (NOT WIN32 AND NOT ${RE_PROJECT}_NATIVE AND NOT ${RE_PROJECT}_WITH_OPENAL)
    set(${RE_PROJECT}_WITH_OPENAL ON)
endif()

#--------------------------------------------------------------------
# Gather source files, and create the target
#--------------------------------------------------------------------

#
# Regular source files
# Module scanning is disabled for these, so we can unity build
# If any of them contain module imports the compiler should error out
#
file(
    GLOB_RECURSE SOURCE_FILES_WITHOUT_MODULES_LIST
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h*"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c*"
)
list(APPEND SOURCE_FILES_WITHOUT_MODULES_LIST StdInc.cpp)
if (${RE_PROJECT}_WITH_OPENAL OR ${RE_PROJECT}_NATIVE)
    # TODO: Remove dsound specific sources
else()
    file(
        GLOB_RECURSE OPENAL_FILES
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/game_sa/Audio/OpenAL/*.c*"
        "${CMAKE_CURRENT_SOURCE_DIR}/game_sa/Audio/OpenAL/*.h*"
    )
    list(REMOVE_ITEM SOURCE_FILES_WITHOUT_MODULES_LIST ${OPENAL_FILES})
endif()

if (${RE_PROJECT}_NATIVE)
    # The native build replaces the Windows platform layer (Win32, Direct3D9, DirectShow,
    # the hooking machinery and the tools menu) with `app/platform/native/`.
    # NOTE: `app/platform/native/` itself MUST stay in the source list.
    list(FILTER SOURCE_FILES_WITHOUT_MODULES_LIST EXCLUDE REGEX "^(app/platform/win|reversiblehooks|toolsmenu)/")
    list(FILTER SOURCE_FILES_WITHOUT_MODULES_LIST EXCLUDE REGEX "^(oswrapper/oswrapper_win|dllmain|InjectHooksMain)\\.cpp$")
    # RenderWare: every retail thunk (`plugin::Call` into the exe) is replaced by the fakerw layer
    # in `game_sa/RenderWare/librw/`, which implements the same C API on top of librw.
    # NOTE: `librw/` is a subdirectory of `RenderWare/`, so it is matched separately below.
    list(FILTER SOURCE_FILES_WITHOUT_MODULES_LIST EXCLUDE REGEX "^game_sa/RenderWare/rw/[^/]*\\.cpp$")
    list(FILTER SOURCE_FILES_WITHOUT_MODULES_LIST EXCLUDE REGEX "^game_sa/RenderWare/D3D[^/]*\\.cpp$")
    list(FILTER SOURCE_FILES_WITHOUT_MODULES_LIST EXCLUDE REGEX "^game_sa/Audio/Loaders/AE(WMA|MF)Decoder\\.")
endif()

#
# Source files that should be scanned for modules
#
set(SOURCE_FILES_WITH_MODULES_LIST) # Empty for now, we dont use modules yet

#
# Module source files
#
file(
    GLOB_RECURSE CXX_MODULE_FILES_LIST
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ixx"
)

if (${RE_PROJECT}_NATIVE)
    add_executable(${RE_PROJECT_LIB_NAME} MACOSX_BUNDLE)
elseif (${RE_PROJECT}_STANDALONE)
    add_executable(${RE_PROJECT_LIB_NAME} WIN32)
else()
    add_library(${RE_PROJECT_LIB_NAME} MODULE)
endif()
if (CXX_MODULE_FILES_LIST)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES ${CXX_MODULE_FILES_LIST})
endif()
if (SOURCE_FILES_WITH_MODULES_LIST)
    set_source_files_properties(${SOURCE_FILES_WITH_MODULES_LIST} PROPERTIES CXX_SCAN_FOR_MODULES ON)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE FILE_SET cxx_modules FILES ${SOURCE_FILES_WITH_MODULES_LIST})
endif()
if (SOURCE_FILES_WITHOUT_MODULES_LIST)
    set_source_files_properties(${SOURCE_FILES_WITHOUT_MODULES_LIST} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
    target_sources(${RE_PROJECT_LIB_NAME} PRIVATE ${SOURCE_FILES_WITHOUT_MODULES_LIST})
endif()
target_link_options(${RE_PROJECT_LIB_NAME} PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/INCREMENTAL>"
)
target_compile_options(${RE_PROJECT_LIB_NAME} PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/MP;/Zc:preprocessor;/W3;/wd4200;/wd4251;/wd4275;/wd4996;/wd26495;/wd26812;/wd28159>"
    "$<$<OR:$<CXX_COMPILER_ID:AppleClang,Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-Wall;-Wno-switch>"
)
if (${RE_PROJECT}_NATIVE)
    # MSVC extensions and attributes are still used by the shared game code
    # (`this->Class::Class()` constructor calls in the hook wrappers, typed anonymous unions, ...)
    target_compile_options(${RE_PROJECT_LIB_NAME} PRIVATE
        "$<$<OR:$<CXX_COMPILER_ID:AppleClang,Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-fms-extensions;-Wno-microsoft-explicit-constructor-call;-Wno-microsoft-anon-tag;-Wno-ignored-attributes>"
        # No debug info: the crash handler's backtrace resolves through the symbol table,
        # and full `-g` at `-O3` doubled the build time of every wave.
        "$<$<OR:$<CXX_COMPILER_ID:AppleClang,Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-g0>"
    )
endif()

if (${RE_PROJECT}_NATIVE)
    # MACOSX_BUNDLE names its own output (`gta_reversed.app`), so no suffix is needed
    set(TARGET_SUFFIX "")
    # The native port still has plenty of non-portable warnings; don't fail the build on them
    set(TARGET_WARNINGS_AS_ERRORS OFF)

    # `Info.plist` of the bundle: identity, display name, Retina support and minimum macOS version.
    set(NATIVE_BUNDLE_IDENTIFIER  "io.github.nic0der-im.gtareversed")
    set(NATIVE_BUNDLE_NAME        "GTA San Andreas (reversed)")
    set(NATIVE_BUNDLE_EXECUTABLE  "gta_reversed")
    set(NATIVE_BUNDLE_VERSION     "1.0.0")
    set(NATIVE_BUNDLE_INFO_PLIST  "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/app/platform/native/Info.plist.in"
        "${NATIVE_BUNDLE_INFO_PLIST}"
        @ONLY
    )
else()
    set(TARGET_WARNINGS_AS_ERRORS ON)
    if (${RE_PROJECT}_STANDALONE)
        set(TARGET_SUFFIX ".exe")
    else()
        set(TARGET_SUFFIX ".asi")
    endif()
endif()
set_target_properties(${RE_PROJECT_LIB_NAME}
    PROPERTIES
    SUFFIX "${TARGET_SUFFIX}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    COMPILE_WARNING_AS_ERROR ${TARGET_WARNINGS_AS_ERRORS}
)
if (${RE_PROJECT}_NATIVE)
    # The bundle is `gta_reversed.app`, with `Contents/MacOS/gta_reversed` as `CFBundleExecutable`.
    set_target_properties(${RE_PROJECT_LIB_NAME}
        PROPERTIES
        OUTPUT_NAME              "gta_reversed"
        MACOSX_BUNDLE_INFO_PLIST "${NATIVE_BUNDLE_INFO_PLIST}"
    )

    # Ad-hoc signature (`-`): Gatekeeper runs the bundle without a developer certificate.
    # Without it macOS reports the binary as damaged once it has been copied around.
    find_program(CODESIGN_EXECUTABLE codesign)
    if (CODESIGN_EXECUTABLE)
        add_custom_command(
            TARGET ${RE_PROJECT_LIB_NAME} POST_BUILD
            COMMAND "${CODESIGN_EXECUTABLE}" --force --deep -s - "$<TARGET_BUNDLE_DIR:${RE_PROJECT_LIB_NAME}>"
            COMMENT "Ad-hoc signing $<TARGET_BUNDLE_DIR:${RE_PROJECT_LIB_NAME}>"
            VERBATIM
        )
    else()
        message(WARNING "`codesign` not found: gta_reversed.app will not be signed")
    endif()
endif()
if (MSVC)
    target_link_options(${RE_PROJECT_LIB_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
endif()

#--------------------------------------------------------------------
# Add dependencies
#--------------------------------------------------------------------

# Adding this here as it has no Conan-center package, and it's header-only
FetchContent_Declare(
    imfilebrowser
    GIT_REPOSITORY https://github.com/AirGuanZ/imgui-filebrowser
    GIT_TAG        47a1884
)
FetchContent_MakeAvailable(imfilebrowser)

if (${RE_PROJECT}_NATIVE)
    # RenderWare for the native build: librw (MIT) pinned to the commit validated by `spikes/librw`.
    # It is fetched into the build tree (`build/<config>/_deps`), never into the repository.
    set(LIBRW_COMMIT "18532c2e13efbc1aa43f0b00b4459831f9414de0" CACHE STRING "librw git commit")
    set(LIBRW_PLATFORM "GL3" CACHE STRING "" FORCE)
    set(LIBRW_GL3_GFXLIB "SDL3" CACHE STRING "" FORCE)
    set(LIBRW_TOOLS OFF CACHE BOOL "" FORCE)
    set(LIBRW_INSTALL OFF CACHE BOOL "" FORCE)
    set(LIBRW_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        librw
        GIT_REPOSITORY https://github.com/aap/librw.git
        GIT_TAG        ${LIBRW_COMMIT}
    )
    FetchContent_MakeAvailable(librw)
endif()

macro(add_dependency)
    foreach (target ${ARGV})
        find_package(${target} CONFIG REQUIRED)

        # Disable warnings of dependencies
        # NOTE: `add_compile_options` has no target/PRIVATE arguments, so on MSVC these
        # end up as ignored directory-wide flags, but a clang build rejects them outright.
        if (NOT ${RE_PROJECT}_NATIVE)
            add_compile_options(${target} PRIVATE
                "$<$<CXX_COMPILER_ID:MSVC>:-w>"
                "$<$<OR:$<CXX_COMPILER_ID:AppleClang,Clang,GCC>,$<CXX_COMPILER_ID:GNU>>:-Wno-everything>"
            )
        endif()
    endforeach()
endmacro()
add_dependency(
    nlohmann_json
    Ogg
    spdlog
    Vorbis
    imgui
    libjpeg-turbo
    Boost
)
# Tracy is a retail-exe-only dependency: the native port compiles the Tracy macros out
# (see `app/platform/native/tracy_compat.h`), so it must not be required there.
if (NOT ${RE_PROJECT}_NATIVE)
    add_dependency(Tracy)
endif()
if (${RE_PROJECT}_USE_SDL3)
    add_dependency(SDL3)
endif()

# Windows-only system libraries; the native build has no equivalent
set(PLATFORM_LINK_LIBS)
if (NOT ${RE_PROJECT}_NATIVE)
    set(PLATFORM_LINK_LIBS ddraw Winmm dxguid strmiids d3d9)
endif()

target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
    nlohmann_json::nlohmann_json
    Ogg::ogg
    spdlog::spdlog
    vorbis::vorbis
    imgui::imgui
    libjpeg-turbo::libjpeg-turbo
    ${PLATFORM_LINK_LIBS}
    Boost::headers
)
if (NOT ${RE_PROJECT}_NATIVE)
    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE Tracy::TracyClient)
endif()
if (${RE_PROJECT}_USE_SDL3)
    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
        SDL3::SDL3
    )
endif()

if (${RE_PROJECT}_NATIVE)
    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
        librw::librw
    )
endif()

if (${RE_PROJECT}_NATIVE)
    set(PLATFORM_INCLUDE_DIRS app/platform/native/ game_sa/RenderWare/librw)
else()
    set(PLATFORM_INCLUDE_DIRS app/platform/win/ app/platform/win/VideoPlayer/)
endif()

target_include_directories(${RE_PROJECT_LIB_NAME} PRIVATE
    .
    /
    toolsmenu/
    game_sa/
    game_sa/Debug/
    game_sa/Animation/
    game_sa/Attractors/

    game_sa/Audio/
    game_sa/Audio/Config
    game_sa/Audio/Entities
    game_sa/Audio/Hardware
    game_sa/Audio/Loaders
    game_sa/Audio/Managers
    game_sa/Audio/Utility

    game_sa/Collision/
    game_sa/Conversations/
    game_sa/Core/
    game_sa/Entity/
    game_sa/Entity/Dummy/
    game_sa/Entity/Object/
    game_sa/Entity/Ped/
    game_sa/Entity/Vehicle/
    game_sa/Enums/
    game_sa/Events/
    game_sa/Frontend/
    game_sa/Fx/
    game_sa/Fx/Blueprints/
    game_sa/Fx/Blueprints/Interp
    game_sa/Interior/
    game_sa/Memory/
    game_sa/Models/
    game_sa/Plant/
    game_sa/Plugins/
    game_sa/Pipelines/
    game_sa/Pipelines/CustomBuilding
    game_sa/Pipelines/CustomCarEnvMap
    game_sa/RenderWare/
    game_sa/RenderWare/rw/
    game_sa/Scripts/
    game_sa/Tasks/
    game_sa/Tasks/TaskTypes/
    game_sa/Tasks/TaskTypes/SeekEntity/

    app/
    app/imgui
    app/platform/
    ${PLATFORM_INCLUDE_DIRS}
    app/securom/

    oswrapper

    libs/imgui
    ${imfilebrowser_SOURCE_DIR}
)

target_sources(${RE_PROJECT_LIB_NAME} PRIVATE
    notsa.natvis
)

if (${RE_PROJECT}_NATIVE)
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE
        NOTSA_NATIVE
        NOTSA_STANDALONE
        NOTSA_USE_SDL3
        FIX_BUGS
        SA_SKINNED_PEDS
        EXTRA_DEBUG_FEATURES
    )
else()
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        USE_GTASA_ALLOCATOR
        EXTRA_DEBUG_FEATURES
        FIX_BUGS
        NOTSA_WINDOWED_MODE # NOTE: This is a workaround for the game to run in windowed mode, and it must be enabled, otherwise shit's gonna crash
        SA_SKINNED_PEDS

        USE_D3D9 # TODO
    )
endif()
target_compile_features(${RE_PROJECT_LIB_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${RE_PROJECT_LIB_NAME} PRIVATE StdInc.cpp)

#--------------------------------------------------------------------
# Configure build options
#--------------------------------------------------------------------
if (${RE_PROJECT}_UNITY_BUILD)
    message(STATUS "Configuring Unity/Jumbo build")
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE NOTSA_UNITY_BUILD)
    set_target_properties(${RE_PROJECT_LIB_NAME}
        PROPERTIES
        UNITY_BUILD TRUE
        UNITY_BUILD_MODE BATCH
        UNITY_BUILD_BATCH_SIZE 16
    )
endif()

if (${RE_PROJECT}_WITH_LTO)
    message(STATUS "Configuring Link Time Optimization")
    set_target_properties(${RE_PROJECT_LIB_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (${RE_PROJECT}_WITH_SANITIZERS)
    message(STATUS "Configuring sanitizers")
   #add_sanitizers(${RE_PROJECT_LIB_NAME}) # TODO
    message(FATAL_ERROR "${RE_PROJECT}_WITH_SANITIZERS is not implemented yet; keep it OFF for now (add_sanitizers() is currently missing).")
endif()

#--------------------------------------------------------------------
# Configure the Audio API
#--------------------------------------------------------------------
if (${RE_PROJECT}_NATIVE)
    # OpenAL Soft backend (Audio/OpenAL + the native bodies of the CAE* hardware classes)
    find_package(OpenAL REQUIRED)
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE USE_OPENAL)
    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE OpenAL::OpenAL)
elseif (${RE_PROJECT}_WITH_OPENAL)
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE USE_OPENAL)

    target_include_directories(${RE_PROJECT_LIB_NAME} PRIVATE
        game_sa/Audio/OpenAL
    )
else()
    target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE USE_DSOUND)

    target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE dsound)
endif()


#--------------------------------------------------------------------
# Configure misc. dependencies for Windows
#--------------------------------------------------------------------
if(WIN32 AND NOT ${RE_PROJECT}_NATIVE)
    if(MSVC)
        # MSVC knows how to find dbghelp on it's own
        target_link_libraries(${RE_PROJECT_LIB_NAME} PRIVATE
            dbghelp
        )

        target_compile_definitions(${RE_PROJECT_LIB_NAME} PRIVATE DIRECTINPUT_VERSION=0x0800)
    else()
        message(FATAL_ERROR "Need to link dbghelp lib for win32")
    endif()
endif()


#--------------------------------------------------------------------
# Install
#--------------------------------------------------------------------
if (${RE_PROJECT}_INSTALL)
    install(
        TARGETS ${RE_PROJECT_LIB_NAME}
        EXPORT ${RE_PROJECT_LIB_NAME}-targets
        RUNTIME DESTINATION "."
    )
    if (MSVC)
        install(FILES $<TARGET_PDB_FILE:${RE_PROJECT_LIB_NAME}> DESTINATION "." OPTIONAL)
    endif()
endif()
