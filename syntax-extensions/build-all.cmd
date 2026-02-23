cd VSCode
mkdir syntax
copy ..\fraze-syntax.json syntax\fraze-syntax.json
copy ..\fraze-language-configuration.json syntax\fraze-language-configuration.json
call vsce package
rmdir /s /q .\syntax
move fraze-syntax-1.0.0.vsix ..\fraze-syntax-vscode.vsix

cd ../VisualStudio
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
msbuild FrazeSyntax.sln /p:Configuration=Release /t:Clean;Rebuild
move bin\Release\fraze-syntax-visualstudio.vsix ..\fraze-syntax-visualstudio.vsix
