#pragma once
#include "math/vector3.h"
#include <string>
#include <unordered_map>

struct Post;
struct Train;
struct Player;

namespace SpaceUI
{
void createPostUI(const Vector3& pos, const Post& post, const std::string* playerName);
void createTrainUI(const Vector3& pos, const Train& train, const std::string* playerName);
void createPlayerUI(const std::unordered_map<std::string, Player>& players);
} // namespace SpaceUI
