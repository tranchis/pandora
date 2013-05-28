
#ifndef __Sector_hxx__
#define __Sector_hxx__

#include <Point2D.hxx>
#include <Raster.hxx>
#include <vector>
#include <iosfwd>

namespace Engine
{
	class World;	
}

namespace Gujarat
{

	class GujaratWorld;
	
enum	BiomassAmountClass
{
	BIOMASS_AMOUNT_LOW = 0,
	BIOMASS_AMOUNT_MED,
	BIOMASS_AMOUNT_HI
};

class Sector
{
	//const Engine::World & _world; needed change due to use of LR methods for raster access.
	const GujaratWorld & _world;
	std::vector< Engine::Point2D<int> >	_cells;
	int					_biomassAmount;
//	BiomassAmountClass			_biomassAmountClass;

private:

	void	computeBiomassAmount( const Engine::Raster& r );
	void	computeBiomassAmountLR( const Engine::Raster& r );

public:
	//Sector( const Engine::World & world );
	Sector( const GujaratWorld & world );
//	Sector( const Sector& other );
	virtual ~Sector();

	bool		isEmpty() const { return _cells.empty(); }
	unsigned	numCells() const { return _cells.size(); }

	const 	std::vector< Engine::Point2D<int> > & cells() const { return _cells; }
	std::vector< Engine::Point2D<int> > & cellsFree() { return _cells; }


	void	addCell( Engine::Point2D<int>& p )
	{
		_cells.push_back( p );
	}

	void	clearCells() 
	{
		_cells.clear();
	}

	Engine::Point2D<int> getNearestTo( Engine::Point2D<int> p ) const;

	void	getAdjacent( Engine::Point2D<int> p,
				std::vector<Engine::Point2D<int> >& adjList ) const;

	int	getBiomassAmount() const
	{
		return _biomassAmount;
	}

	/*
	BiomassAmountClass	getBiomassAmountClass() const
	{
		return _biomassAmountClass;
	}
	*/

	void	updateFeatures();
	void	updateFeatures( const Engine::Raster& r );
	void	updateFeaturesLR( const Engine::Raster& r );

	//void	showFeatures( std::ostream& );
	//std::string	biomassClass() const;
	const GujaratWorld & getWorld() const;
};

class SectorBestFirstSortPtrVecPredicate
{
public:
	bool operator()( const Sector* s1, const Sector* s2 ) const
	{
		if ( s1->getBiomassAmount() > s2->getBiomassAmount() )
		{
			return true;
		}
		return false;
	}
};

} // namespace Gujarat

#endif // __Sector_hxx__

