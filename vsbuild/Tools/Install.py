import os
import shutil

installDestDir = "./Install/"
buildRoot = "../"

dependencyDirs = [
	"Assets/", 
	"Shaders/", 
]
releaseExecDirs = [
	"x64/Release/", 
	"Libraries/openimageio-3.0.9.1/bin/", 
]
debugExecDirs = [
	"x64/Debug/", 
	"Libraries/openimageio-3.0.9.1/debug/bin/", 
]

dependentFiles = [
	"Libraries/VulkanSDK-1.3.290.0/Bin/dxcompiler.dll"
]

# For vulkan validation dependencies in debug mode.
validationFiles = [
	"Libraries/VulkanSDK-1.3.290.0/Bin/VkLayer_khronos_validation.dll", 
	"Libraries/VulkanSDK-1.3.290.0/Bin/VkLayer_khronos_validation.json", 
]

import os
import shutil
import glob

installDestDir = "./Install/"
buildRoot = "../"
dependencyDirs = ["Assets/", "Shaders/"]
releaseExecDirs = ["x64/Release/", "Libraries/openimageio-3.0.9.1/bin/"]
debugExecDirs = ["x64/Debug/", "Libraries/openimageio-3.0.9.1/debug/bin/"]

def InstallValidation(folderName):
	# Ensure the destination directory exists
	os.makedirs(installDestDir, exist_ok=True)

	for file in validationFiles:
		srcPath = os.path.join(buildRoot, file) 
		
		# Check if the source file exists
		if not os.path.exists(srcPath):
			print(f"Warning: Source file not found: {srcPath}")
			continue

		relativePath = os.path.relpath(os.path.dirname(file), start=".")
		
		# Construct the destination directory, preserving the hierarchy
		destSubdir = os.path.join(installDestDir, folderName)
		destSubdir = os.path.join(destSubdir, relativePath)
		
		os.makedirs(destSubdir, exist_ok=True)
		
		destPath = os.path.join(destSubdir, os.path.basename(file))
		shutil.copy2(srcPath, destPath)

def InstallDependencies(folderName, execDirs):
	targetDir = os.path.join(installDestDir, folderName)
	os.makedirs(targetDir, exist_ok=True)
	
	# Copy dependency directories
	for depDir in dependencyDirs:
		sourcePath = os.path.join(buildRoot, depDir)
		destPath = os.path.join(targetDir, os.path.basename(os.path.normpath(depDir)))
		if os.path.exists(sourcePath):
			shutil.copytree(sourcePath, destPath, dirs_exist_ok=True)
	
	# Copy .dll and .exe files
	for execDir in execDirs:
		sourcePath = os.path.join(buildRoot, execDir)
		if os.path.exists(sourcePath):
			for filePath in glob.glob(os.path.join(sourcePath, "*.dll")) + \
							glob.glob(os.path.join(sourcePath, "*.exe")):
				if os.path.isfile(filePath):
					shutil.copy2(filePath, targetDir)

	for dependentFile in dependentFiles:
		srcPath = os.path.join(buildRoot, dependentFile)
		# if os.path.exists(srcPath):
		# 	print(srcPath)
		# 	continue
		shutil.copy2(srcPath, os.path.join(targetDir, os.path.basename(dependentFile)))
		

def InstallDebug():
	InstallDependencies("Debug", debugExecDirs)
	InstallValidation("Debug")

def InstallRelease():
	InstallDependencies("Release", releaseExecDirs)

if __name__ == "__main__":
	os.makedirs(installDestDir, exist_ok=True)
	InstallDebug()
	InstallRelease()