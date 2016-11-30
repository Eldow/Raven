#include "Weapon_GrenadeLauncher.h"
#include "../Raven_Bot.h"
#include "misc/Cgdi.h"
#include "../Raven_Game.h"
#include "../Raven_Map.h"
#include "../lua/Raven_Scriptor.h"
#include "fuzzy/FuzzyOperators.h"


//--------------------------- ctor --------------------------------------------
//-----------------------------------------------------------------------------
GrenadeLauncher::GrenadeLauncher(Raven_Bot*   owner) :

	Raven_Weapon(type_grenade_launcher,
		script->GetInt("GrenadeLauncher_DefaultRounds"),
		script->GetInt("GrenadeLauncher_MaxRoundsCarried"),
		script->GetDouble("GrenadeLauncher_FiringFreq"),
		script->GetDouble("GrenadeLauncher_IdealRange"),
		script->GetDouble("Grenade_MaxSpeed"),
		owner)
{
	//setup the vertex buffer
	const int NumWeaponVerts = 8;
	const Vector2D weapon[NumWeaponVerts] = { Vector2D(0, -3),
		Vector2D(6, -3),
		Vector2D(6, -1),
		Vector2D(15, -1),
		Vector2D(15, 1),
		Vector2D(6, 1),
		Vector2D(6, 3),
		Vector2D(0, 3)
	};
	for (int vtx = 0; vtx<NumWeaponVerts; ++vtx)
	{
		m_vecWeaponVB.push_back(weapon[vtx]);
	}

	//setup the fuzzy module
	InitializeFuzzyModule();

}


//------------------------------ ShootAt --------------------------------------
//-----------------------------------------------------------------------------
inline void GrenadeLauncher::ShootAt(Vector2D pos)
{
	if (NumRoundsRemaining() > 0 && isReadyForNextShot())
	{
		//fire off a grenade!
		m_pOwner->GetWorld()->AddGrenade(m_pOwner, pos);

		m_iNumRoundsLeft--;

		UpdateTimeWeaponIsNextAvailable();

		//add a trigger to the game so that the other bots can hear this shot
		//(provided they are within range)
		m_pOwner->GetWorld()->GetMap()->AddSoundTrigger(m_pOwner, script->GetDouble("GrenadeLauncher_SoundRange"));
	}
}

//---------------------------- Desirability -----------------------------------
//
//-----------------------------------------------------------------------------
double GrenadeLauncher::GetDesirability(double DistToTarget)
{
	if (m_iNumRoundsLeft == 0)
	{
		m_dLastDesirabilityScore = 0;
	}
	else
	{
		//fuzzify distance and amount of ammo
		m_FuzzyModule.Fuzzify("DistToTarget", DistToTarget);
		m_FuzzyModule.Fuzzify("AmmoStatus", (double)m_iNumRoundsLeft);

		m_dLastDesirabilityScore = m_FuzzyModule.DeFuzzify("Desirability", FuzzyModule::max_av);
	}

	return m_dLastDesirabilityScore;
}

//-------------------------  InitializeFuzzyModule ----------------------------
//
//  set up some fuzzy variables and rules
//-----------------------------------------------------------------------------
void GrenadeLauncher::InitializeFuzzyModule()
{
	FuzzyVariable& DistToTarget = m_FuzzyModule.CreateFLV("DistToTarget");

	FzSet& Target_Very_Close = DistToTarget.AddLeftShoulderSet("Target_Very_Close", 0, 25, 150);
	FzSet& Target_Close = DistToTarget.AddTriangularSet("Target_Close", 25, 150, 175);
	FzSet& Target_Medium = DistToTarget.AddTriangularSet("Target_Medium", 150, 175, 300);
	FzSet& Target_Far = DistToTarget.AddTriangularSet("Target_Far", 175, 300, 600);
	FzSet& Target_Very_Far = DistToTarget.AddRightShoulderSet("Target_Very_Far", 300, 600, 1000);

	FuzzyVariable& Desirability = m_FuzzyModule.CreateFLV("Desirability");
	FzSet& VeryDesirable = Desirability.AddRightShoulderSet("VeryDesirable", 65, 75, 100);
	FzSet& MoreDesirable = Desirability.AddTriangularSet("MoreDesirable", 25, 65, 75);
	FzSet& Desirable = Desirability.AddTriangularSet("Desirable", 25, 50, 65);
	FzSet& MehDesirable = Desirability.AddTriangularSet("MehDesirable", 12, 25, 50);
	FzSet& Undesirable = Desirability.AddLeftShoulderSet("Undesirable", 0, 12, 25);

	FuzzyVariable& AmmoStatus = m_FuzzyModule.CreateFLV("AmmoStatus");
	FzSet& Ammo_Loads = AmmoStatus.AddRightShoulderSet("Ammo_Loads", 20, 30, 100);
	FzSet& Ammo_Good_Enough = AmmoStatus.AddTriangularSet("Ammo_Good_Enough", 10, 20, 30);
	FzSet& Ammo_Okay = AmmoStatus.AddTriangularSet("Ammo_Okay", 5, 10, 20);
	FzSet& Ammo_Low = AmmoStatus.AddTriangularSet("Ammo_Low", 0, 5, 10);
	FzSet& Ammo_Panic = AmmoStatus.AddTriangularSet("Ammo_Panic", 0, 0, 5);


	m_FuzzyModule.AddRule(FzAND(Target_Very_Close, Ammo_Loads), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Close, Ammo_Good_Enough), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Close, Ammo_Okay), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Close, Ammo_Low), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Close, Ammo_Panic), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Loads), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Good_Enough), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Okay), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Low), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Close, Ammo_Panic), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Loads), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Good_Enough), VeryDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Okay), MoreDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Low), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Medium, Ammo_Panic), MehDesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Loads), Desirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Good_Enough), MehDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Okay), MehDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Low), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Far, Ammo_Panic), Undesirable);

	m_FuzzyModule.AddRule(FzAND(Target_Very_Far, Ammo_Loads), MehDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Far, Ammo_Good_Enough), MehDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Far, Ammo_Okay), MehDesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Far, Ammo_Low), Undesirable);
	m_FuzzyModule.AddRule(FzAND(Target_Very_Far, Ammo_Panic), Undesirable);
}


//-------------------------------- Render -------------------------------------
//-----------------------------------------------------------------------------
void GrenadeLauncher::Render()
{
	m_vecWeaponVBTrans = WorldTransform(m_vecWeaponVB,
		m_pOwner->Pos(),
		m_pOwner->Facing(),
		m_pOwner->Facing().Perp(),
		m_pOwner->Scale());

	gdi->RedPen();

	gdi->ClosedShape(m_vecWeaponVBTrans);
}