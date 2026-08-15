@echo off
echo building third party libs...

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"

cd assimp\projects\windows
msbuild assimp.sln /p:Configuration=Debug /t:Clean;Rebuild
msbuild assimp.sln /p:Configuration=Release /t:Clean;Rebuild
cd ..\..\..

cd libpng\projects\windows
msbuild libpng.sln /p:Configuration=Debug /t:Clean;Rebuild
msbuild libpng.sln /p:Configuration=Release /t:Clean;Rebuild
cd ..\..\..

cd mir\projects
msbuild MIR.sln /p:Configuration=Debug /t:Clean;Rebuild
msbuild MIR.sln /p:Configuration=Release /t:Clean;Rebuild
cd ..\..

cd zlib\projects\windows
msbuild zlib.sln /p:Configuration=Debug /t:Clean;Rebuild
msbuild zlib.sln /p:Configuration=Release /t:Clean;Rebuild
cd ..\..\..

echo done.
