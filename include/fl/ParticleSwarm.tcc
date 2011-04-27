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
	T epsilon = std::numeric_limits<T>::epsilon ();
	if (this->toleranceF == 0) this->toleranceF = std::sqrt (epsilon);
	maxIterations = 200;
	minRandom = 1e-6;
	attractionGlobal = 1;
	attractionLocal = 1;
	constriction = 1;
	inertia = 1;
	decayRate = 1;
  }

  template<class T>
  void
  ParticleSwarm<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	const T minSlope  = 1e-3;
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
		p.position[d] += scales[d] * randfb ();
		p.velocity[d]  = scales[d] * randfb () / (T) 2;
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
	  searchable.dimension (bestParticle->bestPosition);

	  for (int i = 0; i < count; i++)
	  {
		Particle & p = particles[i];

		Vector<T> vl = p            .bestPosition - p.position;
		Vector<T> vg = bestParticle->bestPosition - p.position;
		T normL = vl.norm (2);
		T normG = vg.norm (2);
		T maxVelocity = constriction * std::max (normL, normG);
		maxVelocity = std::max (maxVelocity, minRandom);
		normL *= attractionLocal;
		normG *= attractionGlobal;
		T normR = std::max (normL, normG);
		normR = std::max (normR, minRandom);
		vl.normalize (normL);
		vg.normalize (normG);
		p.velocity = w * p.velocity + vl + vg;
		for (int j = 0; j < dimension; j++) p.velocity[j] += randfb () * normR;
		T normP = p.velocity.norm (2);
		if (normP > maxVelocity) p.velocity.normalize (maxVelocity);

		p.position += p.velocity;
		searchable.value (p.position, value);
		p.value = value.norm (2) * direction;

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
