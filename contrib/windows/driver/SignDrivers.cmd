inf2cat /os:7_X86,7_X64,8_X86,8_X64,10_X64,10_X86 /driver:.
makecert -r -pe -ss My -n "CN=ReSwitched (self-signed)" -eku 1.3.6.1.5.5.7.3.3 twili.cer
certmgr.exe /add twili.cer /s /r localMachine root
signtool sign /n ReSwitched twili.cat
