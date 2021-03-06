#include "space_renderer.h"
#include "model.h"
#include "box.h"
#include "render_dx9.h"
#include "resource_manager.h"
#include <fstream>

const std::string RAIL_PATH = "content/meshes/rail/rail.obj";
const std::string CITY_PATH = "content/meshes/cities/";
const std::string TRAIN_PATH = "content/meshes/trains/";
const std::string SHADER_LIGHTONLY_PATH = "content/shaders/lightonly.fx";
const std::string SHADER_NORMALMAP_PATH = "content/shaders/normalmap.fx";
const std::string TERRAIN_DIFFUSE_TEXTURE_PATH = "content/maps/terrain.dds";
const std::string TERRAIN_NORMAL_TEXTURE_PATH = "content/maps/terrain_normal.jpg";

const uint CITY_COUNT = 3;
const uint TRAIN_COUNT = 1;
const float RAIL_CONNECTION_OFFSET = 1.0f - 0.01f;
const float RAIL_SCALE = 30.0f;

struct SunLight
{
	Vector3 dir;
	Vector3 color;
	float  scale;
	float  power;

	SunLight()
	{
		memset(this, 0, sizeof(SunLight));
	}
};

class SunEffectProperty : public IEffectProperty
{

public:
	SunEffectProperty(const SunLight& sun) :
		IEffectProperty("g_sunLight"),
		m_sun(sun) {}

	virtual bool applyProperty(LPD3DXEFFECT pEffect) const override
	{
		return SUCCEEDED(pEffect->SetValue(m_name.c_str(), &m_sun, sizeof(SunLight)));
	}

private:
	const SunLight& m_sun;

};

SpaceRenderer::SpaceRenderer():
	m_sun(new SunLight)
{
	m_sun->color = Vector3(1.0f, 0.9f, 0.5f); // light yellow 
	m_sun->dir = Vector3(0.5f, -0.15f, 0.85f);
	m_sun->dir.Normalize();
	m_sun->scale = 1.0f;
	m_sun->power = 10.0f;

	RenderSystemDX9::instance().globalEffectProperties().addProperty(GLOBAL, new SunEffectProperty(*m_sun.get()));
}


SpaceRenderer::~SpaceRenderer()
{
}

void SpaceRenderer::draw(class RendererDX9& renderer)
{
	m_terrain->draw(renderer);

	auto& camera = RenderSystemDX9::instance().renderer().camera();
	camera.beginZBIASDraw(1.001f);

	for (const auto& obj : m_staticMeshes)
	{
		obj->draw(renderer);
	}

	for (const auto& obj : m_dynamicMeshes)
	{
		obj->draw(renderer);
	}

	camera.endZBIASDraw();
}

void SpaceRenderer::createRailModel(const Vector3& from, const Vector3& to)
{
	Vector3 dir(to - from);
	float length = dir.length();
	dir /= length;
	float angle = dir.z >= 0.0f ? acosf(dir.x) : -acosf(dir.x);

	uint numTiles = uint(ceilf(length / RAIL_SCALE));
	float tileLength = length / numTiles;

	auto& rs = RenderSystemDX9::instance();
	Geometry* railGeometry = rs.geometryManager().get(RAIL_PATH);
	Effect* pLightonlyEffect = rs.effectManager().get(SHADER_LIGHTONLY_PATH);

	Matrix tr; tr.id();
	tr.RotateY(angle + PI*0.5f); // rotate pi/2 because model is pre-rotated horizontally;
	tr.Scale(tileLength);
	Vector3 tileDelta(dir * (tileLength * RAIL_CONNECTION_OFFSET));
	Vector3 center(from + tileDelta * 0.5f);
	for (uint i = 1; i <= numTiles; ++i)
	{
		Model* newModel = new Model();
		newModel->setup(railGeometry, pLightonlyEffect);

		tr.SetTranslation(center);
		center += tileDelta;
		newModel->setTransform(tr);
		m_staticMeshes.emplace_back(newModel);
	}
}

void SpaceRenderer::createCityPoint(const Vector3& pos, EPostType type)
{
	assert((int)type <= CITY_COUNT);

	char buf[3];
	int cityIdx = int(type);
	_itoa_s(cityIdx, buf, 10);
	std::string dir = CITY_PATH + buf + "/";
	std::string cityPath = dir + "city.obj";

	auto& rs = RenderSystemDX9::instance();
	Model* newModel = new Model();
	Geometry* cityGeometry = rs.geometryManager().get(cityPath);
	Effect* pLightonlyEffect = rs.effectManager().get(SHADER_LIGHTONLY_PATH);
	newModel->setup(cityGeometry, pLightonlyEffect);
	
	std::string transformPath = dir + "transform.txt";
	std::ifstream fs(transformPath);
	
	float yOffset = 0.0f;
	float scale = 30.0f;
	if (!fs.fail())
	{
		fs >> yOffset;
		fs >> scale;
	}

	Matrix tr; tr.id();
	tr.Scale(scale);
	tr.SetTranslation(Vector3(pos.x, yOffset + pos.y, pos.z));
	newModel->setTransform(tr);
	m_dynamicMeshes.emplace_back(newModel);
}


void SpaceRenderer::setTrain(const Vector3& pos, const Vector3& direction, int trainId)
{
	auto& train = getTrain(trainId);
	auto& rs = RenderSystemDX9::instance();

	Matrix tr; tr.id();

	float angle = direction.z >= 0.0f ? acosf(direction.x) : -acosf(direction.x);
	tr.RotateY(angle + train.data.angle);
	tr.Scale(train.data.scale);
	tr.SetTranslation(Vector3(pos.x, train.data.yOffset + pos.y, pos.z));
	train.model->setTransform(tr);

	m_dynamicMeshes.emplace_back(train.model);
}

const SpaceRenderer::TrainGeometryData& SpaceRenderer::loadTrainGeometry()
{
	static int trainCounter = 0;

	auto it = m_trainModels.find(trainCounter);
	if (it == m_trainModels.end())
	{
		char buf[3];
		_itoa_s(trainCounter + 1, buf, 10);
		std::string dir = TRAIN_PATH + buf + "/";
		std::string trainPath = dir + "train.obj";

		std::string transformPath = dir + "transform.txt";
		std::ifstream fs(transformPath);

		TrainDesc desc = { RAIL_SCALE, 0.0f, 0.0f };
		if (!fs.fail())
		{
			fs >> desc.yOffset;
			fs >> desc.scale;
			fs >> desc.angle;
		}

		auto& rs = RenderSystemDX9::instance();
		auto trainGeometry = rs.geometryManager().get(trainPath);

		it = m_trainModels.try_emplace(trainCounter, trainGeometry, desc).first;
	}

	trainCounter = (trainCounter + 1) % TRAIN_COUNT;
	return it->second;
}

SpaceRenderer::TrainModel& SpaceRenderer::getTrain(int trainId)
{
	auto it = m_trains.find(trainId);
	if (it != m_trains.end())
	{
		return it->second;
	}

	auto& rs = RenderSystemDX9::instance();
	const auto& data = loadTrainGeometry();

	Model* newModel = new Model();
	Effect* pLightonlyEffect = rs.effectManager().get(SHADER_LIGHTONLY_PATH);
	newModel->setup(data.geometry, pLightonlyEffect);
	
	return m_trains.try_emplace(trainId, newModel, data.desc).first->second;
}

void SpaceRenderer::clearDynamics()
{
	m_dynamicMeshes.clear();
}

void SpaceRenderer::setupStaticScene(uint x, uint y)
{
	auto& rs = RenderSystemDX9::instance();
	auto device = rs.renderer().device();

	Effect* pEffect = rs.effectManager().get(SHADER_NORMALMAP_PATH);
	if (!pEffect)
	{
		LOG(MSG_ERROR, "Cannot load shader %s", SHADER_NORMALMAP_PATH);
		return;
	}

	// TERRAIN
	Model* newModel = new Model();
	Box* pTerrain = new Box();
	pTerrain->create(device);
	newModel->setup(pTerrain, pEffect);
	newModel->effectProperties().setTexture("diffuseTex", TERRAIN_DIFFUSE_TEXTURE_PATH.c_str());
	newModel->effectProperties().setTexture("normalTex", TERRAIN_NORMAL_TEXTURE_PATH.c_str());


	Matrix transform; transform.id();
	transform.SetTranslation(Vector3(float(x)*0.5f, -0.5f, float(y)*0.5f));
	transform.Scale(float(x + 20), 1.0f, float(y + 20));
	newModel->setTransform(transform);
	m_terrain = newModel;
}
