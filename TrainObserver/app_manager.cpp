#include "stdafx.h"
#include "app_manager.h"
#include "scene_manager.h"
#include "window_manager.h"
#include "connection_manager.h"
#include "render_dx9.h"
#include "log.h"
#include "json_query_builder.h"
#include "space.h"
#include "ui_manager.h"
#include "SelectGameDlg.h"
#include "PlayerDlg.h"


AppManager::AppManager()
	: m_sceneManager(new SceneManager())
	, m_windowManager(new WindowManager())
	, m_renderSystem(new RenderSystemDX9())
	, m_connectionManager(new ConnectionManager())
	, m_connected(false)
	, m_gameController(this, m_connectionManager.get())
{
}


AppManager::~AppManager()
{
}

bool AppManager::initialize(HINSTANCE hInstance, int nCmdShow, uint width, uint height)
{
	if (FAILED(m_windowManager->create(hInstance, nCmdShow, width, height)))
	{
		LOG(MSG_ERROR, "Failed to initialize window manager");
		return false;
	}

	initLog(new WindowLog(m_windowManager->hwnd()));

	if (FAILED(m_renderSystem->init(m_windowManager->hwnd())))
	{
		LOG(MSG_ERROR, "Failed to initialize render system");
		return false;
	}


	if (FAILED(m_connectionManager->init()))
	{
		LOG(MSG_ERROR, "Failed to initialize connection manager");
		return false;
	}

	if (!m_sceneManager->init(m_renderSystem->renderer()))
	{
		LOG(MSG_ERROR, "Failed to initialize scene manager");
		return false;
	}

	m_windowManager->addInputListener(&m_renderSystem->renderer());
	m_windowManager->addInputListener(m_sceneManager.get());

	m_windowManager->addTickListener(this);
	m_windowManager->addTickListener(&m_renderSystem->renderer());

	auto& renderer = m_renderSystem->renderer();

	renderer.addRenderItem(m_sceneManager.get());
	renderer.addPostRenderItem(&m_renderSystem->uiManager());

	m_gameController.initialize();

	return true;
}

bool AppManager::connect(const char* servername, uint16_t portNumber)
{
	if (m_connected)
	{
		disconnect();
	}

	if (!m_connectionManager->connect(servername, portNumber))
	{
		LOG(MSG_ERROR, "Connection failed");
		m_connectionManager->reset();
		return false;
	}

	if (m_connectionManager->sendMessage(Action::OBSERVER, true))
	{
		std::string msg;
		Result res = m_connectionManager->receiveMessage(msg);
		if (res == Result::OKEY)
		{
			LOG(MSG_NORMAL, "Logged in to server as Observer.");
			m_connected = true;

			JSONQueryReader data(msg);
			std::map<uint32_t, std::string> games;
			std::map<uint32_t, unsigned int> lengths;
			for (const auto& game : data.asArray())
			{
				std::string name = game.get<std::string>("name");
				uint32_t idx = game.get<unsigned int>("idx");
				games[idx] = name;
				lengths[idx] = game.get<unsigned int>("length");
			}

			SelectGameDlg dlg(games);
			if (IDOK == dlg.DoModal())
			{
				uint32_t idGame = dlg.getGameID();
				JSONQueryWriter writer;
				writer.add("idx", idGame);
				m_gameController.maxTurn(lengths[idGame]);
				return m_connectionManager->sendMessage(Action::GAME, false, &writer.str());
			}
		}
	}

	return false;
}

uint AppManager::mainLoop()
{
	return m_windowManager->mainLoop();
}

void AppManager::disconnect()
{
	if (m_connected)
	{
		if (m_connectionManager->sendMessage(Action::LOGOUT))
		{
			LOG(MSG_NORMAL, "Logged out from server.");
			m_connected = false;
		}
	}
}

void AppManager::finalize()
{
	m_gameController.finalize();
	m_connectionManager->reset();
	m_renderSystem->fini();
	m_windowManager->destroy();
}

bool AppManager::loadStaticSpace()
{
	return m_sceneManager->initStaticScene(*m_connectionManager);
}

void AppManager::tick(float deltaTime)
{
	ITickable* pController = &m_gameController;
	pController->tick(deltaTime);
}

AppManager::GameController::GameController(AppManager *pManager, class ConnectionManager *pConnection)
	: m_pAppManager(pManager)
	, m_pConnection(pConnection)
	, m_dlg(new PlayerDlg(this))
{

}

AppManager::GameController::~GameController()
{
	
}

void AppManager::GameController::initialize()
{
	m_dlg->create(m_pAppManager->m_windowManager->hwnd());
}

void AppManager::GameController::finalize()
{
	m_dlg->destroy();
}

void AppManager::GameController::turn(float turnNumber)
{
	m_currentTurn = turnNumber;
}

void AppManager::GameController::maxTurn(int val)
{
	m_nMaxTurn = val;
	m_dlg->maxTurn(val);
}

void AppManager::GameController::tick(float deltaTime)
{
	m_dlg->tick(deltaTime);
	m_pAppManager->m_sceneManager->updateDynamicScene(*m_pConnection, m_currentTurn);
}
