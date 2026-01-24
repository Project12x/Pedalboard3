$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
$arch = "amd64"

# Run the build
cmd /c "call `"$vsDevCmd`" -arch=$arch && cmake --build build_nmake_final --config Debug"
