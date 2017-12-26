#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "defs.hpp"
#include "mutex.h"

struct Line;
class ConnectionManager;
class JSONQueryReader;

struct Coords
{
	uint x = 0;
	uint y = 0;
};

struct SpacePoint
{
	uint	idx;
	uint	post_id;
	Coords	pos;

	std::vector<const Line*> lines;

	SpacePoint(uint idx_, uint post_id_) :
		idx(idx_),
		post_id(post_id_)
	{}

};

struct Line
{
	uint idx;
	uint length;
	uint pid_1;
	uint pid_2;

	SpacePoint*		pt_1;
	SpacePoint*		pt_2;

	Line(uint idx_, uint length_, uint pid1, uint pid2):
		idx(idx_),
		length(length_),
		pid_1(pid1),
		pid_2(pid2)
	{}
};

struct Train
{
	uint idx;
	uint line_idx;
	uint level;
	uint goods;
	uint goods_capacity;
	std::string player_id;
	uint position;
	uint cooldown;
	int speed;
};

enum class EPostType
{
	CITY = 1,
	MARKET = 2,
	MILITARY_STORAGE = 3,
};


struct Post
{
	uint idx;
	uint armor;
	uint armor_capacity;
	uint level;
	uint population;
	uint population_capacity;
	uint product;
	uint product_capacity;
	EPostType type;
	std::string name;
	std::string player_id;
};

struct Player
{
	std::string id;
	std::string name;
	uint rating = 0;

	Player(const std::string& _id, const std::string& _name, uint _rating):
		id(_id),
		name(_name),
		rating(_rating)
	{}

	Player()
	{}
};


class Space
{
	struct DynamicLayer
	{
		std::unordered_map<uint, Train> trains;
		std::unordered_map<uint, Post>	posts;
		std::map<std::string, Player> players;
		int								turn = -1;
	};
public:
	Space();
	~Space();

	bool initStaticLayer(const ConnectionManager& manager);
	bool updateDynamicLayer(const ConnectionManager& manager, float turn);

	void addStaticSceneToRender(class SpaceRenderer& renderer);
	void addDynamicSceneToRender(SpaceRenderer& renderer, float interpolator);

private:
	bool loadLines(const JSONQueryReader& reader);
	bool loadPoints(const JSONQueryReader& reader);
	bool loadPlayers(const JSONQueryReader& reader, DynamicLayer& layer) const;
	bool loadTrains(const JSONQueryReader& reader, DynamicLayer& layer) const;
	bool loadPosts(const JSONQueryReader& reader, DynamicLayer& layer) const;
	bool loadCoordinates(const JSONQueryReader& reader);
	void postCreateStaticLayer();
	void getWorldTrainCoords(const Train& train, struct Vector3& pos, Vector3& dir);
	bool loadDynamicLayer(const ConnectionManager& manager, int turn, DynamicLayer& layer) const;
	const SpacePoint* findPoint(uint idx) const;

private:
	uint			m_idx;
	std::string		m_name;
	Coords			m_size;
	bool			m_staticLayerLoaded;

	std::unordered_map<uint, SpacePoint>	m_points;
	std::unordered_map<uint, Line>	m_lines;

	
	DynamicLayer	m_dynamicLayer;
	DynamicLayer	m_prevDynamicLayer;
	SimpleMutex		m_dynamicMutex;
};

