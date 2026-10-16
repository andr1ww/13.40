#pragma once
#include "pch.h"
#include "Core.h"

namespace SlateApplication
{
	void PrepassWindowAndChildren(TSharedRef<class SWindow> WindowToPrepass);

	void Init();
}
