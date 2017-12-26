#include <assert.h>
#include "space.h"
#include "json_query_builder.h"
#include "connection_manager.h"
#include "log_interface.h"
#include "space_renderer.h"
#include "math\vector3.h"
#include "space_ui.h"


using Vector3 = Vector3;

Space::Space():
	m_staticLayerLoaded(false)
{
}


Space::~Space()
{
}

std::shared_ptr<JSONQueryReader> getLayer(const ConnectionManager& connect, SpaceLayer layerId)
{
	JSONQueryWriter writer;
	writer.add("layer", layerId); // STATIC layer

	if (!connect.sendMessage(Action::MAP, true, &writer.str()))
	{
		LOG(MSG_ERROR, "Failed to create space. Reason: send MAP message failed");
		return nullptr;
	}

	std::string msg;
	if (connect.receiveMessage(msg) != Result::OKEY)
	{
		LOG(MSG_ERROR, "Failed to create space. Reason: receive MAP message failed");
		return nullptr;
	}

	return std::make_shared<JSONQueryReader>(msg);
}

bool Space::initStaticLayer(const ConnectionManager& manager)
{
	if (m_staticLayerLoaded)
		return true;

	auto reader = getLayer(manager, SpaceLayer::STATIC);

	if (reader && reader->isValid())
	{
		m_idx = reader->get<uint>("idx");
		m_name = reader->get<std::string>("name");

		if (!loadLines(*reader))
		{
			LOG(MSG_ERROR, "Failed to create static layer on space. Reason: cannot load lines.");
			return false;
		}

		if (!loadPoints(*reader))
		{
			LOG(MSG_ERROR, "Failed to create static layer on space. Reason: cannot load points.");
			return false;
		}
	}
	else
	{
		LOG(MSG_ERROR, "Failed to create static layer on space. Reason: parcing MAP message failed");
		return false;
	}

	// read geometry coordinates of points
	reader = getLayer(manager, SpaceLayer::COORDINATES);

	if (reader && reader->isValid())
	{
		assert ( m_idx == reader->get<uint>("idx") );

		if (!loadCoordinates(*reader))
		{
			LOG(MSG_ERROR, "Failed to create static layer on space. Reason: cannot load coordinates.");
			return false;
		}
	}
	else
	{
		LOG(MSG_ERROR, "Failed to create static layer on space. Reason: parcing MAP message with coordinates failed");
		return false;
	}

	postCreateStaticLayer();

	m_staticLayerLoaded = true;
	LOG(MSG_NORMAL, "Static space layer created. Points: %d. Lines: %d", m_points.size(), m_lines.size());
	return true;
}

bool Space::loadDynamicLayer(const ConnectionManager& manager, int turn, DynamicLayer& layer) const
{
	JSONQueryWriter writer;
	writer.add("idx", turn);
	if (!manager.sendMessage(Action::TURN, false, &writer.str()))
	{
		return false;
	}

	layer.trains.clear();
	layer.posts.clear();

	auto reader = getLayer(manager, SpaceLayer::DYNAMIC);

	if (reader && reader->isValid())
	{
		if (!loadTrains(*reader, layer))
		{
			LOG(MSG_ERROR, "Failed to create dynamic layer on space. Reason: cannot load trains.");
			return false;
		}

		if (!loadPosts(*reader, layer))
		{
			LOG(MSG_ERROR, "Failed to create dynamic layer on space. Reason: cannot load posts.");
			return false;
		}

		if (!loadPlayers(*reader, layer))
		{
			LOG(MSG_ERROR, "Failed to create dynamic layer on space. Reason: cannot load players.");
			return false;
		}
	}
	else
	{
		LOG(MSG_ERROR, "Failed to create dynamic layer on  space. Reason: parcing MAP message failed");
		return false;
	}

	return true;
}

const SpacePoint* Space::findPoint(uint idx) const
{
	for (const auto& p : m_points)
	{
		if (p.second.post_id == idx)
		{
			return &p.second;
		}
	}

	return nullptr;
}


bool Space::updateDynamicLayer(const ConnectionManager& manager, float turn)
{
	int curTurn = (int)ceilf(turn);
	int prevTurn = (int)floorf(turn);

	if (m_dynamicLayer.turn == curTurn && m_prevDynamicLayer.turn == prevTurn)
	{
		return true;
	}

	bool success = true;
	if (m_dynamicLayer.turn == prevTurn)
	{
		m_prevDynamicLayer = m_dynamicLayer;
	}
	else
	{
		success &= loadDynamicLayer(manager, prevTurn, m_prevDynamicLayer);
	}

	success &= loadDynamicLayer(manager, curTurn, m_dynamicLayer);

	m_prevDynamicLayer.turn = prevTurn;
	m_dynamicLayer.turn = curTurn;

	return success;
}

Vector3 coordToVector3(const Coords& c)
{
	return Vector3(float(c.x), 0.0f, float(c.y));
}

void Space::addStaticSceneToRender(SpaceRenderer& renderer)
{
	renderer.setupStaticScene(m_size.x, m_size.y);

	for (const auto& p : m_lines)
	{
		const Line& line = p.second;

		renderer.createRailModel(coordToVector3(line.pt_1->pos), coordToVector3(line.pt_2->pos));
	}
}

void Space::getWorldTrainCoords(const Train& t, Vector3& position, Vector3& dir)
{
	const Line& line = m_lines.at(t.line_idx);
	float deltaPos = float(t.position) / float(line.length);

	Vector3 p1(float(line.pt_1->pos.x), 0.0f, float(line.pt_1->pos.y));
	Vector3 p2(float(line.pt_2->pos.x), 0.0f, float(line.pt_2->pos.y));

	dir = p2 - p1;
	position = p1 + dir * deltaPos;
	dir.Normalize();
	if (t.speed < 0)
	{
		dir *= -1.0f;
	}
}

void Space::addDynamicSceneToRender(SpaceRenderer& renderer, float interpolator)
{
	renderer.clearDynamics();

	for (const auto& train : m_dynamicLayer.trains)
	{
		const Train& t = train.second;
		Vector3 pos, dir;
		getWorldTrainCoords(t, pos, dir);

		auto it = m_prevDynamicLayer.trains.find(t.idx);
		if (it != m_prevDynamicLayer.trains.end())
		{
			Vector3 pos2, dir2;
			getWorldTrainCoords(it->second, pos2, dir2);

			if (!pos2.almostEqual(pos))
			{
				dir = pos - pos2;
				dir.Normalize();
			}

			pos = pos * interpolator + pos2 * (1.0f - interpolator);
		}

		auto itp = m_dynamicLayer.players.find(train.second.player_id);
		SpaceUI::createTrainUI(pos, train.second, itp != m_dynamicLayer.players.end() ? &itp->second.name : nullptr);
		renderer.setTrain(pos, dir, t.idx);

	}

	for (const auto& p : m_dynamicLayer.posts)
	{
		auto idx = p.second.idx;
		const SpacePoint* point = findPoint(idx);

		if (point)
		{
			Vector3 worldPos(coordToVector3(point->pos));
			auto it = m_dynamicLayer.players.find(p.second.player_id);
			SpaceUI::createPostUI(worldPos, p.second, it != m_dynamicLayer.players.end() ? &it->second.name : nullptr);
			renderer.createCityPoint(worldPos, p.second.type);
		}
	}

	SpaceUI::createPlayerUI(m_dynamicLayer.players);
}

bool Space::loadLines(const JSONQueryReader& reader)
{
	auto values = reader.getValue("line").asArray();
	if (values.size() > 0)
	{
		m_lines.reserve(values.size());
		for (const auto& value : values)
		{
			uint idx = value.get<uint>("idx");
			uint length = value.get<uint>("length");
			auto points = value.getValue("point").asArray();
			uint pid_1, pid_2;

			if (points.size() == 2)
			{
				pid_1 = points[0].get<uint>();
				pid_2 = points[1].get<uint>();
			}
			else
			{
				LOG(MSG_ERROR, "Incorrect format of line!\n");
				return false;
			}

			m_lines.insert(std::make_pair(idx, Line(idx, length, pid_1, pid_2)));
		}
		return true;
	}

	return false;
}

bool Space::loadPoints(const JSONQueryReader& reader)
{
	auto values = reader.getValue("point").asArray();
	if (values.size() > 0)
	{
		m_points.reserve(values.size());
		for (const auto& value : values)
		{
			uint idx = value.get<uint>("idx");
			uint post_id = value.get<uint>("post_id");
			m_points.insert(std::make_pair(idx, SpacePoint(idx, post_id)));
		}
		return true;
	}

	return false;
}

bool Space::loadPlayers(const JSONQueryReader& reader, DynamicLayer& layer) const
{
	layer.players.clear();
	auto values = reader.getValue("rating").asArray();
	if (values.size() > 0)
	{
		//layer.players.reserve(values.size());
		for (const auto& value : values)
		{
			std::string id = value.get<std::string>("idx");
			std::string name = value.get<std::string>("name");
			uint rating = value.get<uint>("rating");
			layer.players.emplace(std::make_pair(id, Player(id, name, rating)));
		}
		return true;
	}

	return false;
}

bool Space::loadTrains(const JSONQueryReader& reader, DynamicLayer& layer) const
{
	auto values = reader.getValue("train").asArray();
	if (values.size() > 0)
	{
		layer.trains.reserve(values.size());
		for (const auto& value : values)
		{
			Train train;
			train.idx = value.get<uint>("idx");
			train.line_idx = value.get<uint>("line_idx");
			train.position = value.get<uint>("position");
			train.cooldown = value.get<uint>("cooldown");
			train.goods = value.get<uint>("goods");
			train.goods_capacity = value.get<uint>("goods_capacity");
			train.speed = value.get<int>("speed");
			train.level = value.get<int>("level");
			train.player_id = value.get<std::string>("player_id");
			layer.trains.insert(std::make_pair(train.idx, train));
		}
		return true;
	}

	return false;
}

bool Space::loadPosts(const JSONQueryReader& reader, DynamicLayer& layer) const
{
	auto values = reader.getValue("post").asArray();
	if (values.size() > 0)
	{
		layer.posts.reserve(values.size());
		for (const auto& value : values)
		{
			Post post;
			post.idx = value.get<uint>("idx");
			post.armor = value.get<uint>("armor");
			post.armor_capacity = value.get<uint>("armor_capacity");
			post.level = value.get<uint>("level");
			post.population = value.get<uint>("population");
			post.population_capacity = value.get<uint>("population_capacity");
			post.product = value.get<uint>("product");
			post.product_capacity = value.get<uint>("product_capacity");
			post.type = static_cast<EPostType>(value.get<uint>("type"));
			post.name = value.get<std::string>("name");
			post.player_id = value.get<std::string>("player_id");
			layer.posts.insert(std::make_pair(post.idx, post));
		}
		return true;
	}

	return false;
}

bool Space::loadCoordinates(const JSONQueryReader& reader)
{
	auto values = reader.getValue("coordinate").asArray();
	if (values.size() > 0)
	{
		assert( values.size() == m_points.size() );
		for (const auto& value : values)
		{
			Coords pos;
			pos.x = value.get<uint>("x");
			pos.y = value.get<uint>("y");
			uint idx = value.get<uint>("idx");
			auto res = m_points.find(idx);
			if (res == m_points.end())
			{
				LOG(MSG_ERROR, "Inconsistent coordinates. Cannot find post with id = %d!", idx);
				return false;
			}

			res->second.pos = pos;
		}

		auto size = reader.getValue("size").asArray();
		assert(size.size() == 2);
		m_size.x = size[0].get<uint>();
		m_size.y = size[1].get<uint>();

		return true;
	}

	return false;
}

void Space::postCreateStaticLayer()
{
	for (auto& p : m_lines)
	{
		Line& line = p.second;
		auto it1 = m_points.find(line.pid_1);
		auto it2 = m_points.find(line.pid_2);

		if (it1 == m_points.end() || it2 == m_points.end())
		{
			LOG(MSG_ERROR, "Incorrect lines vs points connection!");
			return;
		}

		line.pt_1 = &it1->second;
		line.pt_1->lines.emplace_back(&line);
		
		line.pt_2 = &it2->second;
		line.pt_2->lines.emplace_back(&line);
	}
}
