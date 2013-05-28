
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
	for ( unsigned k = 0; k < _HRSectors.size(); k++ )
	{
		_HRSectors[k]->clearCells();
		delete _HRSectors[k];
	}
	for ( unsigned k = 0; k < _LRSectors.size(); k++ )
	{
		_LRSectors[k]->clearCells();
		delete _LRSectors[k];
	}	
	
	_HRSectors.clear();
	_LRSectors.clear();
}

void HunterGatherer::updateKnowledge()
{	
	_collectedResources = 0;	
	std::stringstream logName;
	logName << "agents_" << _world->getId() << "_" << getId();
	
	// updateKnowledge  _HRsectors ???
	// _HRsectors contain only Point2D referencing HR cells in HR raster of resources,
	// so there are no utility attribs to update.
	
	// updateKnowledge  _LRsectors
	//updateKnowledge( _position, getWorld()->getDynamicRaster(eLRResources), _LRSectors);
	
	//if (_HRSectors.size()==0 && _LRSectors.size()==0) return;
	
	updateKnowledge( _position, getWorld()->getDynamicRaster(eLRResources), _HRSectors ,_LRSectors);	
	
	//*?	
	/*
	std::cout << "MSG> BIOMASS_AMOUNT:";
	for(int se=0; se<_HRSectors.size();se++)
		std::cout << " " << se << ":" << _HRSectors[se]->getBiomassAmount();
	std::cout << std::endl;
	
	std::cout << "MSG> BIOMASS_AMOUNT:";
	for(int se=0; se<_LRSectors.size();se++)
		std::cout << " " << se << ":" << _LRSectors[se]->getBiomassAmount();
	std::cout << std::endl;
	*/
	
	
}

void	HunterGatherer::updateKnowledge( 	const Engine::Point2D<int>& agentPos, const Engine::Raster& dataRaster, std::vector< Sector* >& HRSectors, std::vector< Sector* >& LRSectors  ) const
{		
	
	GujaratWorld * gw = (GujaratWorld*)_world;
	
	if(HRSectors.size()==0)
	{
		HRSectors.resize(_numSectors);	
		for ( unsigned k = 0; k < _numSectors; k++ )
		{
			HRSectors[k] = new Sector(*gw);
		}
		
		for ( int x=-_homeRange; x<=_homeRange; x++ )
		{
			for ( int y=-_homeRange; y<=_homeRange; y++ )
			{
				
				int indexSector = GujaratState::sectorsMask(x+_homeRange,y+_homeRange,GujaratState::getHRSectorsMask());
				if ( indexSector == - 1 )
				{
					continue;
				}
			
				Engine::Point2D<int> p;
				p._x = agentPos._x + x;
				p._y = agentPos._y + y;
				// TODO overlapboundaries
				if ( !_world->getBoundaries().isInside(p) )
				{
					continue;
				}
				HRSectors[indexSector]->addCell( p );
				//getWorld()->setValue( "sectors", p, 1 );	
			}
		}
	}
	
	
	
	
//********** LR	
	if(LRSectors.size()==0)
	{
		LRSectors.resize(_numSectors);
		for ( unsigned k = 0; k < _numSectors; k++ )
		{			
			LRSectors[k] = new Sector(*gw);	
		}
	
		//std::cout << "**********************" << std::endl;
	
		//TODO Those low resolution calculi should be arranged by the GujaratWorld class
		register int C = ((GujaratConfig)((GujaratWorld*)_world)->getConfig())._lowResolution;
		Engine::Point2D<int> LRpos;
		((GujaratWorld*)_world)->worldCell2LowResCell( agentPos, LRpos );
		for ( int x=-_lowResHomeRange; x<=_lowResHomeRange; x++ )
		{
			for ( int y=-_lowResHomeRange; y<=_lowResHomeRange; y++ )
			{				
				//*?
				if (x==0 && y==0)
				{
					continue;
				}
				
				int indexSector = GujaratState::sectorsMask(x+_lowResHomeRange,y+_lowResHomeRange,GujaratState::getLRSectorsMask());
			
				if ( indexSector == - 1 )
				{
					continue;
				}			
			
				//std::cout << "CELL:"<< x << "," << y << " at " << indexSector << std::endl;
			
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
			
				LRSectors[indexSector]->addCell( LRxycell );
			
			}//for
		}//for
		
		for ( unsigned k = 0; k < _numSectors; k++ )
		{
			LRSectors[k]->addCell( LRpos );
		}	
	}
	for ( unsigned k = 0; k < _numSectors; k++ )
	{
		LRSectors[k]->updateFeaturesLR(dataRaster);
	}
}

void HunterGatherer::clearSectorKnowledge() 
	{ 
		for ( unsigned i = 0; i < _numSectors; i++ )
		{
			delete _HRSectors[i]; 
		}		
		_HRSectors.clear(); 
		
		for ( unsigned i = 0; i < _numSectors; i++ )
		{
			delete _LRSectors[i]; 
		}		
		_LRSectors.clear(); 
	}

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
	agent->setNumSectors( ((GujaratConfig)((GujaratWorld*)_world)->getConfig())._numSectors );
	
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
	if ( !_world->getOverlapBoundaries().isInside(loc) )
		return false;
	// Check that the home of another agent resides in loc
	std::vector<Agent * > agents = _world->getAgent(loc);
	if(agents.size()==0)
	{
		agents.clear();
		return true;
	}

	for(int i=0; i<agents.size(); i++)
	{
		Agent * agent = agents.at(i);
		if(agent->exists() && agent!=this)
		{
			agents.clear();
			return false;
		}
	}
	agents.clear();
	return true;
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

