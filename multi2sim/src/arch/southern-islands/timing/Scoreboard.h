/*
 *  Multi2Sim
 *  Copyright (C) 2015  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ARCH_SOUTHERN_ISLANDS_TIMING_SOCREBOARD_H
#define ARCH_SOUTHERN_ISLANDS_TIMING_SCOREBOARD_H

#include <vector>
#include <set>


namespace SI
{

// Forward declaration
class Instruction;
class ComputeUnit;
class Wavefront;
class Uop;

// Class representing the score board in a Compute Unit
class ScoreBoard
{
	// Keep track of pending writes to scalar registers.
	std::vector<std::set<unsigned>> scalar_register_table;

	// Keep track of pending writes to vector registers
	std::vector<std::set<unsigned>> vector_register_table;

	// Keep track of long operation pending writes to scalar registers such as
	// global store
	std::vector<std::set<unsigned>> long_operation_scalar_register_table;

	// Keep track of long operation pending writes to vector registers such as
	// global store
	std::vector<std::set<unsigned>> long_operation_vector_register_table;

	// SM that it belongs to, assigned in constructor
	ComputeUnit *compute_unit;

	// Scoreboard in Compute Unit
	int id;

public:

	/// Constructor
	ScoreBoard(int id, ComputeUnit *compute_unit);

	/// Reserve the given scalar register in the given wavefront
	void ReserveScalarRegister(Wavefront *wavefront, unsigned register_index);

	/// Reserve the given vector register in the given wavefront
	void ReserveVectorRegister(Wavefront *wavefront, unsigned register_index);

	/// Reserve registers destination registers in the given instruction which
	/// belongs to the given wavefront
	void ReserveRegisters(Wavefront *wavefront, Uop *uop);

	/// Reserve the predicate register in the given wavefront
	void ReservePredicate(Wavefront *wavefront, unsigned register_index);

	/// Release the given scalar register in the given wavefront
	void ReleaseScalarRegister(Wavefront *wavefront, unsigned register_index);

	/// Release the given scalar register in the given wavefront
	void ReleaseVectorRegister(Wavefront *wavefront, unsigned register_index);

	/// Release the given predicate in the given wavefront
	void ReleasePredicate(Wavefront *wavefront, unsigned predicate_index);

	/// Release registers used in the given instruction which belongs to the
	/// given wavefront
	void ReleaseRegisters(Wavefront *wavefront, Uop *uop);

	/// Check if the given instruction has collision with the score board
	/// register table. Return true if WAW or RAW hazard detected.(no WAR
	/// since in-order issue)
	bool CheckCollision(Wavefront *wavefront, Uop *uop);
};

} // namespace SI

#endif
