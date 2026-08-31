
-- Say Hello to the AST
target("formatter",function()
  set_kind("binary")

  add_files("$(projectdir)/modules/formatter/*.c")
end)

