set shell=CreateObject("Shell.Application")
' shell.ShellExecute "application", "arguments", "path", "verb", window
shell.ShellExecute "afterburner-kill.bat",,"D:\Simon\Cloud\Programming\GPU Clock\", "runas", 0
set shell=nothing 