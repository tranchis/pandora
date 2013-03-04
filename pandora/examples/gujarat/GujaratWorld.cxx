
#include <GujaratWorld.hxx>

#include <Raster.hxx>
#include <Point2D.hxx>
#include <HunterGatherer.hxx>
//#include <AgroPastoralist.hxx>
#include <Exceptions.hxx>
#include <GujaratConfig.hxx>
#include <OriginalDemographics.hxx>

#include <GeneralState.hxx>
#include <Logger.hxx>
#include <RasterLoader.hxx>
#include <Statistics.hxx>
#include <iomanip>
#include <algorithm>

#include <limits>

namespace Gujarat
{

GujaratWorld::GujaratWorld( Engine::Simulation & simulation, const GujaratConfig & config ) 
	: World(simulation, config._homeRange+1, true, config._resultsFile), _agentKey(0), _climate(config,*this), _config(config)					
{
	// overlap is maxHomeRange + 1 to allow splits to be in adjacent worlds
	// TODO code a function proces config for resources 
	_yearlyBiomass.resize(4);
	_dailyRainSeasonBiomassIncrease.resize(4);
	_dailyDrySeasonBiomassDecrease.resize(4);
	_remainingBiomass.resize(4);
	_remainingBiomass[3] = _remainingBiomass[2] = _remainingBiomass[1] = 0.0f;
}

GujaratWorld::~GujaratWorld()
{
}    

/*
Engine::Point2D<int> GujaratWorld::findNearestWater( const Engine::Point2D<int> & point )
{
	Engine::Point2D<int> nearestWater(-1,-1);
	float bestDist = std::numeric_limits<float>::max();
	Engine::Point2D<int> index;
	bool found = false;
	int nextDist = 1;
	while(!found)
	{
		for(index._x=point._x-nextDist; index._x<point._x+nextDist; index._x++)
		{
			for(index._y=point._y-nextDist; index._y<point._y+nextDist; index._y++)
			{
				if(_overlapBoundaries.isInside(index) && getValue("soils", index)==WATER)
				{
					found = true;
					float dist = index.distance(point);
					if(dist<bestDist)
					{
						bestDist = dist;
						nearestWater = index;
					}
				}

			}
		}		
		nextDist++;
	}
	//std::cout << "final dist: " << nextDist << std::endl;
	return nearestWater;
}
*/

void GujaratWorld::createRasters()
{
	std::stringstream logName;
	logName << "simulation_" << _simulation.getId();
	log_DEBUG(logName.str(), getWallTime() << " creating static rasters");
	registerStaticRaster("soils", _config.isStorageRequired("soils"), eSoils);
	Engine::GeneralState::rasterLoader().fillGDALRaster(getStaticRaster(eSoils), _config._soilFile, this);	

	registerStaticRaster("dem", _config.isStorageRequired("dem"), eDem);
	Engine::GeneralState::rasterLoader().fillGDALRaster(getStaticRaster(eDem), _config._demFile, this);

	registerStaticRaster("distWater", _config.isStorageRequired("distWater"), eDistWater);
	Engine::GeneralState::rasterLoader().fillGDALRaster(getStaticRaster(eDistWater), _config._distWaterFile, this);

	if(_config._biomassDistribution.compare("linDecayFromWater")==0 || _config._biomassDistribution.compare("logDecayFromWater")==0)
	{
		registerStaticRaster("weightWater", _config.isStorageRequired("weightWater"), eWeightWater);
		Engine::GeneralState::rasterLoader().fillGDALRaster(getStaticRaster(eWeightWater), _config._weightWaterFile, this);
	}

	//registerDynamicRaster("weightWater", true);
	//getDynamicRaster("weightWater").setInitValues(0, std::numeric_limits<int>::max(), 0);
	/*
	registerStaticRaster("duneMap", eDuneMap, _config.isStorageRequired("duneMap"));
	Engine::GeneralState::rasterLoader().fillGDALRaster(getStaticRaster("duneMap"), _config._duneMapFile, this);
	*/

	log_DEBUG(logName.str(), getWallTime() << " creating dynamic rasters");

	/*
	registerDynamicRaster("moisture", _config.isStorageRequired("moisture"));
	getDynamicRaster("moisture").setInitValues(0, std::numeric_limits<int>::max(), 0);
	*/
	
	registerDynamicRaster("resources", _config.isStorageRequired("resources"), eResources);
	getDynamicRaster(eResources).setInitValues(0, std::numeric_limits<int>::max(), 0);
	
	// we need to keep track of resource fractions
	registerDynamicRaster("resourcesFraction", false, eResourcesFraction);
	getDynamicRaster(eResourcesFraction).setInitValues(0, 100, 0);

	registerDynamicRaster("forageActivity", _config.isStorageRequired("forageActivity"), eForageActivity); 
	getDynamicRaster(eForageActivity).setInitValues(0, std::numeric_limits<int>::max(), 0);
	/*
	registerDynamicRaster("homeActivity", eHomeActivity, _config.isStorageRequired("homeActivity"));
	getDynamicRaster("homeActivity").setInitValues(0, std::numeric_limits<int>::max(), 0);
	registerDynamicRaster("farmingActivity", eFarmingActivity, _config.isStorageRequired("farmingActivity"));
	getDynamicRaster("farmingActivity").setInitValues(0, std::numeric_limits<int>::max(), 0);
	*/

	/*
	registerDynamicRaster("resourceType", eResourceType, _config.isStorageRequired("resourceType")); // type of resources: wild, domesticated or fallow
	getDynamicRaster("resourceType").setInitValues(0, SEASONALFALLOW, WILD);
	registerDynamicRaster("consecutiveYears", eConsecutiveYears, _config.isStorageRequired("consecutiveYears")); // years passed using a given cell for a particular use
	getDynamicRaster("consecutiveYears").setInitValues(0, 3, 0);
	registerDynamicRaster("sectors", eSectors, _config.isStorageRequired("sectors"));
	getDynamicRaster("sectors").setInitValues(0, _config._numSectors, 0);
	*/

	log_DEBUG(logName.str(), getWallTime() << " generating settlement areas");
	_settlementAreas.generateAreas( *this, _config._lowResolution);
	log_DEBUG(logName.str(), getWallTime() << " create rasters done");

std::cout << "init LR" << std::endl;	
	
	// Low Ressolution Rasters
	int LowResRasterSideSize = getLowResMapsSideSize();
	Engine::Point2D<int> lowResSize2D( LowResRasterSideSize, LowResRasterSideSize);
	
	registerDynamicRaster("eLRResources", _config.isStorageRequired("eLRResources")
			, eLRResources, lowResSize2D);
	getDynamicRaster(eLRResources).setInitValues(0, std::numeric_limits<int>::max(), 0);
	fillLRRaster(eLRResources, 0);
	
	registerDynamicRaster("eLRResourcesFraction", _config.isStorageRequired("eLRResourcesFraction")
			, eLRResourcesFraction, lowResSize2D);
	getDynamicRaster(eLRResourcesFraction).setInitValues(0, std::numeric_limits<int>::max(), 0);
	fillLRRaster(eLRResourcesFraction, 0);
	
	registerDynamicRaster("eLRForageActivity", _config.isStorageRequired("eLRForageActivity")
			, eLRForageActivity, lowResSize2D);
	getDynamicRaster(eLRForageActivity).setInitValues(0, std::numeric_limits<int>::max(), 0);
	
	registerDynamicRaster("eLRHomeActivity", _config.isStorageRequired("eLRHomeActivity")
			, eLRHomeActivity, lowResSize2D);
	getDynamicRaster(eLRHomeActivity).setInitValues(0, std::numeric_limits<int>::max(), 0);
	
	registerDynamicRaster("eLRFarmingActivity", _config.isStorageRequired("eLRFarmingActivity")
			, eLRFarmingActivity, lowResSize2D);
	getDynamicRaster(eLRFarmingActivity).setInitValues(0, std::numeric_limits<int>::max(), 0);
	
	registerDynamicRaster("eLRPopulation", _config.isStorageRequired("eLRPopulation")
			, eLRPopulation, lowResSize2D);
	getDynamicRaster(eLRPopulation).setInitValues(0, std::numeric_limits<int>::max(), 0);
	fillLRRaster(eLRPopulation, 0);
	
	
	/*registerDynamicRaster("eLRMoisture", _config.isStorageRequired("eLRMoisture")
			, eLRMoisture, Engine::Point2D<int>(LowResRasterSideSize,LowResRasterSideSize));
	getDynamicRaster(eLRMoisture).setInitValues(0, std::numeric_limits<int>::max(), 0);
	*/
	// Low Ressolution Soil Counters
	
std::cout << "init LR dune" << std::endl;		

std::cout << "max "<< _config._cellsPerLowResCellSide*_config._cellsPerLowResCellSide << std::endl;		

	registerStaticRaster("LRCounterSoilDUNE", false, LRCounterSoilDUNE, lowResSize2D );
	getStaticRaster(LRCounterSoilDUNE).setDefaultInitValues(0, _config._cellsPerLowResCellSide*_config._cellsPerLowResCellSide, 0);	
	fillLowResCounterRaster(LRCounterSoilDUNE,eSoils,DUNE);
	
std::cout << "init LR interdune" << std::endl;		
	
	registerStaticRaster("LRCounterSoilINTERDUNE", false, LRCounterSoilINTERDUNE, lowResSize2D);
	getStaticRaster(LRCounterSoilINTERDUNE).setDefaultInitValues(0, _config._cellsPerLowResCellSide*_config._cellsPerLowResCellSide, 0);
	fillLowResCounterRaster(LRCounterSoilINTERDUNE,eSoils,INTERDUNE);
	
std::cout << "init LR water" << std::endl;		
	

	if(_config._biomassDistribution.compare("linDecayFromWater")==0 || _config._biomassDistribution.compare("logDecayFromWater")==0)
	{
	
		registerStaticRaster("LRCounterSoilWATER", false, LRCounterSoilWATER, lowResSize2D);
		getStaticRaster(LRCounterSoilWATER).setDefaultInitValues(0, _config._cellsPerLowResCellSide*_config._cellsPerLowResCellSide, 0);
		fillLowResCounterRaster(LRCounterSoilWATER,eSoils,WATER);

		std::cout << "init LR weightwater" << std::endl;		
	
		registerStaticRaster("eLRWeightWater", false, eLRWeightWater, lowResSize2D);
		getStaticRaster(eLRWeightWater).setDefaultInitValues(0, std::numeric_limits<int>::max(), 0);
		fillLowResMeanRaster(eLRWeightWater,eWeightWater);
	}
std::cout << "end init LR" << std::endl;	
}


void GujaratWorld::fillLRRaster(enum Rasters idLRRaster, int val)
{
	Engine::Point2D<int> index;
	
	for(index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++)		
	{	
		for(index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++)		
		{			
			Engine::Point2D<int> mapCell; 
			worldCell2LowResCell(index, mapCell);
			setInitValueLR(idLRRaster,mapCell,val);
		}
	}
}



void GujaratWorld::fillLowResCounterRaster(enum Rasters idRasterCounter, enum Rasters idRasterSource,int target)
{
	Engine::Point2D<int> index;
	
	for(index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++)		
	{	
		for(index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++)		
		{
			int val = getValue(idRasterSource,index);
			Engine::Point2D<int> mapCell; 
			worldCell2LowResCell(index, mapCell);
			int count = getValueLR(idRasterCounter,mapCell);
			if(val==target)
			{
				setInitValueLR(idRasterCounter,mapCell,count+1);
			}
			
		}
	}
}

void GujaratWorld::fillLowResMeanRaster(enum Rasters idRasterCounter, enum Rasters idRasterSource)
{
	Engine::Point2D<int> index;
	for(index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++)		
	{	
		for(index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++)		
		{
			int val = getValue(idRasterSource,index);
			Engine::Point2D<int> mapCell; 
			worldCell2LowResCell( index, mapCell);
			int count = getValueLR(idRasterCounter,mapCell);
			setInitValueLR(idRasterCounter,mapCell,count+val);
		}
	}
	
	for(	index._x=_boundaries._origin._x/_config._cellsPerLowResCellSide; 
			index._x<(_boundaries._origin._x+_boundaries._size._x)/_config._cellsPerLowResCellSide; 
			index._x++)		
	{	
		for( index._y=_boundaries._origin._y/_config._cellsPerLowResCellSide; 
			 index._y<(_boundaries._origin._y+_boundaries._size._y)/_config._cellsPerLowResCellSide; 
			 index._y++)		
		{	 
//			std::cout << "get value at " << index << std::endl;
			int count = getValueLR(idRasterCounter,index);
			setInitValueLR(idRasterCounter
						,index						,count/(_config._cellsPerLowResCellSide*_config._cellsPerLowResCellSide));
		}
	}
}


int GujaratWorld::getValueLR( const int & index, const Engine::Point2D<int> & position ) const
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	return _rasters.at(index)->getValue(localPosition);
}


int GujaratWorld::getValueLR( const Engine::Raster & r, const Engine::Point2D<int> & position ) const
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	return r.getValue(localPosition);
}


void GujaratWorld::setValueLR( const int & index, const Engine::Point2D<int> & position, int value )
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	((Engine::Raster*)_rasters.at(index))->setValue(localPosition, value);
}

void GujaratWorld::setValueLR( Engine::Raster & r, const Engine::Point2D<int> & position, int value )
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	r.setValue(localPosition, value);
}

void GujaratWorld::setInitValueLR( const int & index, const Engine::Point2D<int> & position, int value )
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	((Engine::Raster*)_rasters.at(index))->setInitValue(localPosition, value);
}

void GujaratWorld::setInitValueLR( Engine::Raster & r, const Engine::Point2D<int> & position, int value )
{
	Engine::Point2D<int> originLR;
	worldCell2LowResCell( _overlapBoundaries._origin, originLR );
	Engine::Point2D<int> localPosition(position - originLR);
	r.setInitValue(localPosition, value);
}


void GujaratWorld::createAgents()
{
	std::stringstream logName;
	logName << "simulation_" << _simulation.getId();
	log_DEBUG(logName.str(), getWallTime() << " creating agents");
	for(int i=0; i<_config._numHG; i++)
	{ 
		if((i%_simulation.getNumTasks())==_simulation.getId())
		{			
			log_DEBUG(logName.str(), getWallTime() << " new HG with index: " << i);
			std::ostringstream oss;
 			oss << "HunterGatherer_" << i;
			HunterGatherer * agent = new HunterGatherer(oss.str());
			addAgent(agent);
			//_config._hgInitializer->initialize(agent);
			
			agent->createInitialPopulation(_config._adulthoodAge);

			agent->setSocialRange( _config._socialRange );
			agent->setHomeMobilityRange( _config._homeRange );
			agent->setHomeRange( _config._homeRange );
			agent->setLowResHomeRange( _config._lowResHomeRange );
			//agent->setSurplusForReproductionThreshold( _config._surplusForReproductionThreshold );
			//agent->setSurplusWanted( _config._surplusWanted );
			//agent->setSurplusSpoilageFactor( _config._surplusSpoilage );
			
			//agent->setFoodNeedsForReproduction(_config._hgFoodNeedsForReproduction);			
			agent->setWalkingSpeedHour( _config._walkingSpeedHour / _config._cellResolution );
			agent->setForageTimeCost( _config._forageTimeCost );
			//agent->setAvailableForageTime( _config._availableForageTime );
			agent->setMassToCaloriesRate( _config._massToEnergyRate * _config._energyToCalRate );
			agent->setNumSectors( _config._numSectors );

			agent->initializePosition();
			log_DEBUG(logName.str(), getWallTime() << " new HG: " << agent);

			Engine::Point2D<int> LRpos;
			worldCell2LowResCell(agent->getPosition(),LRpos);
			setValueLR(eLRPopulation,LRpos,1+getValueLR(eLRPopulation,LRpos));

			
		}
	}

	/*
	for(int i=0; i<_config._numAP; i++)
	{ 
		if((i%_simulation.getNumTasks())==_simulation.getId())
		{
			std::ostringstream oss;
 			oss << "AgroPastoralist_" << i;
			AgroPastoralist * agent = new AgroPastoralist(oss.str());
			addAgent(agent); 
			_config._apInitializer->initialize(agent);
			agent->setSocialRange( _config._socialRange );
			//agent->setSurplusSpoilageFactor( _config._surplusSpoilage );
			agent->setHomeMobilityRange( _config._socialRange );
			agent->setMaxCropHomeDistance( _config._maxCropHomeDistance );
			agent->setMassToCaloriesRate( _config._massToEnergyRate * _config._energyToCalRate );

			agent->initializePosition();
			std::cout << _simulation.getId() << " new AgroPastoralist: " << agent << std::endl;
		}
	}
	*/
}

void GujaratWorld::updateRainfall()
{		
	_climate.step();
}

// TODO adapt to lowres
void GujaratWorld::updateSoilCondition()
{
	Engine::Point2D<int> index;
	if(_climate.getSeason()==HOTWET)
	{
		for(index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++)		
		{
			for(index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++)
			{
				setValue(eResources, index, getValue(eMoisture, index));
				if(getValue(eResourceType, index)==WILD)
				{
					continue;
				}
				
				if(getValue(eResourceType, index)==SEASONALFALLOW)
				{
					setValue(eResourceType, index, DOMESTICATED);
				}
				int consecutiveYears = getValue(eConsecutiveYears, index);
				consecutiveYears++;				
				if(consecutiveYears<3)
				{
					setValue(eConsecutiveYears, index, consecutiveYears);
				}
				else
				{
					setValue(eConsecutiveYears, index, 0);
					if(getValue(eResourceType, index)==FALLOW)
					{
						setValue(eResourceType, index, WILD);

					}
					else
					{
						setValue(eResourceType, index, FALLOW);
					}
				}
			}
		}
	}
	else if(_climate.getSeason()==HOTDRY)
	{
		for(index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++)		
		{
			for(index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++)
			{
				if(getValue(eResourceType, index)==DOMESTICATED)
				{
					setValue(eResourceType, index, SEASONALFALLOW);
				}
			}
		}
	}
}

void GujaratWorld::updateResources()
{
	Engine::Point2D<int> index;
	for( index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++ )		
	{
		for( index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++ )
		{
			// 3. Increment or Decrement cell biomass depending on yearly biomass
			//    figures and current timestep
			int currentValue = getValue(eResources, index);
			float currentFraction = (float)getValue(eResourcesFraction, index)/100.0f;			
			Soils cellSoil = (Soils)getValue(eSoils, index);
			if(cellSoil!=WATER)
			{
				Seasons season = _climate.getSeason();
				bool wetSeason = false;
				if(season==HOTWET)
				{
					wetSeason = true;
				}			
				float newValue = std::max(0.0f, currentValue+currentFraction+getBiomassVariation(wetSeason, cellSoil, index));
				currentValue = newValue;
				float fraction = 100.0f*(newValue  - currentValue);
				setValue(eResources, index, currentValue);
				setValue(eResourcesFraction, index, (int)fraction);
			}
		}
	}
}

void GujaratWorld::updateResourcesLowResMap()
{
	Seasons season = _climate.getSeason();
	bool wetSeason = false;	
	if(season==HOTWET)
	{
		wetSeason = true;
	}
	
	Engine::Point2D<int> index;
	worldCell2LowResCell( _boundaries._origin, index );
	Engine::Point2D<int> bound(_boundaries._origin._x+_boundaries._size._x, _boundaries._origin._y+_boundaries._size._y);
	Engine::Point2D<int> boundLR;
	worldCell2LowResCell( bound, boundLR );
	int firstY = index._y;
	while( index._x < boundLR._x  )                
	{
		while( index._y < boundLR._y )
		{
			int currentRes = getValueLR(eLRResources,index);
			int F          = getValueLR(eLRResourcesFraction,index);
			
			//AgentResourceKnowledge::instance()->getFractionAt(iterator); 
			float currentF = F / 100.0;
		
		// forbidden : r = rand()*samplesize
		// mandatory : r = sum[1..samplesize](rand())
		// it does not generates the same distribution
		
			/*float BMV_D = 0.0; // Dune Biomass Variation
			for(int i=0; i<getValueLR(LRCounterSoilDUNE,index); i++)
			{
				BMV_D = BMV_D + getBiomassVariation(wetSeason,DUNE,index,eLRWeightWater);
			}*/
			//BMV_D = std::max(0.0f,BMV_D);
			float BMV_D = getValueLR(LRCounterSoilDUNE,index) * getBiomassVariation(wetSeason,DUNE,index,eLRWeightWater);
		
			/*float BMV_I = 0.0; // InterDune Biomass Variation
			for(int i=0; i<getValueLR(LRCounterSoilINTERDUNE,index); i++)
			{
				BMV_I = BMV_I + getBiomassVariation(wetSeason,INTERDUNE,index,eLRWeightWater);
			}*/
			float BMV_I = getValueLR(LRCounterSoilINTERDUNE,index) * getBiomassVariation(wetSeason,INTERDUNE,index,eLRWeightWater);
			
			//BMV_I = std::max(0.0f,BMV_I);
		
			// BMV_W is a factor that stands in for the missing weightWater factor.
		
			float BMV_W = 0.0f; // InterDune Biomass Variation due to water
			/*
			 *		for(int i=0; i<getValue(rasterWaterLR,index); i++)
			*		{
			*			BMV_W = BMV_W + max(0,BiomVar(wetSeason,WATER,Point2D<int>(0,0)));
			}
			BMV_W += max(0,BMV_W);
			*/
		
			float newResources = std::max(0.0f,currentRes + currentF + BMV_D + BMV_I + BMV_W); 			
			
			currentRes = (int)newResources;
			currentF = 100.0f*(newResources - currentRes);
			setValueLR(eLRResources,index,currentRes);
			setValueLR(eLRResourcesFraction,index,(int)currentF);
			
			index._y++;
		}//while
		index._y = firstY;
		//std::cout << std::endl;
		index._x++;		
	}//while
	
}


void GujaratWorld::recomputeYearlyBiomass()
{
	// update all the map resources to the minimum of the year (in case it was diminished by agents)
	Engine::Point2D<int> index;
	for( index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++ )                
	{
		for( index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++ )
		{
			setValue(eResourcesFraction, index, 0);
			setValue(eResources, index, _remainingBiomass[getValue(eSoils, index)]);
		}
	}

	std::stringstream logName;
	logName << "World_" << _simulation.getId();
	// 1. Compute factor between actual rain and average rain		
	float raininessFactor = _climate.getRain() / _climate.getMeanAnnualRain();
	
	// each cell is 31.5m * 31.5m
	double areaOfCell = _config._cellResolution * _config._cellResolution;

	// 2. For each soil type compute yearly biomass	

	// data expressed in g/m2
	_yearlyBiomass[WATER] = 0.0f;
	_yearlyBiomass[DUNE] = areaOfCell*_config._duneBiomass * raininessFactor * _config._duneEfficiency;
	_yearlyBiomass[INTERDUNE] = areaOfCell*_config._interduneBiomass * _config._interduneEfficiency * raininessFactor;
	
	//std::cout << std::setprecision(2) << std::fixed << "area of cell: " << areaOfCell << " interdune biomass: " << _config._interduneBiomass << " efficiency: " << _config._interduneEfficiency << " raininess factor: " << raininessFactor << " yearly biomass: " << _yearlyBiomass[INTERDUNE] << std::endl;
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " rain: " << _climate.getRain() << " with mean: " << _climate.getMeanAnnualRain());
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " yearly biomass of dune: " << _yearlyBiomass[DUNE] << " and interdune: " << _yearlyBiomass[INTERDUNE]);

	// yearly biomass is the area of a triangle with max height at the end of wet season
	// A_1 + A_2 = biomass, being A_1 = daysPerSeason*h/2 and A_2 = 2*daysPerSeason*h/2
	// dPS*h/2 + 2*dPS*h/2 = biomass, so h = biomass/1.5*dPS
	// and A_2 = 2*A_1

	double heightInterDune = (_yearlyBiomass[INTERDUNE] + 120.0f*_config._interduneMinimum-60.0*_remainingBiomass[INTERDUNE]) / (180+240*_config._interduneMinimum);
	_dailyRainSeasonBiomassIncrease[INTERDUNE] = (heightInterDune-_remainingBiomass[INTERDUNE])/120.0f;
	_remainingBiomass[INTERDUNE] = heightInterDune *_config._interduneMinimum;
	_dailyDrySeasonBiomassDecrease[INTERDUNE] = heightInterDune*(1-_config._interduneMinimum)/240.0f;

	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " biomass height interdune: " << heightInterDune << " increment: " << _dailyRainSeasonBiomassIncrease[INTERDUNE] << " and decrease: " << _dailyDrySeasonBiomassDecrease[INTERDUNE]);
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " calories height interdune: " << _config._massToEnergyRate*_config._energyToCalRate*heightInterDune << " increment: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyRainSeasonBiomassIncrease[INTERDUNE] << " and decrease: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyDrySeasonBiomassDecrease[INTERDUNE]);

	double heightDune = (_yearlyBiomass[DUNE] + 120.0f*_config._duneMinimum-60.0*_remainingBiomass[DUNE]) / (180+240*_config._duneMinimum);
	_dailyRainSeasonBiomassIncrease[DUNE] = (heightDune-_remainingBiomass[DUNE])/120.0f;
	_remainingBiomass[DUNE] = heightDune *_config._duneMinimum;
	_dailyDrySeasonBiomassDecrease[DUNE] = heightDune*(1-_config._duneMinimum)/240.0f;

	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " biomass height dune: " << heightDune << " increment: " << _dailyRainSeasonBiomassIncrease[DUNE] << " and decrease: " << _dailyDrySeasonBiomassDecrease[DUNE]);
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " calories height dune: " << _config._massToEnergyRate*_config._energyToCalRate*heightDune << " increment: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyRainSeasonBiomassIncrease[DUNE] << " and decrease: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyDrySeasonBiomassDecrease[DUNE]);

	_dailyRainSeasonBiomassIncrease[WATER] = 0.0f;
	_dailyDrySeasonBiomassDecrease[WATER] = 0.0f;
	_remainingBiomass[WATER] = 0.0f;
}

void GujaratWorld::recomputeLowResYearlyBiomass()
{
	// update all the map resources to the minimum of the year (in case it was diminished by agents)
	Engine::Point2D<int> index;
	worldCell2LowResCell( _boundaries._origin, index );
	Engine::Point2D<int> bound(_boundaries._origin._x+_boundaries._size._x, _boundaries._origin._y+_boundaries._size._y);
	Engine::Point2D<int> boundLR;
	worldCell2LowResCell( bound, boundLR );
	int firstY = index._y;
	while( index._x<boundLR._x  )                
	{
		while( index._y<boundLR._y )
		{
			setValueLR(eLRResourcesFraction, index, 0);
			int remBio = _remainingBiomass[DUNE]*getValueLR(LRCounterSoilDUNE, index) +
						_remainingBiomass[INTERDUNE]*getValueLR(LRCounterSoilINTERDUNE, index) +
						_remainingBiomass[WATER]*getValueLR(LRCounterSoilWATER, index);
			setValueLR(eLRResources, index, remBio);
			index._y++;
		}
		index._y = firstY;
		index._x++;
	}
	
	
	// update all the map resources to the minimum of the year (in case it was diminished by agents)
	/*
	Engine::Point2D<int> index;
	for( index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++ )                
	{
		for( index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++ )
		{
			Engine::Point2D<int> indexLR;
			worldCell2LowResCell( index, indexLR );
			setValueLR(eLRResourcesFraction, indexLR, 0);
			//setValue(eResources, index, _remainingBiomass[getValue(eSoils, index)]);
		}
	}
	
	for( index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++ )                
	{
		for( index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++ )
		{
			Engine::Point2D<int> indexLR;
			worldCell2LowResCell( index, indexLR );
			setValueLR(eLRResources, indexLR, 0);
		}
	}
	
	for( index._x=_boundaries._origin._x; index._x<_boundaries._origin._x+_boundaries._size._x; index._x++ )                
	{
		for( index._y=_boundaries._origin._y; index._y<_boundaries._origin._y+_boundaries._size._y; index._y++ )
		{
			Engine::Point2D<int> indexLR;
			worldCell2LowResCell( index, indexLR );
			int val  = getValueLR(eLRResources, indexLR);
			int soil = getValue(eSoils, index);
			setValueLR(eLRResources, indexLR, val + _remainingBiomass[soil]);
		}
	}
	*/
	
	std::stringstream logName;
	logName << "World_" << _simulation.getId();
	// 1. Compute factor between actual rain and average rain		
	float raininessFactor = _climate.getRain() / _climate.getMeanAnnualRain();
	
	// each cell is 31.5m * 31.5m
	double areaOfCell = _config._cellResolution * _config._cellResolution;
	
	// 2. For each soil type compute yearly biomass	
	
	// data expressed in g/m2
	_yearlyBiomass[WATER] = 0.0f;
	_yearlyBiomass[DUNE] = areaOfCell*_config._duneBiomass * raininessFactor * _config._duneEfficiency;
	_yearlyBiomass[INTERDUNE] = areaOfCell*_config._interduneBiomass * _config._interduneEfficiency * raininessFactor;
	
	//std::cout << std::setprecision(2) << std::fixed << "area of cell: " << areaOfCell << " interdune biomass: " << _config._interduneBiomass << " efficiency: " << _config._interduneEfficiency << " raininess factor: " << raininessFactor << " yearly biomass: " << _yearlyBiomass[INTERDUNE] << std::endl;
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " rain: " << _climate.getRain() << " with mean: " << _climate.getMeanAnnualRain());
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " yearly biomass of dune: " << _yearlyBiomass[DUNE] << " and interdune: " << _yearlyBiomass[INTERDUNE]);
	
	// yearly biomass is the area of a triangle with max height at the end of wet season
	// A_1 + A_2 = biomass, being A_1 = daysPerSeason*h/2 and A_2 = 2*daysPerSeason*h/2
	// dPS*h/2 + 2*dPS*h/2 = biomass, so h = biomass/1.5*dPS
	// and A_2 = 2*A_1
	
	double heightInterDune = (_yearlyBiomass[INTERDUNE] + 120.0f*_config._interduneMinimum-60.0*_remainingBiomass[INTERDUNE]) / (180+240*_config._interduneMinimum);
	_dailyRainSeasonBiomassIncrease[INTERDUNE] = (heightInterDune-_remainingBiomass[INTERDUNE])/120.0f;
	_remainingBiomass[INTERDUNE] = heightInterDune *_config._interduneMinimum;
	_dailyDrySeasonBiomassDecrease[INTERDUNE] = heightInterDune*(1-_config._interduneMinimum)/240.0f;
	
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " biomass height interdune: " << heightInterDune << " increment: " << _dailyRainSeasonBiomassIncrease[INTERDUNE] << " and decrease: " << _dailyDrySeasonBiomassDecrease[INTERDUNE]);
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " calories height interdune: " << _config._massToEnergyRate*_config._energyToCalRate*heightInterDune << " increment: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyRainSeasonBiomassIncrease[INTERDUNE] << " and decrease: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyDrySeasonBiomassDecrease[INTERDUNE]);
	
	double heightDune = (_yearlyBiomass[DUNE] + 120.0f*_config._duneMinimum-60.0*_remainingBiomass[DUNE]) / (180+240*_config._duneMinimum);
	_dailyRainSeasonBiomassIncrease[DUNE] = (heightDune-_remainingBiomass[DUNE])/120.0f;
	_remainingBiomass[DUNE] = heightDune *_config._duneMinimum;
	_dailyDrySeasonBiomassDecrease[DUNE] = heightDune*(1-_config._duneMinimum)/240.0f;
	
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " biomass height dune: " << heightDune << " increment: " << _dailyRainSeasonBiomassIncrease[DUNE] << " and decrease: " << _dailyDrySeasonBiomassDecrease[DUNE]);
	log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " calories height dune: " << _config._massToEnergyRate*_config._energyToCalRate*heightDune << " increment: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyRainSeasonBiomassIncrease[DUNE] << " and decrease: " << _config._massToEnergyRate*_config._energyToCalRate*_dailyDrySeasonBiomassDecrease[DUNE]);
	
	_dailyRainSeasonBiomassIncrease[WATER] = 0.0f;
	_dailyDrySeasonBiomassDecrease[WATER] = 0.0f;
	_remainingBiomass[WATER] = 0.0f;
}


void GujaratWorld::stepEnvironment()
{
	// at the end of simulation
	_climate.advanceSeason();

	std::stringstream logName;
	logName << "World_" << _simulation.getId();

	// if this is the first step of a wet season, rainfall and biomass are calculated for the entire year
	if ( _climate.rainSeasonStarted() )
	{
		log_INFO(logName.str(), getWallTime() << " timestep: " << getCurrentTimeStep() << " year: " << getCurrentTimeStep()/360 << " num agents: " << _agents.size());
		updateRainfall();
		recomputeLowResYearlyBiomass();
	}
	// resources are updated each time step
								
	updateResourcesLowResMap();

	getDynamicRaster(eLRResources).updateCurrentMinMaxValues();

	//ExpandLR2HRRaster(eLRResources,eResources);
	
	// these rasters are only updated at the beginning of seasons
//	if ( !_climate.cellUpdateRequired() ) return;


	//updateSoilCondition();
}

const Climate & GujaratWorld::getClimate() const
{
	return _climate;
}

long int GujaratWorld::getNewKey()
{
	return _agentKey++;
}

float GujaratWorld::getBiomassVariation( bool wetSeason, Soils cellSoil, const Engine::Point2D<int> & index, int indexWeightWaterRaster ) const
{
	//return 10.0f;
	
	double variation = 0.0f;
	if(_config._biomassDistribution.compare("standard")==0 || 1==1)
	{
		if(wetSeason)
		{
			variation = _dailyRainSeasonBiomassIncrease.at(cellSoil);
		}
		else
		{
			variation = -_dailyDrySeasonBiomassDecrease.at(cellSoil);
		}
	}
	else if(_config._biomassDistribution.compare("linDecayFromWater")==0 || _config._biomassDistribution.compare("logDecayFromWater")==0)
	{
		if (indexWeightWaterRaster == -1)
		{
			indexWeightWaterRaster = eWeightWater;
		}
		variation = _config._waterDistConstant*getValue(indexWeightWaterRaster, index);
		//variation = 1600.0f*1600.0f*float(getValue("weightWater", index))/16423239174.0f;
		if(wetSeason)
		{
			variation *= (float)_dailyRainSeasonBiomassIncrease.at(cellSoil);			
		}
		else
		{
			variation *= -1.0f*(float)_dailyDrySeasonBiomassDecrease.at(cellSoil);
		}
		//std::cout << "variation: " << variation << " constant: " << _config._waterDistConstant << " weight: " << getValue("weightWater", index) << " increase: " <<_dailyRainSeasonBiomassIncrease.at(cellSoil) << " dist to water: " << getValue("distWater", index) << std::endl;
		//variation /= 100.0f;
	}
	else
	{
		std::stringstream oss;
		oss << "GujaratWorld::getBiomassVariation - unknown biomass distribution: " << _config._biomassDistribution;
		throw Engine::Exception(oss.str());

	}
	
	//std::cout << "variation " << variation << std::endl;
	return variation;
	
}	
	
void GujaratWorld::worldCell2LowResCell( const Engine::Point2D<int> pos, Engine::Point2D<int> & result ) const
{
	result._x = pos._x / _config._cellsPerLowResCellSide;
	result._y = pos._y / _config._cellsPerLowResCellSide;
}
	
void GujaratWorld::LowRes2HighResCellCorner(Engine::Point2D<int> pos, Engine::Point2D<int> &result ) const	
{
	result._x = pos._x * _config._cellsPerLowResCellSide;
	result._y = pos._y * _config._cellsPerLowResCellSide;
}	
	
	
int GujaratWorld::getLowResMapsSideSize() 
{ 
	return _overlapBoundaries._size._x / _config._cellsPerLowResCellSide; 
}
	
	

} // namespace Gujarat

