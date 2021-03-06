#pragma once

#include "rootex/app/application.h"
#include "rootex/framework/systems/hierarchy_system.h"
#include "framework/systems/script_system.h"

class HierarchyGraph;

/// Application that runs when game is run without the editor
class GameApplication : public Application
{
	String getLevelNameFromCommandLine(const char* s);
	Variant onExitEvent(const Event* event);

public:
	GameApplication();
	GameApplication(GameApplication&) = delete;
	~GameApplication() = default;
	
	String getAppTitle() const override { return "Game"; }
};
