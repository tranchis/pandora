
#include <HunterGatherer.hxx>
#include <GujaratWorld.hxx>
#include <Exceptions.hxx>
#include <Action.hxx>
#include <Sector.hxx>
#include <cmath>
#include <cassert>
#include <AgentController.hxx>

#include <GujaratState.hxx>
#include <GeneralState.hxx>
#include <Logger.hxx>

#include <GujaratConfig.hxx>


namespace Gujarat
{

HunterGatherer::HunterGatherer( const std::string & id ) 
	: GujaratAgent(id)/*, _surplusForReproductionThreshold(2), _surplusWanted(1)*/, _homeRange(50),
	_numSectors( -1 )
{
}

void HunterGatherer::registerAttributes()
{
	std::stringstream logName;
	logName << "Serializer_" << _world->getId();
	
	log_DEBUG(logName.str(), "registering attributes for type: " << getType());

	registerIntAttribute("MoveHome actions");
//	registerIntAttribute("Forage actions");
	registerIntAttribute("agent age");
//	registerIntAttribute("male alive");
//	registerIntAttribute("male age");
//	registerIntAttribute("female alive");
//	registerIntAttribute("female age");
	registerIntAttribute("children");
	registerIntAttribute("collected resources");
	registerIntAttribute("starving %");
//	registerIntAttribute("starving days x 100");
	log_DEBUG(logName.str(), "registering attributes for type: " << getType() << " finished");
}

HunterGatherer::~HunterGatherer()
{
	for ( unsigned k = 0; k < _sectors.size(); k++ )
	{
		delete _sectors[k];
	}
}


void HunterGatherer::updateKnowledge()
{
	// H/G can't preserve resources
	//std::cout << "collected from last time: " << _collectedResources << " surplus: " << _collectedResources - computeConsumedResources(1);
	//_collectedResources -= computeConsumedResources(1)
	//_collectedResources = std::max(0, _collectedResources - computeConsumedResources(1));
	//_collectedResources *= getSurplusSpoilageFactor();
	
	//std::cout << " spoiled: " << _collectedResources << " needed resources: " << computeConsumedResources(1) <<  std::endl;
	
	_collectedResources = 0;
	
	updateKnowledge( _position, getWorld()->getDynamicRaster(eResources), _sectors);
	
}


void HunterGatherer::updateKnowledge( const Engine::Point2D<int>& agentPos, const Engine::Raster& dataRaster, std::vector<Sector*>& sectors ) const
//void HunterGatherer::updateKnowledge( const Engine::Point2D<int>& agentPos, const enum Rasters dataRasterIdx, std::vector<Sector*>& sectors ) const
{
	std::stringstream logName;
	logName << getWorld()->getId() << "_" << getId() << ":updateKnowledge";

	if(sectors.size()==0)
	{
		sectors.resize(_numSectors);
		for ( unsigned k = 0; k < _numSectors; k++ )
		{
			sectors[k] = new Sector(((Engine::Agent*)this)->getWorldRef());
		}
	}
	else
	{
		for ( unsigned k = 0; k < _numSectors; k++ )
		{			
			sectors[k]->clearCells();
		}
	}
	
	register int C = ((GujaratConfig)((GujaratWorld*)_world)->getConfig())._cellsPerLowResCellSide;
	
	for ( int x=-_lowResHomeRange; x<=_lowResHomeRange; x++ )
	{
		for ( int y=-_lowResHomeRange; y<=_lowResHomeRange; y++ )
		{
			int indexSector = GujaratState::sectorsMask(x+_lowResHomeRange,y+_lowResHomeRange);

			if ( indexSector == - 1 )
			{
				continue;
			}			
			
			Engine::Point2D<int> LRpos;
			((GujaratWorld*)_world)->worldCell2LowResCell( agentPos, LRpos );
			Engine::Point2D<int> LRxycell(x+LRpos._x,y+LRpos._y);
			
			Engine::Point2D<int> corners[4]; // 4 corners that bound the world cells belonging to the low res cell
			corners[0]._x = LRxycell._x*C;
			corners[0]._y = LRxycell._y*C;
			corners[1]._x = LRxycell._x*C;
			corners[1]._y = LRxycell._y*C + C-1;
			corners[2]._x = LRxycell._x*C + C-1;
			corners[2]._y = LRxycell._y*C;
			corners[3]._x = LRxycell._x*C + C-1;
			corners[3]._y = LRxycell._y*C + C-1;
			
			// All four corners of the low res cell must be in the boundaries
			
			if(	!_world->getOverlapBoundaries().isInside(corners[0]) ||
				!_world->getOverlapBoundaries().isInside(corners[1]) ||
				!_world->getOverlapBoundaries().isInside(corners[2]) ||
				!_world->getOverlapBoundaries().isInside(corners[3]))	
			{
				continue;
			}
	
			sectors[indexSector]->addCell( LRxycell );
			
		}//for
	}//for

	for ( unsigned k = 0; k < _numSectors; k++ )
	{
		sectors[k]->updateFeatures(dataRaster);
	}
	
	
}
/*
void HunterGatherer::updateKnowledge()
{
	// H/G can't preserve resources
	//std::cout << "collected from last time: " << _collectedResources << " surplus: " << _collectedResources - computeConsumedResources(1);
	//_collectedResources -= computeConsumedResources(1);
	//_collectedResources *= getSurplusSpoilageFactor();

	//std::cout << " spoiled: " << _collectedResources << " needed resources: " << computeConsumedResources(1) <<  std::endl;
	_collectedResources = 0;
	std::stringstream logName;
	logName << "agents_" << _world->getId() << "_" << getId();

	log_DEBUG(logName.str(), "update knowledge");
	// sectors not initialized
	log_DEBUG(logName.str(), "new sectors");
	if(_sectors.size()==0)
	{
		_sectors.resize(_numSectors);
		for ( unsigned k = 0; k < _numSectors; k++ )
		{
			_sectors[k] = new Sector( getWorldRef());
		}
	}
	else
	{
		for ( unsigned k = 0; k < _numSectors; k++ )
		{
			//std::cout << this << "clearing sector: " << k << std::endl;
			_sectors[k]->clearCells();
			//std::cout << "DONE!" <<  std::endl;
		}
	}
	
	log_DEBUG(logName.str(), "create sectors with home range: " << _homeRange);
	for ( int x=-_homeRange; x<=_homeRange; x++ )
	{
		for ( int y=-_homeRange; y<=_homeRange; y++ )
		{
			int indexSector = GujaratState::sectorsMask(x+_homeRange,y+_homeRange);
			if ( indexSector == - 1 )
			{
				continue;
			}

			Engine::Point2D<int> p;
			p._x = _position._x + x;
			p._y = _position._y + y;
			// TODO overlapboundaries
			if ( !_world->getBoundaries().isInside(p) )
			{
				continue;
			}
			_sectors[indexSector]->addCell( p );
			//getWorld()->setValue( "sectors", p, 1 );
		}
	}
	log_DEBUG(logName.str(), "update features");

	for ( unsigned k = 0; k < _numSectors; k++ )
	{
		//std::cout << "Sector #" << (k+1) << " features:" << std::endl;
		_sectors[k]->updateFeatures();
		//_sectors[k]->showFeatures( std::cout );
	}
	log_DEBUG(logName.str(), "end update knowledge");
}
*/
void HunterGatherer::selectActions()
{
	std::list<MDPAction*> actions;
	GujaratState::controller().selectActions(*this, actions);
	std::list<MDPAction*>::iterator it=actions.begin();
	while(it!=actions.end())
	{
		_actions.push_back((Engine::Action*)(*it));
		it = actions.erase(it);
	}
}

GujaratAgent * HunterGatherer::createNewAgent()
{	
	GujaratWorld * world = (GujaratWorld*)_world;
	std::ostringstream oss;
	oss << "HunterGatherer_" << world->getId() << "-" << world->getNewKey();
	
	HunterGatherer * agent = new HunterGatherer(oss.str());

	agent->setSocialRange( _socialRange );
	agent->setHomeMobilityRange( _homeMobilityRange );
	agent->setHomeRange( _homeRange );
	agent->setLowResHomeRange( _lowResHomeRange );
	
	//agent->setSurplusForReproductionThreshold( _surplusForReproductionThreshold );
	//agent->setSurplusWanted( _surplusWanted );
	//agent->setSurplusSpoilageFactor( _surplusSpoilageFactor );
	//agent->setFoodNeedsForReproduction( _foodNeedsForReproduction );			

	agent->setWalkingSpeedHour( _walkingSpeedHour );
	agent->setForageTimeCost( _forageTimeCost );
	//agent->setAvailableForageTime( _availableForageTime );
	agent->setMassToCaloriesRate( _massToCaloriesRate );
	agent->setNumSectors( _sectors.size() );
	
	// initially the agent will be a couple
	agent->_populationAges.resize(2);

	return agent;
}

/*
bool HunterGatherer::needsResources()
{
	return _collectedResources < (_surplusForReproductionThreshold + _surplusWanted);
}
*/

bool HunterGatherer::cellValid( Engine::Point2D<int>& loc )
{
	
// Take care, this function works with the HR cells assumption : no more than 1 agent per cell
	
	bool isValid=true;
	if ( !_world->getOverlapBoundaries().isInside(loc) )
		return false;
	// Check that the home of another agent resides in loc
	std::vector<Agent * > agents = _world->getAgent(loc);
	if(agents.size()>0)
	{
		for(int i=0; i<agents.size() && isValid; i++)
		{
			Agent * agent = agents.at(i);
			isValid = !(agent->exists() && agent!=this);
		}
	}
	agents.clear();
	return isValid;
}

bool HunterGatherer::cellRelevant( Engine::Point2D<int>& loc )
{
	Soils soilType = (Soils) _world->getValue(eSoils, loc);
	int resourceType = _world->getValue(eResourceType, loc);
	return soilType == INTERDUNE && resourceType == WILD;
}

void HunterGatherer::serialize()
{
	serializeAttribute("agent age", _age);
/*
	if(_populationAges[0]!=-1)
	{
		serializeAttribute("male alive", 1);
		serializeAttribute("male age", _populationAges[0]);
	}
	else
	{
		serializeAttribute("male alive", 0);
		serializeAttribute("male age", std::numeric_limits<int>::max());
	}
	
	if(_populationAges[1]!=-1)
	{
		serializeAttribute("female alive", 1);
		serializeAttribute("female age", _populationAges[1]);
	}
	else
	{
		serializeAttribute("female alive", 0);
		serializeAttribute("female age", std::numeric_limits<int>::max());
	}
*/
	int numChildren = 0;
	for(unsigned i=2; i<_populationAges.size(); i++)
	{
		if(_populationAges[i]!=-1)
		{
			numChildren++;
		}
	}
	serializeAttribute("children", numChildren);
	serializeAttribute("collected resources", _collectedResources);
	serializeAttribute("starving %", getPercentageOfStarvingDays());
//	serializeAttribute("starving days x 100", _starved*100.0f);
	serializeAttribute("MoveHome actions", _moveHomeActionsExecuted);
//	serializeAttribute("Forage actions", _forageActionsExecuted);
}

} // namespace Gujarat

