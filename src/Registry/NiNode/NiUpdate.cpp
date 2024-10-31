#include "NiUpdate.h"

#include "NiMath.h"

namespace Registry::NiNode
{
	NiUpdate::Process::Process(const std::vector<RE::Actor*>& a_positions, const Scene* a_scene) :
		positions([&]() {
			std::vector<NiNode::NiPosition> v{};
			v.reserve(a_positions.size());
			for (size_t i = 0; i < a_positions.size(); i++) {
				auto& it = a_positions[i];
				auto sex = a_scene->GetNthPosition(i)->sex.get();
				v.emplace_back(it, sex);
			}
			return v;
		}()) {}

	bool NiUpdate::Process::VisitPositions(std::function<bool(const NiPosition&)> a_visitor) const
	{
		std::scoped_lock lk{ _m };
		for (auto&& pos : positions) {
			if (a_visitor(pos))
				return true;
		}
		return false;
	}

	void NiUpdate::Process::UpdateInteractions(float a_delta)
	{
		std::unique_lock lk{ _m, std::defer_lock };
		if (!lk.try_lock()) {
			return;
		}
		std::vector<NiPosition::Snapshot> snapshots{};
		snapshots.reserve(positions.size());
		for (auto&& it : positions) {
			snapshots.emplace_back(it);
		}
		for (auto&& fst : snapshots) {
			// Genital interactions for each combination
			if (fst.position.sex.none(Sex::Female)) {
				for (auto&& schlong : fst.position.nodes.schlongs) {
					for (auto&& snd : snapshots) {
						if (snd.GetHeadPenisInteractions(fst, schlong))
							break;
						if (snd.GetHandPenisInteractions(fst, schlong))
							break;
						if (fst == snd)
							continue;
						if (snd.GetCrotchPenisInteractions(fst, schlong)) {
							break;
						}
					}
				}
			}
			// Misc types for each combination
			for (auto&& snd : snapshots) {
				if (fst != snd) {
					snd.GetHeadHeadInteractions(fst);
					//	  fst.GetVaginaVaginaInteractions(snd);
				}
				//	 fst->GetHeadVaginaInteractions(*snd);
				//	 fst->GetGenitalLimbInteractions(*snd);	// <- Split this into Vaginal/Limb & Penis/Limb?
				//	 fst->GetHeadAnimObjInteractions(*snd);
			}
		}
		assert(positions.size() == snapshots.size());
		for (size_t i = 0; i < positions.size(); i++) {
			auto& pos = positions[i];
			for (auto&& act : snapshots[i].interactions) {
				auto where = pos.interactions.find(act);
				if (where == pos.interactions.end()) {
					continue;
				}
				const float delta_dist = (act.referencePoint - where->referencePoint).Length();
				if (a_delta != 0.0f) {
					act.velocity = (where->velocity + (delta_dist / a_delta)) / 2;
				} else {
					act.velocity = where->velocity;
				}
			}
			positions[i].interactions = { snapshots[i].interactions.begin(), snapshots[i].interactions.end() };
		}
	}

	void NiUpdate::Install()
	{
		// UpdateThirdPerson
		REL::Relocation<std::uintptr_t> addr{ RELOCATION_ID(39446, 40522), 0x94 };
		stl::write_thunk_call<NiUpdate>(addr.address());
		logger::info("Registered Functions");
	}

	void NiUpdate::thunk(RE::NiAVObject* a_obj, RE::NiUpdateData* updateData)
	{
		func(a_obj, updateData);
		static const auto gameDaysPassed = RE::Calendar::GetSingleton()->gameDaysPassed;
		if (!gameDaysPassed) {
			return;
		}
		std::scoped_lock lk{ _m };
		const auto ms_passed = gameDaysPassed->value * 24 * 60'000;
		static float ms_passed_last = ms_passed;
		const auto delta = ms_passed - ms_passed_last;
		ms_passed_last = ms_passed;
		for (auto&& [_, process] : processes) {
			process->UpdateInteractions(delta);
		}
	}


	void NiUpdate::Register(RE::FormID a_id, std::vector<RE::Actor*> a_positions, const Scene* a_scene) noexcept
	{
		try {
			const auto where = std::ranges::find(processes, a_id, [](auto& it) { return it.first; });
			if (where != processes.end()) {
				processes.erase(where);
			}
			auto process = std::make_unique<Process>(a_positions, a_scene);
			processes.emplace_back(a_id, std::move(process));
		} catch (const std::exception& e) {
			logger::error("Cannot register sound processing unit, Error: {}", e.what());
		}
	}

	void NiUpdate::Unregister(RE::FormID a_id) noexcept
	{
		const auto where = std::ranges::find(processes, a_id, [](auto& it) { return it.first; });
		if (where == processes.end()) {
			logger::error("No object registered using ID {:X}", a_id);
			return;
		}
		processes.erase(where);
	}

	bool NiUpdate::IsRegistered(RE::FormID a_id) noexcept
	{
		return std::ranges::contains(processes, a_id, [](auto& it) { return it.first; });
	}

	const NiUpdate::Process* NiUpdate::GetProcess(RE::FormID a_id) noexcept
	{
		const auto where = std::ranges::find(processes, a_id, [](auto& it) { return it.first; });
		return where == processes.end() ? nullptr : where->second.get();
	}

}	 // namespace Registry::NiNode
