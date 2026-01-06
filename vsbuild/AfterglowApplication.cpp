#include "AfterglowApplication.h"

#include "AfterglowStaticMeshComponent.h"
#include "AfterglowCameraComponent.h"
#include "ActionComponentLibrary.h"
#include "AfterglowMaterialManager.h"
#include "LocalClock.h"
#include "Tests.h"
#include "VertexStructs.h"
AfterglowApplication::AfterglowApplication() : 
	_window(),
	_renderer(_window), 
	_system(_window, _renderer.materialManager(), _renderer.ui()) {

	// reflectionTest::test();
	// structLayoutTest::test();

	//DEBUG_ERROR(
	//		"\n" + AfterglowMaterialAsset("Assets/Shared/Materials/PostProcess.mat").generateShaderCode(
	//		shader::Stage::Fragment
	//		)
	//);

	auto& materialManager = _renderer.materialManager();

	_renderer.bindRenderableContext(_system.renderableContext());

	DEBUG_WARNING("UnlitMaterial");
	auto& unlitMaterial = materialManager.createMaterial("Unlit", AfterglowMaterial::defaultMaterial());
	unlitMaterial.setScalar(shader::Stage::Shared, "TEST", 1.0);
	unlitMaterial.setScalar(shader::Stage::Shared, "TEST1", 4.0);
	unlitMaterial.setScalar(shader::Stage::Shared, "TEST2", 1321.232);
	unlitMaterial.setScalar(shader::Stage::Shared, "TEST3", 16.0);
	materialManager.submitMaterial("Unlit");

	//  Material assets.
	// Asset monitor will listening modifications of these assets.
	std::string standardMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/Standard.mat");
	std::string skySphereMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/SkySphere.mat");
	std::string postProcessMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/PostProcess.mat");
	
	// Create skyshpere
	auto& skySphere = _system.createEntity<AfterglowStaticMeshComponent>("SkySphere");
	skySphere.get<AfterglowTransformComponent>().setScaling({0.01f, 0.01f, 0.01f});
	auto& skySphereMesh = skySphere.get<AfterglowStaticMeshComponent>();
	skySphereMesh.setModel("Assets/Shared/Models/SkySphere.fbx");
	skySphereMesh.addImportFlags(model::ImportFlag::IgnoreLighting | model::ImportFlag::IgnoreVertexColor);
	auto skyMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Shared/MaterialInstances/DefaultSky.mati"
	);
	skySphereMesh.setMaterial(skyMaterialName);
	// Remind that createEntity may reallocte memory, cause dangling handle, so just do all of these thing in its own slope.
	// e.g. create skySphere -> get<skySphereMesh> -> create battleMage, now the skySphereMesh& is dangling, require to skySphere->get<...>() again.

	// Create battle mage's static mesh.
	auto& battleMage = _system.createEntity<AfterglowStaticMeshComponent>("BattleMage");
	auto& battleMageTransform = battleMage.get<AfterglowTransformComponent>();
	battleMageTransform.setScaling({ 0.01f, 0.01f, 0.01f });
	battleMageTransform.setTranslation({0.0f, 0.0f, 0.0f});
	auto& battleMageMesh = battleMage.get<AfterglowStaticMeshComponent>();
	battleMageMesh.setModel("Assets/Characters/BattleMage/BattleMage.fbx");
	auto battleMageMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/MaterialInstances/BattleMage.mati"
	);
	auto battleMageMaterialWeaponName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/MaterialInstances/BattleMageWeapon.mati"
	);
	battleMageMesh.setMaterial(battleMageMaterialName);
	battleMageMesh.setMaterial(battleMageMaterialWeaponName, 2);
	battleMageMesh.addImportFlags(model::ImportFlag::GenerateTangent);
	_system.addComponent<acl::EntityRotator>(battleMage);

	// @deprecated: Produral terrain is coming.
	// Create terrain
	auto& terrain = _system.createEntity<AfterglowStaticMeshComponent>("Terrain");
	terrain.get<AfterglowStaticMeshComponent>().setModel("Assets/Shared/Models/Terrain.fbx");
	terrain.get<AfterglowStaticMeshComponent>().disable();

	// Create test box
	_system.createEntity<>("EmptyEntity", battleMage);

	auto& box = _system.createEntity<AfterglowStaticMeshComponent, acl::EntityRotator>("Box", battleMage);
	auto& boxTransform = box.get<AfterglowTransformComponent>();
	boxTransform.setScaling({40.0f, 20.0f, 20.0f});
	boxTransform.setTranslation({ 80.0f,-20.0f, 50.0f });
	boxTransform.setEuler({0.0_deg, 15.0_deg , 45.0_deg });
	// boxTransform.setGlobalScaling({ 20.0f, 20.0f, 20.0f });
	auto& boxMesh = box.get<AfterglowStaticMeshComponent>();
	boxMesh.setModel("Assets/Shared/Models/Box.fbx");
	boxMesh.setMaterial(battleMageMaterialWeaponName);

	auto& boxB = _system.createEntity<AfterglowStaticMeshComponent, acl::EntityRotator>("BoxB", box);
	auto& boxBTransform = boxB.get<AfterglowTransformComponent>();
	boxBTransform.setGlobalScaling({ 1.0f, 1.0f, 1.0f });
	boxBTransform.setTranslation({ 0.0f, -40.0f, 0.0f });
	auto& boxBMesh = boxB.get<AfterglowStaticMeshComponent>();
	boxBMesh.setModel("Assets/Shared/Models/Box.fbx");

	// Create test ball
	auto& ball = _system.createEntity<AfterglowStaticMeshComponent>("Ball");
	auto& ballMesh = ball.get<AfterglowStaticMeshComponent>();
	ballMesh.setModel("Assets/Shared/Models/Sphere.fbx");
	ballMesh.setMaterial(battleMageMaterialWeaponName);
	auto& ballTransform = ball.get<AfterglowTransformComponent>();
	ballTransform.setScaling({ 1.0f, 1.0f, 1.0f});
	ballTransform.setTranslation({ 4.0f, 0.0f, 2.0f });

	// Runtime mateiral test.
	//auto& battleMageMaterial = materialManager.createMaterialInstance("BattleMageMaterial", "Unlit");
	//auto& battleMageWeaponMaterial = materialManager.createMaterialInstance("BattleMageWeaponMaterial", "Unlit");
	//battleMageMaterial.setTexture("albedo", "Assets/Characters/BattleMage/Material/Color01/19A_Body_Sss.png");
	//battleMageWeaponMaterial.setTexture("albedo", "Assets/Characters/BattleMage/Material/Color01/19A_Body_W00_Sss.png");

	//battleMageMesh.setMaterialInstance("BattleMageMaterial");
	//battleMageMesh.setMaterialInstance("BattleMageWeaponMaterial..", 2);

	// Initialize main camera.
	auto& mainCamera = _system.createEntity<AfterglowCameraComponent, acl::SimpleController>("MainCamera");
	auto& mainCameraTransform = mainCamera.get<AfterglowTransformComponent>();
	mainCameraTransform.setTranslation({0.f, 2.0f, 1.0f});
	mainCamera.get<AfterglowCameraComponent>().setTarget(
		battleMage.get<AfterglowTransformComponent>().globalTranslation() + AfterglowTranslation{0.0f, 0.0f,  0.8f}
	);

	auto& otherCamera = _system.createEntity<AfterglowCameraComponent>("OtherCamera", boxB);
	auto& otherCameraTransform = otherCamera.get<AfterglowTransformComponent>();
	otherCameraTransform.setTranslation({0.0f, 20.0f, 0.5f });
	otherCamera.get<AfterglowCameraComponent>().setGlobalTarget({ 0.0f, 0.0f, 0.8f });

	_system.removeComponent<AfterglowCameraComponent>(otherCamera);

	// And God said, Let there be light: and there was light component. 
	auto& directionalLight = _system.createEntity<AfterglowDirectionalLightComponent>("DirectionalLight");
	auto& diectionalLightTransform = directionalLight.get<AfterglowTransformComponent>();
	diectionalLightTransform.setGlobalEuler({45.0_deg, 45.0_deg, 30.0_deg});
	directionalLight.get<AfterglowDirectionalLightComponent>().setIntensity(constant::pi);
	directionalLight.get<AfterglowDirectionalLightComponent>().setColor(0xFFEEDD00);
	
	// ACES Precomputed tables.
	std::string acesTablesMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ACESTables.mat");
	auto& acesTables = _system.createEntity<AfterglowComputeComponent>("ACESTables");
	acesTables.get<AfterglowComputeComponent>().setComputeMaterial(acesTablesMaterialName);

	// Post process component.
	auto& postProcessMaterialInstance = materialManager.createMaterialInstance("DefaultPostProcessMaterial", postProcessMaterialName);
	auto& postprocess = _system.createEntity<AfterglowPostProcessComponent, AfterglowComputeComponent>("PostProcess");
	postprocess.get<AfterglowPostProcessComponent>().setPostProcessMaterial("DefaultPostProcessMaterial");
	postprocess.get<AfterglowComputeComponent>().setComputeMaterial("DefaultPostProcessMaterial");

	// Interactive test.
	auto& interactiveTest = _system.createEntity<acl::InteractiveTest>("InteractiveTest");

	// Compute component test
	// Meteorograph
	std::string meteorographMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/Meteorograph.mat");
	auto& meteorograph = _system.createEntity<AfterglowComputeComponent>("Meteorograph");
	meteorograph.get<AfterglowComputeComponent>().setComputeMaterial(meteorographMaterialName);

	// Boid instancings.
	std::string boidInstancingMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/BoidInstancing.mat");
	auto& boids = _system.createEntity<AfterglowStaticMeshComponent, AfterglowComputeComponent>("Boids");
	boids.get<AfterglowTransformComponent>().setScaling({ 0.2f, 0.2f, 0.2f });
	auto& boidMesh = boids.get<AfterglowStaticMeshComponent>();
	boidMesh.setModel("Assets/Shared/Models/PaperAirplane.fbx");
	boidMesh.setMaterial(boidInstancingMaterialName);
	boids.get<AfterglowComputeComponent>().setComputeMaterial(boidInstancingMaterialName);

	// Generated fractal noise
	std::string fractalNoiseMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/FractalNoise.mat");
	auto& fractalNoise = _system.createEntity<AfterglowComputeComponent>("FractalNoise");
	fractalNoise.get<AfterglowComputeComponent>().setComputeMaterial(fractalNoiseMaterialName);

	// Terrain data
	std::string terrainDataMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/TerrainData.mat");
	auto& terrainData = _system.createEntity<AfterglowComputeComponent>("TerrainData");
	terrainData.get<AfterglowComputeComponent>().setComputeMaterial(terrainDataMaterialName);

	// Terrain mesh
	// std::string terrainMeshMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Shared/MaterialInstances/TerrainMesh.mati");
	std::string terrainMeshMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/TerrainMesh.mat");
	auto& terrainMesh = _system.createEntity<AfterglowComputeComponent>("TerrainMesh");
	terrainMesh.get<AfterglowComputeComponent>().setComputeMaterial(terrainMeshMaterialName);

	// Water mesh
	std::string waterMeshMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/WaterMesh.mat");
	auto& waterMesh = _system.createEntity<AfterglowComputeComponent>("WaterMesh");
	waterMesh.get<AfterglowComputeComponent>().setComputeMaterial(waterMeshMaterialName);

	// ParticleExample
	std::string particleExampleMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ParticleExample.mat");
	auto& particleExample = _system.createEntity<AfterglowComputeComponent>("ParticleExample");
	particleExample.get<AfterglowComputeComponent>().setComputeMaterial(particleExampleMaterialName);

	// Grass data
	std::string grassDataMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/GrassData.mat");
	auto& grassData = _system.createEntity<AfterglowComputeComponent>("GrassData");
	grassData.get<AfterglowComputeComponent>().setComputeMaterial(grassDataMaterialName);

	// Grass instancing
	std::string grassInstancingMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/GrassInstancing.mat");
	auto& grassInstances = _system.createEntity<AfterglowStaticMeshComponent, AfterglowComputeComponent>("GrassInstances");
	grassInstances.get<AfterglowTransformComponent>().setScaling({ 1.0f, 1.0f, 1.0f });
	auto& grassInstancesMesh = grassInstances.get<AfterglowStaticMeshComponent>();
	grassInstancesMesh.setModel("Assets/Shared/Models/InstancingGrass.fbx");
	grassInstancesMesh.addImportFlags(model::ImportFlag::IgnoreNormalMap | model::ImportFlag::IgnoreTextureMapping);
	grassInstancesMesh.setMaterial(grassInstancingMaterialName);
	// TODO: Try to use a simplified model contains pos, normal, color only.
	grassInstances.get<AfterglowComputeComponent>().setComputeMaterial(grassInstancingMaterialName);

	DEBUG_WARNING("---------------------");
}

void AfterglowApplication::run() {
	 _system.startSystemThread();
	 _renderer.startRenderThread();

	while (!_window.shouldClose()) {
		_window.update();
		// TODO: Update GlobalClock in SceneThread.
		// std::cout << GlobalClock::fps() << '\n';
	}

	 _renderer.stopRenderThread();
	 _system.stopSystemThread();
}