@echo off

REM This is the temp folder
set "tempFolder=%TEMP%"
set "configFilePath=%tempFolder%\postopek_files.config"

IF EXIST "%configFilePath%" (
    REM Delete the file at configFilePath
    DEL "%configFilePath%"
)
REM Check if an argument is provided
IF "%1"=="" (
    echo ERROR: No directory argument provided.
    exit
)

REM Check if the argument is a valid directory
IF NOT EXIST "%1\." (
    echo ERROR: Not a valid directory.
    exit
)

echo MARKDOWN_FILES_DIR=%1 >> %configFilePath%
echo SUCCESS: CONFIG file saved.