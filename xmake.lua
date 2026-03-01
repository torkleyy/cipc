
set_project("cipc")
set_version("0.1.0")

set_languages("c11")

add_rules("mode.debug", "mode.release")

-- Optional: export compile_commands.json for tooling (clangd, etc.)
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

-- Global warnings / diagnostics
if is_plat("windows") and is_tool("cc", "cl") then
    -- MSVC
    add_cxflags("/W4", "/permissive-", "/Zc:preprocessor", {force = true})
    -- Treat warnings as errors (comment out if too strict)
    add_cxflags("/WX", {force = true})
else
    -- GCC/Clang
    add_cxflags(
        "-Wall", "-Wextra", "-Wpedantic",
        "-Wshadow", "-Wconversion", "-Wsign-conversion",
        "-Wformat=2", "-Wundef",
        "-Wstrict-prototypes", "-Wmissing-prototypes",
        "-Wold-style-definition",
        {force = true}
    )
    -- Treat warnings as errors (comment out if too strict)
    add_cxflags("-Werror", {force = true})
end

-- Security hardening + UB reduction (release-friendly defaults)
if not (is_plat("windows") and is_tool("cc", "cl")) then
    -- Fortify is meaningful with optimization; enable in release
    if is_mode("release") then
        add_defines("_FORTIFY_SOURCE=2")
        add_cxflags("-O2", {force = true})
    end

    -- Stack protector (works in both modes)
    add_cxflags("-fstack-protector-strong", {force = true})

    -- Linker hardening where supported
    if is_plat("linux") then
        add_ldflags("-Wl,-z,relro", "-Wl,-z,now", {force = true})
        add_ldflags("-Wl,-z,noexecstack", {force = true})
    elseif is_plat("macosx") then
        -- Use Darwin-compatible hardening flags.
        add_ldflags("-Wl,-dead_strip", {force = true})
    end

    -- Position independent executable where explicitly supported
    if is_plat("linux") then
        add_cxflags("-fPIE", {force = true})
        add_ldflags("-pie", {force = true})
    end
else
    -- MSVC hardening-ish knobs
    add_cxflags("/GS", {force = true})        -- buffer security checks
    add_ldflags("/DYNAMICBASE", "/NXCOMPAT", {force = true})
end

-- Developer safety: sanitizers in debug (clang/gcc)
if is_mode("debug") then
    add_defines("DEBUG")
    if not (is_plat("windows") and is_tool("cc", "cl")) then
        -- Address + Undefined sanitizers (great default in debug)
        add_cxflags("-fno-omit-frame-pointer", "-g", {force = true})
        add_cxflags("-fsanitize=address,undefined", {force = true})
        add_ldflags("-fsanitize=address,undefined", {force = true})
    end
end


target("cipc")
  set_kind("static")
  add_files("src/*.c")
  add_includedirs("include", {public = true})

target("server")
  set_kind("binary")
  add_deps("cipc")
  add_files("examples/server.c")

target("client")
  set_kind("binary")
  add_deps("cipc")
  add_files("examples/client.c")
