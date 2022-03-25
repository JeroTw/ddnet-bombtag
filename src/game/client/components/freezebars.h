#ifndef GAME_CLIENT_COMPONENTS_FREEZEBARS_H
#define GAME_CLIENT_COMPONENTS_FREEZEBARS_H
#include <game/client/component.h>

class CFreezeBars : public CComponent
{
	void RenderFreezeBar(const int ClientID);

public:
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnRender() override;
};

#endif
