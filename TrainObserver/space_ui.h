#pragma once
#include "math/vector3.h"
#include <string>

struct Post;
struct Train;

namespace SpaceUI
{
void createPostUI(const Vector3& pos, const Post& post, const std::string& playerName);
void createTrainUI(const Vector3& pos, const Train& train, const std::string& playerName);
} // namespace SpaceUI
