#pragma once

class ITickable
{
public:
	virtual void tick(float deltaTime) = 0;
};

class IInputListener
{
public:
	virtual void onMouseMove(int x, int y, int delta_x, int delta_y, bool bLeftButton) {};
	virtual void onMouseWheel(int nMouseWheelDelta) {};
	virtual void processInput(float deltaTime, unsigned char keys[256]) {};
};