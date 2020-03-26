#pragma once

#include "component.h"
#include "script/interpreter.h"

class ScriptComponent : public Component
{
public:
	static Component* Create(const JSON::json& componentData);
	static Component* CreateDefault();

private:
	sol::environment m_Env;
	LuaTextResourceFile* m_ScriptFile;
	Vector<String> m_Functions;
	HashMap<String, String> m_Connections;

	friend class EntityFactory;

	ScriptComponent(LuaTextResourceFile* luaFile, HashMap<String, String> connections = {});
	ScriptComponent(ScriptComponent&) = delete;
	virtual ~ScriptComponent();

	bool isSuccessful(const sol::function_result& result);

public:
	static const ComponentID s_ID = (ComponentID)ComponentIDs::ScriptComponent;

	virtual bool setup() override;

	void connect(const String& function, const String& eventType);
	void call(const String& function);
	void onBegin();
	virtual void onUpdate(float deltaMilliSeconds);
	void onEnd();

	ComponentID getComponentID() const override { return s_ID; }
	virtual String getName() const override { return "ScriptComponent"; }
	virtual JSON::json getJSON() const override;

#ifdef ROOTEX_EDITOR
	virtual void draw();
#endif
};
