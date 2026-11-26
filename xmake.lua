add_rules("mode.release")

option("target_type")
    set_default("client")
    set_values("server", "client")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")
add_requires("levilamina 26.60.0", {configs = {target_type = get_config("target_type") or "client"}})
add_requires("levibuildscript")

set_runtimes("MD")
set_toolchains("clang-cl")

target("NoDelayMine")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker", {modVersion = "26.60.1"})
    add_shflags("/DELAYLOAD:bedrock_runtime.dll", {force = true})
    add_cxflags("/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE")
    add_packages("levilamina")
    set_kind("shared")
    set_languages("c++20")
    add_files("src/**.cpp")
    add_includedirs("src")
