add_rules("mode.debug","mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputDir = "."})

add_includedirs("include")


-- hello world project
includes("xmake/modules/hello_world.lua")
