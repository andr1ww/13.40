#pragma once
#include "pch.h"

namespace FortPlayerPawnAthena
{
	void EndGameReviveFromDBNO(AFortPlayerPawnAthena* _this);
	void ServerReviveFromDBNO_Implementation(AFortPlayerPawnAthena* _this, AController* EventInstigator);

	void Init();
}
