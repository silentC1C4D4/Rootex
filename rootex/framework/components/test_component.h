#pragma once

#include "component.h"

class TestComponent : public Component
{
	static Component* Create(const JSON::json& componentData);
	static Component* CreateDefault();

	friend class EntityFactory;

public:
	virtual String getName() const override { return "TestComponent"; }
	static const ComponentID s_ID = (ComponentID)ComponentIDs::TestComponent;

	ComponentID getComponentID() const override { return s_ID; }
	virtual JSON::json getJSON() const override;
};
