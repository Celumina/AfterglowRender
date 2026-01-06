for %%f in (%*) do (
	dxc -T vs_6_0 -E main -spirv -fvk-use-gl-layout -Fo "%%f.spv" "%%f"
)

pause