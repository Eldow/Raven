#include "Raven_TargetingSystem.h"
#include "Raven_Bot.h"
#include "Raven_SensoryMemory.h"
#include <algorithm>



//-------------------------------- ctor ---------------------------------------
//-----------------------------------------------------------------------------
Raven_TargetingSystem::Raven_TargetingSystem(Raven_Bot* owner):m_pOwner(owner),
                                                               m_pCurrentTarget(0)
{}



//----------------------------- Update ----------------------------------------

//-----------------------------------------------------------------------------
void Raven_TargetingSystem::Update()
{
  double ClosestDistSoFar = MaxDouble;
  m_pCurrentTarget       = 0;
  bool IsLeaderNear = false;
  bool IsAllyNear = false;
  //grab a list of all the opponents the owner can sense
  std::list<Raven_Bot*> SensedBots, SensedAllies;
  Raven_Bot* lastAlly = nullptr;
  SensedBots = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedOpponents();
  SensedAllies = m_pOwner->GetSensoryMem()->GetListOfRecentlySensedAllies();
  std::list<Raven_Bot*>::const_iterator curAlly = SensedBots.begin();
  std::list<Raven_Bot*>::const_iterator curBot = SensedBots.begin();
  if (!m_pOwner->IsLeader() && m_pOwner->GetTeam() != 0) {
	  //Leader nearby
	  for (curAlly; curAlly != SensedBots.end(); ++curAlly)
	  {
		  //Check if the ally target is near the owner
		  bool found = (std::find(SensedBots.begin(), SensedBots.end(), (*curAlly)->GetTargetSys()->GetTarget()) != SensedBots.end());
		  if ((*curAlly) == m_pOwner->GetLeader() && found) {
			  IsLeaderNear = true;
			  m_pCurrentTarget = (*curAlly)->GetTargetSys()->GetTarget();
		  }
		  if ((*curAlly)->GetTargetSys()->GetTarget() && found) {
			  lastAlly = *curAlly;
		  }
	  }
	  //No Leader nearby
	  if (!IsLeaderNear && SensedAllies.size() > 0 && lastAlly != nullptr) {
		  m_pCurrentTarget = lastAlly->GetTargetSys()->GetTarget();
		  IsAllyNear = true;
	  }
  }
  if ((!IsAllyNear && !IsLeaderNear) || m_pOwner->IsLeader())
  {

	  for (curBot; curBot != SensedBots.end(); ++curBot)
	  {
		  //make sure the bot is alive and that it is not the owner
		  if ((*curBot)->isAlive() && (*curBot != m_pOwner))
		  {
			  double dist = Vec2DDistanceSq((*curBot)->Pos(), m_pOwner->Pos());

			  if (dist < ClosestDistSoFar)
			  {
				  ClosestDistSoFar = dist;
				  m_pCurrentTarget = *curBot;
			  }
		  }
	  }
  }
}




bool Raven_TargetingSystem::isTargetWithinFOV()const
{
  return m_pOwner->GetSensoryMem()->isOpponentWithinFOV(m_pCurrentTarget);
}

bool Raven_TargetingSystem::isTargetShootable()const
{
  return m_pOwner->GetSensoryMem()->isOpponentShootable(m_pCurrentTarget);
}

Vector2D Raven_TargetingSystem::GetLastRecordedPosition()const
{
  return m_pOwner->GetSensoryMem()->GetLastRecordedPositionOfOpponent(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenVisible()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenVisible(m_pCurrentTarget);
}

double Raven_TargetingSystem::GetTimeTargetHasBeenOutOfView()const
{
  return m_pOwner->GetSensoryMem()->GetTimeOpponentHasBeenOutOfView(m_pCurrentTarget);
}
