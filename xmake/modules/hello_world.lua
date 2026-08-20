
-- Say Hello to the world
target("hello_world",function()
  set_kind("binary")
  add_deps("quick_log")

  add_files("$(projectdir)/modules/hello_world/*.c")
end)

