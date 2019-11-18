@echo on
set APPVEYOR_REPO_NAME=lutraconsulting/input

if [%APPVEYOR_REPO_TAG%]==[true] (
    set APK_FILE=inputapp-win-x86_64-%APPVEYOR_REPO_TAG_NAME%.exe
) else (
    set APK_FILE=inputapp-win-x86_64-%APPVEYOR_REPO_COMMIT%.exe
)
set SRC_FILE=C:\projects\input\x86_64\inputapp-win-x86_64.exe
if not exist %SRC_FILE% (echo missing_result & goto error)

xcopy %SRC_FILE% %APK_FILE% /Y

if [%APPVEYOR_PULL_REQUEST_TITLE%] != [] (
    echo "Deploying pull request
    set DROPBOX_FOLDER="pulls"
    set GITHUB_API=https://api.github.com/repos/%APPVEYOR_REPO_NAME%/issues/%APPVEYOR_PULL_REQUEST_NUMBER%/comments
) else if [%APPVEYOR_REPO_TAG%]==[true] (
    echo "Deploying tagged release"
    set DROPBOX_FOLDER="tags"
    set GITHUB_API=https://api.github.com/repos/%APPVEYOR_REPO_NAME%/commits/%APPVEYOR_REPO_COMMIT%/comments
) else if [%APPVEYOR_REPO_BRANCH%] == [master] (
    echo "Deploying master branch"
    set DROPBOX_FOLDER="master"
    set GITHUB_API=https://api.github.com/repos/%APPVEYOR_REPO_NAME%/commits/%APPVEYOR_REPO_COMMIT%/comments
) else (
    echo "Deploying other commit"
    set DROPBOX_FOLDER="other"
    set GITHUB_API=https://api.github.com/repos/%APPVEYOR_REPO_NAME%/commits/%APPVEYOR_REPO_COMMIT%/comments
)

rem do not leak DROPBOX_TOKEN
echo "C:\Python36-x64\python ./scripts/uploader.py --source %APK_FILE% --destination "/%DROPBOX_FOLDER%/%APK_FILE%""
@echo off
%PYEXE% ./scripts/uploader.py --source %APK_FILE% --destination "/%DROPBOX_FOLDER%/%APK_FILE%" --token %DROPBOX_TOKEN% > uploader.log
@echo off

tail -n 1 uploader.log > last_line.log
set /p APK_URL= < last_line.log

rem do not leak GITHUB_TOKEN
@echo off
curl -u inputapp-bot:%GITHUB_TOKEN% -X POST --data '{"body": "win-apk: [x86_64]('%APK_URL%') (SDK: ['%WINSDKTAG%'](https://github.com/lutraconsulting/input-sdk/releases/tag/'%WINSDKTAG%'))"}' %GITHUB_API%
@echo off

echo "all done!"
goto end
:error
echo ENV ERROR %ERRORLEVEL%: %DATE% %TIME%
echo "error!"
exit /b 1

:end