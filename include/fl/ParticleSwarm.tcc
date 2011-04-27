/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_particle_swarm_tcc
#define fl_particle_swarm_tcc


#include "fl/search.h"
#include "fl/lapack.h"
#include "fl/random.h"

#include <vector>
#include <limits>


namespace fl
{
  // class ParticleSwarm ------------------------------------------------------

  template<class T>
  ParticleSwarm<T>::ParticleSwarm (int particleCount, T toleranceF, int patience)
  : particleCount (particleCount),
	toleranceF (toleranceF),
	patience (patience)
  {
	if (this->toleranceF == 0) this->toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	maxIterations = 200;
	attractionGlobal = 2;
	attractionLocal = 2;
	constriction = 0.8;
	inertia = 1;
	decayRate = 1;
  }

  template<class T>
  void
  ParticleSwarm<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	const T epsilon   = std::numeric_limits<T>::epsilon ();
	const T minSlope  = std::sqrt (epsilon);
	const T direction = toleranceF < 0 ? (T) -1 : (T) 1;

	SearchableGreedy<T> * greedy = dynamic_cast<SearchableGreedy<T> *> (&searchable);

	int dimension = point.rows ();
	int count = particleCount < 1 ? dimension : particleCount;
	std::vector<Particle> particles (count);

	// Initialize: Probe area around starting point to determine sensitivity.
	// Then spread initial particles at roughly the 1-sigma distance.

	Vector<T> scales;
	searchable.dimension (point);
	searchable.gradient (point, scales);
	if (greedy) point = greedy->bestPoint;
	for (int d = 0; d < dimension; d++)
	{
	  T & s = scales[d];
	  if (std::abs (s) < minSlope) s = s < (T) 0 ? (T) -1 : (T) 1;
	  else                         s = (T) 1 / s;
	}

	Particle * bestParticle = & particles[0];
	Vector<T> value;
	for (int i = 0; i < count; i++)
	{
	  Particle & p = particles[i];

	  p.position.copyFrom (point);
	  p.velocity.resize (dimension);
	  for (int d = 0; d < dimension; d++)
	  {
		p.position[d] += scales[d] * randGaussian ();
		p.velocity[d]  = scales[d] * randGaussian () / (T) 2;
	  }
	  p.bestPosition.copyFrom (p.position);

	  searchable.value (p.position, value);
	  p.value = value.norm (2) * direction;
	  p.bestValue = p.value;
	  if (p.value < bestParticle->value) bestParticle = &p;
	}

	// Iterate until convergence
	T lastBestValue = bestParticle->bestValue;
	int lastImprovement = 0;
	T w = inertia;
	for (int iteration = 0; iteration < maxIterations; iteration++)
	{
	  std::cerr << "=============================================" << std::endl;
	  std::cerr << iteration << std::endl;
	  std::cerr << "best = " << bestParticle->bestValue << " " << bestParticle->bestPosition << std::endl;
	  searchable.dimension (bestParticle->bestPosition);

	  for (int i = 0; i < count; i++)
	  {
		std::cerr << "  ----------------------------------------------" << std::endl;
		std::cerr << "  " << i << std::endl;

		Particle & p = particles[i];

		p.velocity = constriction
		             * (  w                           * p.velocity
		                + randf () * attractionGlobal * (bestParticle->bestPosition - p.position)
		                + randf () * attractionLocal  * (p            .bestPosition - p.position));

		p.position += p.velocity;
		searchable.value (p.position, value);
		p.value = value.norm (2) * direction;

		std::cerr << "    position = " << p.position << std::endl;
		std::cerr << "    velocity = " << p.velocity << std::endl;
		std::cerr << "    value    = " << p.value << std::endl;

		if (p.value < p.bestValue)
		{
		  p.bestValue = p.value;
		  p.bestPosition.copyFrom (p.position);
		  if (p.bestValue < bestParticle->bestValue) bestParticle = &p;
		}
	  }

	  // convergence check
	  if (bestParticle->bestValue < toleranceF) break;
	  if (bestParticle->bestValue < lastBestValue) lastImprovement = 0;
	  else if (++lastImprovement > patience) break;

	  w *= decayRate;
	}
	point = bestParticle->bestPosition;
  }
}


#endif
