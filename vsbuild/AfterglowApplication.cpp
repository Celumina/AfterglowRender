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
	_renderer.bindRenderableContext(_system.renderableContext());

	// Test Scene
	auto& materialManager = _renderer.materialManager();

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
	std::string ignoreMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/Ignore.mat");
	std::string skySphereMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/SkySphere.mat");
	
	// Create skyshpere
	auto& skySphere = _system.createEntity<AfterglowStaticMeshComponent>("SkySphere");
	skySphere.get<AfterglowTransformComponent>().setScaling({0.01f, 0.01f, 0.01f});
	auto& skySphereMesh = skySphere.get<AfterglowStaticMeshComponent>();
	skySphereMesh.setModel("Assets/Shared/Models/SkySphere.fbx");
	skySphereMesh.setProperty(renderable::Property::DynamicCulling, false);
	skySphereMesh.addImportFlags(model::ImportFlag::IgnoreLighting | model::ImportFlag::IgnoreVertexColor);
	auto skyMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Shared/MaterialInstances/DefaultSky.mati"
	);
	skySphereMesh.setMaterial(skyMaterialName);
	// Remind that createEntity may reallocte memory, cause dangling handle, so just do all of these thing in its own slope.
	// e.g. create skySphere -> get<skySphereMesh> -> create battleMage, now the skySphereMesh& is dangling, require to skySphere->get<...>() again.

	// Create battle mage's static mesh.
	std::string arcToonMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ArcToon.mat");
	std::string arcOutlineMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ArcOutline.mat");
	std::string arcCardShadowMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/arcCardShadow.mat");
	auto& battleMage = _system.createEntity<AfterglowStaticMeshComponent>("BattleMage");
	auto& battleMageTransform = battleMage.get<AfterglowTransformComponent>();
	battleMageTransform.setScaling({ 0.01f, 0.01f, 0.01f });
	battleMageTransform.setTranslation({0.0f, 0.0f, 0.0f});
	auto& battleMageMesh = battleMage.get<AfterglowStaticMeshComponent>();
	battleMageMesh.setModel("Assets/Characters/BattleMage/BattleMage.fbx");
	battleMageMesh.setDrawCount(2);
	std::string battleMageMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/Materials/BattleMage.mati"
	);
	std::string battleMageCardShadowMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/Materials/BattleMageCardShadow.mati"
	);
	std::string battleMageMaterialWeaponName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/Materials/BattleMageWeapon.mati"
	);
	std::string battleMageOutlineMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/BattleMage/Materials/BattleMageOutline.mati"
	);
	battleMageMesh.setMaterial(battleMageMaterialName);
	battleMageMesh.setMaterial(battleMageCardShadowMaterialName, 1);
	battleMageMesh.setMaterial(battleMageMaterialWeaponName, 2);
	battleMageMesh.setMaterial(battleMageOutlineMaterialName, 0, 1);
	battleMageMesh.addImportFlags(model::ImportFlag::GenerateTangent);
	_system.addComponent<acl::EntityRotator>(battleMage);

	// Create test boxes
	_system.createEntity<>("EmptyEntity", battleMage);

	auto& box = _system.createEntity<AfterglowStaticMeshComponent, acl::EntityRotator>("Box", battleMage);
	auto& boxTransform = box.get<AfterglowTransformComponent>();
	boxTransform.setScaling({ 40.0f, 20.0f, 20.0f });
	boxTransform.setTranslation({ 80.0f,-20.0f, 50.0f });
	boxTransform.setEuler({ 0.0_deg, 15.0_deg , 45.0_deg });
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

	// Yvonne
	std::string endfieldBodyMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldBody.mat");
	std::string endfieldPBRMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldPBR.mat");
	std::string endfieldThinFilmMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldThinFilm.mat");
	std::string endfieldEyeShadowMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldEyeShadow.mat");
	std::string endfieldFaceMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldFace.mat");
	std::string endfieldEyelashMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldEyelash.mat");
	std::string endfieldIrisMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldIris.mat");
	std::string endfieldHairMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldHair.mat");
	std::string endfieldHairShadowMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldHairShadow.mat");
	std::string endfieldOutlineMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/EndfieldOutline.mat");
	auto& yvonne = _system.createEntity<AfterglowStaticMeshComponent, acl::EntityRotator, acl::MaterialObjectStateParamUpdater>("Yvonne");
	yvonne.get<AfterglowTransformComponent>().setGlobalTranslation({ 0.0f, 4.0f, 0.1f});
	auto& yvonneMesh = yvonne.get<AfterglowStaticMeshComponent>();
	//yvonneMesh.addImportFlags(model::ImportFlag::PNTBCT0);
	yvonneMesh.setDrawCount(2);
	yvonneMesh.setModel("Assets/Characters/Yvonne/Yvonne.fbx");
	std::string yvonneBodyMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneBody.mati"
	);
	std::string yvonneNeckMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneNeck.mati"
	);
	std::string yvonneClothMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneCloth.mati"
	);
	std::string yvonneTailMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneTail.mati"
	);
	std::string yvonneThinFilmMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneThinFilm.mati"
	);
	std::string yvonneEyeShadowMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneEyeShadow.mati"
	);
	std::string yvonneFaceMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneFace.mati"
	);
	std::string yvonneEyelashMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneEyelash.mati"
	);
	std::string yvonneIrisMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneIris.mati"
	);
	std::string yvonneHairMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneHair.mati"
	);
	std::string yvonneHairShadowMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneHairShadow.mati"
	);
	std::string yvonneBodyOutlineMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneBodyOutline.mati"
	);
	std::string yvonneClothOutlineMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneClothOutline.mati"
	);
	std::string yvonneHairOutlineMaterialName = materialManager.registerMaterialInstanceAsset(
		"Assets/Characters/Yvonne/Materials/YvonneHairOutline.mati"
	);
	yvonneMesh.setMaterial(yvonneBodyMaterialName, 0);
	yvonneMesh.setMaterial(yvonneNeckMaterialName, 1);
	yvonneMesh.setMaterial(yvonneClothMaterialName, 2);
	yvonneMesh.setMaterial(yvonneTailMaterialName, 3);
	yvonneMesh.setMaterial(yvonneThinFilmMaterialName, 4);
	yvonneMesh.setMaterial(yvonneClothMaterialName, 5);
	yvonneMesh.setMaterial(yvonneEyeShadowMaterialName, 6);
	yvonneMesh.setMaterial(yvonneFaceMaterialName, 7);
	yvonneMesh.setMaterial(yvonneEyelashMaterialName, 8);
	yvonneMesh.setMaterial(yvonneIrisMaterialName, 9);
	yvonneMesh.setMaterial(yvonneHairMaterialName, 10);
	yvonneMesh.setMaterial(yvonneHairShadowMaterialName, 11);
	yvonneMesh.setMaterial(yvonneBodyOutlineMaterialName, 0, 1);
	yvonneMesh.setMaterial(yvonneClothOutlineMaterialName, 2, 1);
	yvonneMesh.setMaterial(yvonneClothOutlineMaterialName, 3, 1);
	yvonneMesh.setMaterial(ignoreMaterialName, 4, 1); // Clip the thin film outline
	yvonneMesh.setMaterial(yvonneClothOutlineMaterialName, 5, 1);
	yvonneMesh.setMaterial(yvonneHairOutlineMaterialName, 10, 1);
	// TODO: Updateder support multiple mats
	auto& yvonneMaterialUpdater = yvonne.get<acl::MaterialObjectStateParamUpdater>();
	yvonneMaterialUpdater.bindMaterial(endfieldFaceMaterialName);
	yvonneMaterialUpdater.bindMaterialInstance(yvonneFaceMaterialName);

	// @deprecated: Produral terrain is coming.
	// Create terrain
	auto& terrain = _system.createEntity<AfterglowStaticMeshComponent>("Terrain");
	terrain.get<AfterglowStaticMeshComponent>().setModel("Assets/Shared/Models/Terrain.fbx");
	terrain.get<AfterglowStaticMeshComponent>().disable();

	// Create test ball
	auto ballPBRMaterial = materialManager.registerMaterialInstanceAsset("Assets/Shared/MaterialInstances/BallPBR.mati");
	auto& ball = _system.createEntity<AfterglowStaticMeshComponent>("Ball");
	auto& ballMesh = ball.get<AfterglowStaticMeshComponent>();
	ballMesh.setModel("Assets/Shared/Models/Sphere.fbx");
	ballMesh.setMaterial(ballPBRMaterial);
	auto& ballTransform = ball.get<AfterglowTransformComponent>();
	ballTransform.setScaling({ 1.0f, 1.0f, 1.0f});
	ballTransform.setTranslation({ 4.0f, 0.0f, 2.0f });

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
	auto& directionalLight = _system.createEntity<AfterglowDirectionalLightComponent, acl::EntityRotator>("DirectionalLight");
	auto& diectionalLightTransform = directionalLight.get<AfterglowTransformComponent>();
	diectionalLightTransform.setGlobalEuler({45.0_deg, 45.0_deg, 30.0_deg});
	directionalLight.get<AfterglowDirectionalLightComponent>().setIntensity(constant::pi);
	directionalLight.get<AfterglowDirectionalLightComponent>().setColor(0xFFEEDD00);
	directionalLight.get<acl::EntityRotator>().setAngularSpeed(0.0f);
	
	// ACES Precomputed tables.
	std::string acesTablesMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ACESTables.mat");
	auto& acesTables = _system.createEntity<AfterglowComputeComponent>("ACESTables");
	acesTables.get<AfterglowComputeComponent>().setComputeMaterial(acesTablesMaterialName);

	// Post process component.
	// It's not necessary to create a material manually, material and compute component were created in PostProcess::awake() 
	auto& postprocess = _system.createEntity<AfterglowPostProcessComponent>("PostProcess");

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
	boidMesh.setProperty(renderable::Property::DynamicCulling, false);
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
	grassInstancesMesh.setProperty(renderable::Property::DynamicCulling, false);
	// TODO: Try to use a simplified model contains pos, normal, color only.
	grassInstances.get<AfterglowComputeComponent>().setComputeMaterial(grassInstancingMaterialName);

	// Furry character
	std::string ankhaMaterial = materialManager.registerMaterialAsset("Assets/Characters/Ankha/Materials/Ankha.mat");
	std::string ankhaBodyMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Characters/Ankha/Materials/AnkhaBody.mati");
	std::string ankhaHairMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Characters/Ankha/Materials/AnkhaHair.mati");
	std::string ankhaEyeMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Characters/Ankha/Materials/AnkhaEye.mati");
	std::string ankhaDressMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Characters/Ankha/Materials/AnkhaDress.mati");
	auto& ankhaBody = _system.createEntity<AfterglowStaticMeshComponent>("Ankha");
	auto& ankhaBodyTransform = ankhaBody.get<AfterglowTransformComponent>();
	ankhaBodyTransform.setGlobalTranslation({ 0.0f, 6.0f, 0.5f });
	//ankhaBodyTransform.setEuler({ 90_deg, 0_deg, 0_deg });
	ankhaBodyTransform.setScaling({ 0.02f, 0.02f, 0.02f });
	auto& ankhaBodyMesh = ankhaBody.get<AfterglowStaticMeshComponent>();
	ankhaBodyMesh.setModel("Assets/Characters/Ankha/Ankha.fbx");
	ankhaBodyMesh.setMaterial(ankhaDressMaterialName);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 1);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 2);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 3);
	ankhaBodyMesh.setMaterial(ankhaEyeMaterialName, 4);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 5);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 6);
	ankhaBodyMesh.setMaterial(ankhaEyeMaterialName, 7);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 8);
	ankhaBodyMesh.setMaterial(ankhaHairMaterialName, 9);
	ankhaBodyMesh.setMaterial(ankhaBodyMaterialName, 10);

	std::string shellFurMaterialName = materialManager.registerMaterialAsset("Assets/Shared/Materials/ShellFur.mat");
	uint32_t ankhaFurInstanceCount = 8;
	std::string ankhaFurMaterialName = materialManager.registerMaterialInstanceAsset("Assets/Characters/Ankha/Materials/AnkhaFur.mati");
	materialManager.materialInstance(ankhaFurMaterialName)->setScalar(shader::Stage::Shared, "instanceCount", static_cast<float>(ankhaFurInstanceCount));
	//materialManager.materialInstance(ankhaFurMaterialName)->setScalar(shader::Stage::Shared, "invInstanceCount", 1.0f / ankhaFurInstanceCount);
	auto& ankhaFur = _system.createEntity<AfterglowStaticMeshComponent>("AnkhaFur");

	auto& ankhaFurTransform = ankhaFur.get<AfterglowTransformComponent>();
	ankhaFurTransform.setGlobalTranslation({ 0.0f, 6.0f, 0.5f });
	//ankhaFurTransform.setEuler({ 90_deg, 0_deg, 0_deg });
	ankhaFurTransform.setScaling({ 0.02f, 0.02f, 0.02f });
	auto& ankhaFurMesh = ankhaFur.get<AfterglowStaticMeshComponent>();
	ankhaFurMesh.setModel("Assets/Characters/Ankha/AnkhaFur.fbx");
	ankhaFurMesh.setMaterial(ankhaFurMaterialName);
	ankhaFurMesh.setInstanceCount(ankhaFurInstanceCount);

	DEBUG_WARNING("---------------------");
}

void AfterglowApplication::run() {
	_renderer.startRenderThread();
	 _system.startSystemThread();

	while (!_window.shouldClose()) {
		_window.update();
		// TODO: Update GlobalClock in SceneThread.
		// std::cout << GlobalClock::fps() << '\n';
	}

	_system.stopSystemThread();
	_renderer.stopRenderThread();
}