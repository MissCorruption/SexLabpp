#include "Collision.h"

#include "Registry/Util/Premutation.h"
#include "Registry/Util/RayCast/ObjectBound.h"

namespace Registry::Collision
{

	Position::Snapshot::Snapshot(Position& a_position) :
		position(a_position),
		bHead([&]() {
			const auto nihead = a_position.nodes.head.get();
			if (!nihead)
				return ObjectBound{};
			auto ret = ObjectBound::MakeBoundingBox(nihead);
			return ret ? *ret : ObjectBound{};
		}()) {}

	void Position::Snapshot::GetHeadHeadInteractions(const Snapshot& a_partner)
	{
		if (!bHead.IsValid())
			return;
		assert(position.nodes.head);
		const auto& headworld = position.nodes.head->world;
		const auto mouthstart = GetHeadForwardPoint(bHead.boundMax.y);
		const auto pmouthstart = a_partner.GetHeadForwardPoint(a_partner.bHead.boundMax.y);
		if (!pmouthstart || !mouthstart)
			return;
		auto& partnernodes = a_partner.position.nodes;
		assert(partnernodes.head);
		const auto vMyHead = *mouthstart - headworld.translate;
		const auto vPartnerHead = *pmouthstart - partnernodes.head->world.translate;
		auto angle = Node::GetVectorAngle(vMyHead, vPartnerHead);
		if (std::abs(angle - 180) < Settings::fAngleMouth) {
			if (bHead.IsPointInside(*pmouthstart) || a_partner.bHead.IsPointInside(*mouthstart)) {
				float distance = pmouthstart->GetDistance(*mouthstart);
				interactions.emplace_back(a_partner.position.actor, Interaction::Action::Kissing, distance);
			}
		}
	}

	void Position::Snapshot::GetHeadVaginaInteractions(const Snapshot& a_partner)
	{
		if (!bHead.IsValid())
			return;
		if (a_partner.position.sex.none(Sex::Female, Sex::Futa))
			return;
		assert(position.nodes.head);
		auto& headworld = position.nodes.head->world;
		const auto mouthstart = GetHeadForwardPoint(bHead.boundMax.y);
		const auto vMyHead = *mouthstart - headworld.translate;
		auto& partnernodes = a_partner.position.nodes;
		auto vVaginal = partnernodes.GetVaginalVector();
		if (!vVaginal || !partnernodes.clitoris || !mouthstart)
			return;
		if (!bHead.IsPointInside(partnernodes.clitoris->world.translate))
			return;
		auto angle = Node::GetVectorAngle(*vVaginal, vMyHead);
		if ((angle - 180) > Settings::fAngleMouth * 2)
			return;
		float distance = partnernodes.clitoris->world.translate.GetDistance(*mouthstart);
		interactions.emplace_back(a_partner.position.actor, Interaction::Action::Oral, distance);
	}

	void Position::Snapshot::GetHeadPenisInteractions(const Snapshot& a_partner)
	{
		if (!bHead.IsValid())
			return;
		if (a_partner.position.sex.none(Sex::Male, Sex::Futa))
			return;
		const auto pMouth = GetHeadForwardPoint(bHead.boundMax.y);
		if (!pMouth)
			return;
		assert(position.nodes.head);
		auto& headworld = position.nodes.head->world;
		const auto vHead = headworld.rotate.GetVectorY();
		Node::Segment sHead{ headworld.translate, *pMouth };
		auto& partnernodes = a_partner.position.nodes;
		for (auto&& p : partnernodes.schlongs) {
			assert(p.base);
			const auto& base = p.base->world;
			const auto pTip = p.GetTipReferencePoint();
			const auto vSchlong = p.GetTipReferenceVector();
			const auto vBaseToHead = headworld.translate - base.translate;
			//const auto vRot = headworld.rotate * vSchlong;
			const auto aBaseToMouth = Node::GetVectorAngle(vHead, vSchlong);
			const auto aBaseToHead = Node::GetVectorAngle(vHead, vBaseToHead);
			const auto dCenter = headworld.translate.GetDistance(pTip);
			const auto dMouth = [&]() {
				Node::Segment sSchlong{ base.translate, pTip };
				const auto seg = Node::ClosestSegmentBetweenSegments(sHead, sSchlong);
				return seg.first.GetDistance(seg.second);
			}();
			const auto in_front_of_head = std::abs(aBaseToMouth - 180) < Settings::fAngleMouth;
			const auto at_side_of_head = std::abs(aBaseToMouth - 90) < 45.0f;
			const auto close_to_mouth = dMouth < bHead.boundMax.x;
			const auto penetrating_skull = dCenter < ((at_side_of_head ? bHead.boundMax.x : bHead.boundMax.y) * Settings::fHeadPenetrationRatio);

			if (penetrating_skull) {
				if (in_front_of_head && close_to_mouth) {
					interactions.emplace_back(a_partner.position.actor, Interaction::Action::Oral, dCenter);
					assert(partnernodes.pelvis);
					const auto tip_throat = pTip.GetDistance(headworld.translate) < bHead.boundMax.y * 0.1;
					const auto pelvis_head = partnernodes.pelvis->world.translate.GetDistance(headworld.translate) <= bHead.boundMax.y;
					if (tip_throat || pelvis_head) {
						interactions.emplace_back(a_partner.position.actor, Interaction::Action::Deepthroat, dCenter);
					}
				} else {
					interactions.emplace_back(a_partner.position.actor, Interaction::Action::Skullfuck, dCenter);
				}
			} else if (at_side_of_head && close_to_mouth) {
				interactions.emplace_back(a_partner.position.actor, Interaction::Action::LickingShaft, dCenter);
			} else if (aBaseToHead > 120.0f && pTip.GetDistance(headworld.translate) < bHead.boundMax.y * (1 / Settings::fHeadPenetrationRatio)) {
				interactions.emplace_back(a_partner.position.actor, Interaction::Action::Facial, dCenter);
			}
		}
	}

	void Position::Snapshot::GetHeadAnimObjInteractions(const Snapshot& a_other)
	{
		bool out;
		a_other.position.actor->GetGraphVariableBool("bAnimObjectLoaded", out);
		if (!out) {
			return;
		}
		const auto point = GetHeadForwardPoint(bHead.boundMax.y);
		if (!point)
			return;
		const auto getimpl = [&](auto pos) {
			if (!pos)
				return;
			const auto d = pos->world.translate.GetDistance(*point);
			if (d > Settings::fAnimObjDist)
				return;
			interactions.emplace_back(a_other.position.actor, Interaction::Action::AnimObjFace, d);
		};
		const auto& n = a_other.position.nodes;
		getimpl(n.animobj_a);
		getimpl(n.animobj_b);
		getimpl(n.animobj_r);
		getimpl(n.animobj_l);
	}

	void Position::Snapshot::GetCrotchPenisInteractions(const Snapshot& a_other)
	{
		if (a_other.position.sex.none(Sex::Male, Sex::Futa))
			return;
		const auto vVaginal = position.nodes.GetVaginalVector();
		const auto vAnal = position.nodes.GetAnalVector();
		const auto pVaginal = position.nodes.GetVaginalStart();
		const auto pAnal = position.nodes.GetAnalStart();
		const auto vCrotch = position.nodes.GetCrotchVector();
		for (auto&& p : a_other.position.nodes.schlongs) {
			const auto vSchlong = p.GetTipReferenceVector();
			assert(p.base && position.nodes.pelvis);
			const auto vPelvisToBase = p.base->world.translate - position.nodes.pelvis->world.translate;
			const auto dSchlongToPelvisLine = vPelvisToBase.Dot(vSchlong) / vPelvisToBase.SqrLength();
			if (dSchlongToPelvisLine > Settings::fDistanceCrotch)
				continue;
			const auto angleCrotch = Node::GetVectorAngle(vCrotch, vSchlong);
			if (vVaginal && vAnal && position.nodes.clitoris) {	// female
				assert(pVaginal && pAnal);
				const float dVag = pVaginal->GetDistance(p.base->world.translate), dAnal = pAnal->GetDistance(p.base->world.translate);
				const float anglePen = Node::GetVectorAngle(dVag <= dAnal ? *vVaginal : *vAnal, vSchlong);
				if (std::abs(anglePen - 180) < Settings::fAnglePenetration) {
					const auto act = dVag <= dAnal ? Interaction::Action::Vaginal : Interaction::Action::Anal;
					interactions.emplace_back(a_other.position.actor, act, dSchlongToPelvisLine);
				} else if (std::abs(angleCrotch - 180) < Settings::fAngleGrinding) {
					const auto distance = position.nodes.clitoris->world.translate.GetDistance(p.base->world.translate);
					interactions.emplace_back(a_other.position.actor, Interaction::Action::Grinding, distance);
				}
			} else {	 // male/no 3ba
				if (std::abs(angleCrotch - 90) < Settings::fAnglePenetration) {
					interactions.emplace_back(a_other.position.actor, Interaction::Action::Anal, dSchlongToPelvisLine);
				} else if (std::abs(angleCrotch - 180) < Settings::fAngleGrinding) {
					float distance;
					if (const auto& c = position.nodes.clitoris) {
						distance = c->world.translate.GetDistance(p.base->world.translate);
					} else {
						const auto& v = position.nodes.schlongs;
						if (v.empty()) {
							const auto base = position.nodes.ApproximateBase();
							distance = base.GetDistance(p.base->world.translate);
						} else {
							distance = std::numeric_limits<float>::max();
							for (auto&& i : v) {
								distance = std::min(distance, i.base->world.translate.GetDistance(p.base->world.translate));
							}
						}
					}
					interactions.emplace_back(a_other.position.actor, Interaction::Action::Grinding, distance);
				}
			}
		}
	}

	void Position::Snapshot::GetVaginaVaginaInteractions(const Snapshot& a_other)
	{
		if (position.sex.none(Sex::Female, Sex::Futa) || a_other.position.sex.none(Sex::Female, Sex::Futa))
			return;
		const auto &c1 = position.nodes.clitoris, &c2 = a_other.position.nodes.clitoris;
		if (!c1 || !c2)
			return;
		const auto distance = c1->world.translate.GetDistance(c2->world.translate);
		if (distance > Settings::fDistanceCrotch)
			return;
		auto vVaginal = position.nodes.GetVaginalVector();
		auto vVaginalPartner = a_other.position.nodes.GetVaginalVector();
		if (!vVaginal || !vVaginalPartner)
			return;
		const auto angle = Node::GetVectorAngle(*vVaginal, *vVaginalPartner);
		if (std::abs(angle - 180) > Settings::fAngleGrinding)
			return;
		interactions.emplace_back(a_other.position.actor, Interaction::Action::Grinding, distance);
	}

	void Position::Snapshot::GetGenitalLimbInteractions(const Snapshot& a_other)
	{
		const auto& othernodes = a_other.position.nodes;
		const auto impl = [&](const decltype(othernodes.hand_left)& activePoint, auto action) {
			const auto make = [&](RE::NiPoint3 refPoint) {
				const auto d = activePoint->world.translate.GetDistance(refPoint);
				if (d > Settings::fDistanceHand)
					return false;
				interactions.emplace_back(a_other.position.actor, action, d);
				return true;
			};
			if (!activePoint)
				return false;
			for (auto&& p : position.nodes.schlongs) {
				const auto& refpoint = p.mid ? p.mid->world.translate : position.nodes.ApproximateMid();
				if (make(refpoint))
					return true;
			}
			if (const auto& c = position.nodes.clitoris) {
				if (make(c->world.translate))
					return true;
			}
			return false;
		};
		for (auto&& node : { othernodes.hand_left, othernodes.hand_right }) {
			if (impl(node, Interaction::Action::HandJob))
				break;
		}
		for (auto&& node : { othernodes.foot_left, othernodes.foot_right }) {
			if (impl(node, Interaction::Action::FootJob))
				break;
		}
	}

	std::optional<RE::NiPoint3> Position::Snapshot::GetHeadForwardPoint(float distance) const
	{
		const auto& nihead = position.nodes.head;
		if (!nihead)
			return std::nullopt;
		const auto vforward = nihead->world.rotate.GetVectorY();
		return (vforward * distance) + nihead->world.translate;
	}

	Handler::Process::Process(const std::vector<RE::Actor*>& a_positions, const Scene* a_scene) :
		positions([&]() {
			std::vector<Collision::Position> v{};
			v.reserve(a_positions.size());
			for (size_t i = 0; i < a_positions.size(); i++) {
				auto& it = a_positions[i];
				auto sex = a_scene->GetNthPosition(i)->sex.get();
				v.emplace_back(it, sex);
			}
			return v;
		}()),
		active(true), _t(&Handler::Process::Update, this) {}

	Handler::Process::~Process()
	{
		active = false;
		_t.join();
	}

	void Handler::Process::Update()
	{
		const auto main = RE::Main::GetSingleton();
		const auto ui = RE::UI::GetSingleton();
		while (active) {
			do {
				std::this_thread::sleep_for(INTERVAL);
			} while (main->freezeTime || ui->numPausesGame > 0);
			const auto getVelocity = [](std::vector<Position::Interaction>& interactions, const Position& pos) {
				for (auto&& i : interactions) {
					auto where = pos.interactions.find(i);
					if (where == pos.interactions.end()) {
						continue;
					}
					const float delta_dist = i.distance - where->distance;
					i.velocity = (where->velocity + (delta_dist / INTERVAL.count())) / 2;
				}
			};
			std::vector<std::shared_ptr<Position::Snapshot>> snapshots{};
			snapshots.reserve(positions.size());
			for (auto&& it : positions) {
				auto shared = std::make_shared<Position::Snapshot>(it);
				auto& obj = snapshots.emplace_back(shared);
				obj->GetGenitalLimbInteractions(*obj);
				obj->GetHeadAnimObjInteractions(*obj);
			}
			if (positions.size() >= 2) {
				Combinatorics::for_each_permutation(snapshots.begin(), snapshots.begin() + 2, snapshots.end(),
					[&](auto start, [[maybe_unused]] auto end) {
						assert(std::distance(start, end) == 2);
						auto& fst = **start;
						auto& snd = **(start + 1);
						fst.GetHeadHeadInteractions(snd);
						fst.GetHeadVaginaInteractions(snd);
						fst.GetHeadPenisInteractions(snd);
						fst.GetHeadAnimObjInteractions(snd);
						fst.GetCrotchPenisInteractions(snd);
						fst.GetVaginaVaginaInteractions(snd);
						fst.GetGenitalLimbInteractions(snd);
						return false;
					});
			}
			assert(positions.size() == snapshots.size());
			for (size_t i = 0; i < positions.size(); i++) {
				for (auto&& act : snapshots[i]->interactions) {
					auto where = positions[i].interactions.find(act);
					if (where == positions[i].interactions.end()) {
						continue;
					}
					const float delta_dist = act.distance - where->distance;
					act.velocity = (where->velocity + (delta_dist / INTERVAL.count())) / 2;
				}
				positions[i].interactions = { snapshots[i]->interactions.begin(), snapshots[i]->interactions.end() };
			}
		}
	}

	void Handler::Register(RE::FormID a_id, std::vector<RE::Actor*> a_positions, const Scene* a_scene) noexcept
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

	void Handler::Unregister(RE::FormID a_id) noexcept
	{
		const auto where = std::ranges::find(processes, a_id, [](auto& it) { return it.first; });
		if (where == processes.end()) {
			logger::error("No object registered using ID {:X}", a_id);
			return;
		}
		processes.erase(where);
	}

	bool Handler::IsRegistered(RE::FormID a_id) const noexcept
	{
		return std::ranges::contains(processes, a_id, [](auto& it) { return it.first; });
	}

	const Handler::Process* Handler::GetProcess(RE::FormID a_id) const
	{
		const auto where = std::ranges::find(processes, a_id, [](auto& it) { return it.first; });
		return where == processes.end() ? nullptr : where->second.get();
	}

}	 // namespace Registry::Collision
