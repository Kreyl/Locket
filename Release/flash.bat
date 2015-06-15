:start
"c:\ST-LINK\ST-LINK Utility\ST-LINK_CLI.exe" -c SWD -P Locket.hex 0x08000000 -Run

@REM pause

@ping 10.25.11.254 -n 1 -w 2000 > nul

goto start