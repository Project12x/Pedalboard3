$vsDevCmd = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
$arch = "amd64"

# Create a cmd process to run the batch file and then the cmake command
# We use cmd /c to run the batch file and then output the environment 
cmd /c "call `"$vsDevCmd`" -arch=$arch && cmake -S . -B build_nmake_final -G `"NMake Makefiles`""
