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

#include <cassert>

#include "FetchBuffer.h"
#include "ComputeUnit.h"


namespace SI
{

unsigned FetchBuffer::num_slots_per_entry = 4;


FetchBuffer::FetchBuffer(int id, ComputeUnit *compute_unit) :
					id(id),
					compute_unit(compute_unit)
			{
				// Initialize the uop buffer
				buffer.resize(compute_unit->fetch_buffer_size *
							num_slots_per_entry);

				// Initialize the last fetched warp index
				last_fetched_wavefront_index = 0;

				// Initialize the last dispatched warp index
				last_issued_wavefront_index = -1;

				last_fetched_entry_full = false;

				branch_unit_issued = false;
				scalar_unit_issued = false;
				simd_unit_issued = false;
				vector_memory_issued = false;
				lds_unit_issued = false;
			}

void FetchBuffer::Remove(int index,
			std::list<std::unique_ptr<Uop>>::iterator it)
{
	assert(it != buffer[index].end());
	buffer[index].erase(it);
}

void FetchBuffer::ResetExecutionUnitIssueFlag()
{
	branch_unit_issued = false;
	scalar_unit_issued = false;
	simd_unit_issued = false;
	vector_memory_issued = false;
	lds_unit_issued = false;
}

}

