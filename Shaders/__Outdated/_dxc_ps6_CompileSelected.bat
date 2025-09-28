for %%f in (%*) do (
	dxc -T ps_6_0 -E main -spirv -fvk-use-gl-layout -Fo "%%f.spv" "%%f"
)

pause