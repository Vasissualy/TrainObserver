#include "scene_manager.h"
#include "skybox.h"
#include "space.h"
#include "space_renderer.h"





SceneManager::SceneManager():
	m_renderer(new SpaceRenderer())
{
}


SceneManager::~SceneManager()
{
}

bool SceneManager::init(RendererDX9& renderer)
{
	const char* skyTextures[] = { 
		"maps/SkyBox_Front.jpg",
		"maps/SkyBox_Back.jpg",
		"maps/SkyBox_Left.jpg",
		"maps/SkyBox_Right.jpg",
		"maps/SkyBox_Top.jpg",
		"maps/SkyBox_Bottom.jpg" };

	bool result = true;

	m_skybox.reset(new SkyBox);
	result |= m_skybox->create(renderer, skyTextures);

	m_space.reset(new Space());

	renderer.addRenderItem(this);

	m_renderer->setupStaticScene();

	return result;
}

Space& SceneManager::space()
{
	return *m_space;
}

void SceneManager::draw(RendererDX9& renderer)
{
	m_skybox->draw(renderer);
	m_renderer->draw(renderer);
}

void SceneManager::tick(float deltaTime)
{
}