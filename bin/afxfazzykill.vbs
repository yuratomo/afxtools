Set ao = CreateObject("afxw.obj")
If WScript.Arguments.Count = 1 Then
  Set wp = GetObject("winmgmts:{impersonationLevel=impersonate}").ExecQuery("select * from Win32_Process where ProcessId='" & WScript.Arguments(0) & "'")
  for each p in wp
    ao.MesPrint p.ProcessId & " - " & p.Name & " Çã≠êßèIóπÇµÇ‹ÇµÇΩÅB"
    p.terminate
  Next
  Set wp = Nothing
Else
  ao.MesPrint "[Usage]"
  ao.MesPrint "  afxfazzykill.vbs pid"
End if
Set ao = Nothing
