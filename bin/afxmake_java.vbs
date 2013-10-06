Set para = Wscript.Arguments
if para.Count = 1 then
  Set ao = Createobject("afxw.obj")
  ao.Exec "&EXCD -O"""+para(0)+""""
  ao.Exec "&TOW"
  Set ao = Nothing
end if
