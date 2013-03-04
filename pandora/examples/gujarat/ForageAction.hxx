#ifndef __ForageAction_hxx__
#define __ForageAction_hxx__

#include <MDPAction.hxx>
#include <Sector.hxx>
#include <Point2D.hxx>

namespace Engine
{
	class Agent;
}

namespace Gujarat
{

class GujaratAgent;
class GujaratWorld;

class ForageAction : public MDPAction
{
	Sector* _forageArea;
	bool _ownsForageAreaPointer;
	int	_biomassCollected;
	int	_caloriesCollected;
	bool _useFullPopulation;

	void selectBestNearestCell( const Engine::Point2D<int>& n
								, const GujaratWorld *gw
								, Engine::Raster& resourceRaster
								, int explorableCells
								, int& bestScore
								, Engine::Point2D<int>& best ) const;
											
			
	void doWalk( const GujaratAgent& agent
				, const Engine::Point2D<int>& n0
				, double maxDist
				, Engine::Raster& resourceRaster
				, int& collected ) const;
	
	void doWalk( GujaratAgent& agent
				, const Engine::Point2D<int>& n0
				, double maxDist
				, Engine::Raster& resourceRaster
				, int& collected );
	

public:
	ForageAction( Sector* loc, bool ownsPointer = false );
	virtual ~ForageAction();

	// if fullPopulation = false, only half the individuals will contribute to the action (the rest is moving home)
	void execute( Engine::Agent& agent);
	virtual void executeMDP( const GujaratAgent& agent, const HunterGathererMDPState& s, HunterGathererMDPState& sp ) const;

	int	getTimeNeeded() const { return 1; }
	virtual MDPAction* copy() const;
	virtual std::string describe() const;
	
	int	getBiomassCollected() const { return _biomassCollected; }
	int	getCaloriesCollected() const { return _caloriesCollected; }
	
	void setFullPopulation( bool useFullPopulation );
	const Sector & getSector() const {return *_forageArea;}

	int LRcellOutcomeHeuristic( Engine::Point2D<int> kCell2D
								, const Gujarat::GujaratWorld * gw
								, int explorableCells
								, const Engine::Raster& resRaster) const
	{
		long   n                          = gw->getValueLR(LRCounterSoilINTERDUNE,kCell2D);
		int    res                        = gw->getValueLR(resRaster,kCell2D);
		float  meanRes                    = ((float)res) / ((float)n);
		int    expectedExploredCells      = std::min((int)n,explorableCells);
		float  expectedRetrievedResources = meanRes * expectedExploredCells;		
		int    score                      = expectedRetrievedResources;
	/*std::cout << "score heuristic:" << n 
					<< "," << res
					<< "," << meanRes
					<< "," << expectedExploredCells
					<< "," << expectedRetrievedResources
					<< "," << score << std::endl;*/
	
	return score;
	}
	
	
};

}

#endif // ForageAction.hxx
