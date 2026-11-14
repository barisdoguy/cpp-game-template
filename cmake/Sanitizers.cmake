# AddressSanitizer + UndefinedBehaviorSanitizer catch out-of-bounds access,
# use-after-free and UB at runtime. Use in Debug builds and in CI.
function(game_enable_sanitizers target)
    if(NOT GAME_ENABLE_SANITIZERS)
        return()
    endif()
    if(MSVC)
        message(WARNING "GAME_ENABLE_SANITIZERS is only wired up for GCC/Clang")
        return()
    endif()
    target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
endfunction()
