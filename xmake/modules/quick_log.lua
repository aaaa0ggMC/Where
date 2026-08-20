-- quick log
target("quick_log", function()
  set_kind("static")
  
  add_files("$(projectdir)/modules/quick_log/*.c")
end)
