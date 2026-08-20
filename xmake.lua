add_rules("mode.debug","mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputDir = "."})

add_includedirs("include")

-- Say Hello to the world
target("hello_world",function()
  set_kind("binary")
  add_files("modules/hello_world/*.c")
end)

