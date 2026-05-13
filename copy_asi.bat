@echo off
setlocal

set "SRC=%~dp0build\Release\RDR2ChromaSync.asi"
set "DST=I:\games\Red Dead Redemption 2\RDR2ChromaSync.asi"
set "CFG_SRC=%~dp0config.json"
set "CFG_DST=I:\games\Red Dead Redemption 2\RDR2ChromaSync_config.json"

if not exist "%SRC%" (
  echo Source not found: %SRC%
  exit /b 1
)

copy /Y "%SRC%" "%DST%"
if errorlevel 1 (
  echo Copy failed.
  exit /b 1
)

if exist "%CFG_SRC%" (
  copy /Y "%CFG_SRC%" "%CFG_DST%"
  if errorlevel 1 (
    echo Config copy failed.
    exit /b 1
  )
  echo Copied config to: %CFG_DST%
) else (
  echo Config not found: %CFG_SRC%
)

echo Copied to: %DST%
exit /b 0
