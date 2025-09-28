for %%f in (%*) do (
	glslc "%%f" -o "%%f.spv"
)

pause