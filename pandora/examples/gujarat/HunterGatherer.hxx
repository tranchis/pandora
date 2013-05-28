
#ifndef __HunterGatherer_hxx__
#define __HunterGatherer_hxx__

#include <GujaratAgent.hxx>

namespace Gujarat
{
class Sector;
class ForageAction;

class HunterGatherer : public GujaratAgent
{	
	//int 					_surplusForReproductionThreshold;
	//int 					_surplusWanted;
	int 					_homeRange; // MpiBasicAttribute
	int 					_numSectors; // MpiBasicAttribute
	int						_lowResHomeRange; // MpiBasicAttribute
	
	//void updateKnowledge();

	// this method checks whether P1 and P2 are in the same side of the line that goes from B to C
	bool sameSide( Engine::Point2D<int> P1, Engine::Point2D<int> P2, Engine::Point2D<int> A, Engine::Point2D<int> B );
	bool insideTriangle( Engine::Point2D<int> p,  Engine::Point2D<int> b, Engine::Point2D<int> c ); 
	
	void selectActions();
	
	GujaratAgent * createNewAgent();

	std::vector<Sector *> _HRSectors;
	std::vector<Sector *> _LRSectors;

public:
	HunterGatherer( const std::string & id );
	virtual ~HunterGatherer();

	void setPosition(Engine::Point2D<int> p) { 	GujaratAgent::setPosition(p); _HRSectors.clear(); _LRSectors.clear(); }
	
	void setHomeRange( int v ) { _homeRange = v; }
	int  getHomeRange() const { return _homeRange; }
	void setNumSectors( int v ) { _numSectors = v; }
	int  getNumSectors() const { return _numSectors; }
	void setLowResHomeRange(int v) { _lowResHomeRange = v; }
	int getLowResHomeRange() const { return _lowResHomeRange; }
	
	const std::vector<Sector *> & getHRSectors() const { return _HRSectors; }
	const std::vector<Sector *> & getLRSectors() const { return _LRSectors; }
	
	Sector * getHRSectors(int i) { return _HRSectors[i]; }
	
	//void setSurplusForReproductionThreshold( int v ) { _surplusForReproductionThreshold = v; }
	//void setSurplusWanted( int v ) { _surplusWanted = v; }
	bool cellValid( Engine::Point2D<int>& loc );
	bool cellRelevant( Engine::Point2D<int>& loc );
	
//	bool needsResources(int timeSteps);
//	bool needsResources();

	void updateResources( int v ) { _collectedResources += v; }
	
	void updateKnowledge();
	void updateKnowledge( 	const Engine::Point2D<int>& agentPos, const Engine::Raster& dataRaster, std::vector< Sector* >& HRSectors, std::vector< Sector* >& LRSectors  )const;
	void clearSectorKnowledge();
	
	void serialize();
	void registerAttributes();

	
	////////////////////////////////////////////////
	// This code has been automatically generated //
	/////// Please do not modify it ////////////////
	////////////////////////////////////////////////
#ifdef PANDORAMPI
	HunterGatherer( void * );
	void * fillPackage();
	void sendVectorAttributes(int);
	void receiveVectorAttributes(int);
#endif // PANDORAMPI
	////////////////////////////////////////////////
	//////// End of generated code /////////////////
	////////////////////////////////////////////////

};

} // namespace Gujarat

#endif // __HunterGatherer_hxx__

