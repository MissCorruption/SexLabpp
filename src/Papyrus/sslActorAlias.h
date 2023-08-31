#pragma once

#include "Registry/Animation.h"

namespace Papyrus::sslActorAlias
{
	void LockActorImpl(VM* a_vm, StackID a_stackID, RE::BGSRefAlias* a_alias);
	void UnlockActorImpl(VM* a_vm, StackID a_stackID, RE::BGSRefAlias* a_alias);

	std::vector<RE::TESForm*> StripByData(VM* a_vm, StackID a_stackID, RE::BGSRefAlias* a_alias, 
		Registry::Position::StripData a_stripdata, std::vector<uint32_t> a_defaults, std::vector<uint32_t> a_overwrite);
	std::vector<RE::TESForm*> StripByDataEx(VM* a_vm, StackID a_stackID, RE::BGSRefAlias* a_alias,
		Registry::Position::StripData a_stripdata, std::vector<uint32_t> a_defaults, std::vector<uint32_t> a_overwrite, std::vector<RE::TESForm*> a_mergewith);

	inline bool Register(VM* a_vm)
	{
		REGISTERFUNC(LockActorImpl, "sslActorLibrary", false);
		REGISTERFUNC(UnlockActorImpl, "sslActorLibrary", false);

		REGISTERFUNC(StripByData, "sslActorLibrary", false);
		REGISTERFUNC(StripByDataEx, "sslActorLibrary", false);

		return true;
	}
}	 // namespace Papyrus::sslActorAlias
