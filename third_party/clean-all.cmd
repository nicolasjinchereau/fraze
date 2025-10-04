@echo off
echo removing bin\, lib\, obj\ and .vs\ folders...

cd assimp
if exist obj rmdir /s /q obj
if exist lib rmdir /s /q lib
if exist projects\windows\.vs rmdir /s /q projects\windows\.vs

cd ..\libpng
if exist obj rmdir /s /q obj
if exist lib rmdir /s /q lib
if exist projects\windows\.vs rmdir /s /q projects\windows\.vs

cd ..\zlib
if exist obj rmdir /s /q obj
if exist lib rmdir /s /q lib
if exist projects\windows\.vs rmdir /s /q projects\windows\.vs

cd ..

echo done.
