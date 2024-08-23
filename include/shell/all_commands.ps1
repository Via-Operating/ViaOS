# Translated to powershell by bradinator

# DON'T DELETE THIS! THIS IS AN IMPORTANT FILE!
$header = "viaSh.h"

"" | Out-File -FilePath $header -Encoding UTF8

Add-Content -Path $header -Value "// DON'T DELETE THIS FILE!"
Add-Content -Path $header -Value "#ifndef __ALL_HEADERS__"
Add-Content -Path $header -Value "#define __ALL_HEADERS__"

Get-ChildItem -Path "term\viaSh" -Filter "*.h" | ForEach-Object {
    Add-Content -Path $header -Value "#include <via/shell/$($_.Name)>"
}

Add-Content -Path $header -Value "#endif"
