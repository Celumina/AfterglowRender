mkdir SPIRV

for %%f in (*.vert) do (
	glslc "%%f" -o ".\\SPIRV\\%%f.spv"
)
for %%f in (*.frag) do (
	glslc "%%f" -o ".\\SPIRV\\%%f.spv"
)
for %%f in (*.comp) do (
	glslc "%%f" -o ".\\SPIRV\\%%f.spv"
)
pause