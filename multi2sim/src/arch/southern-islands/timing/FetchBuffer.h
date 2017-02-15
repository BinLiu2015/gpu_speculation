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

#ifndef ARCH_SOUTHERN_ISLANDS_TIMING_FETCH_BUFFER_H
#define ARCH_SOUTHERN_ISLANDS_TIMING_FETCH_BUFFER_H

#include <memory>
#include <list>

#include "Uop.h"



namespace SI
{

// Forward declarations
class ComputeUnit;


/// Class representing a fetch buffer in the compute unit front-end
class FetchBuffer
{
	// Global fetch buffer identifier, assigned in constructor
	int id;

	// Compute unit that it belongs to, assigned in constructor
	ComputeUnit *compute_unit;

	// Buffer of instructions
	std::vector<std::list<std::unique_ptr<Uop>>> buffer;

	// Index of last fetched wavefront in the fetch buffer
	int last_fetched_wavefront_index;

	// Index of last dispatched wavefront in the fetch buffer
	int last_issued_wavefront_index;

	// Flag shows last fetched buffer entry is full
	bool last_fetched_entry_full;

	// Flag shows whether there is a successful issue to the execution unit
	bool branch_unit_issued;
	bool scalar_unit_issued;
	bool simd_unit_issued;
	bool vector_memory_issued;
	bool lds_unit_issued;

public:
	
	//
	// Static fields
	//

	/// Number of slots per fetch buffer entry
	static unsigned num_slots_per_entry;


	//
	// Class members
	//

	/// Constructor
	FetchBuffer(int id, ComputeUnit *compute_unit);

	/// Return the number of uops in the given fetch buffer entry
	int getEntrySize(int index) { return buffer[index].size(); }

	/// Return the number of the fetch buffer size
	int getBufferSize() { return buffer.size();}

	/// Return the identifier for this fetch buffer
	int getId() const { return id; }

	/// Add instruction to the end of the given buffer entry
	void addUop(int index, std::unique_ptr<SI::Uop> uop)
	{
		buffer[index].push_back(std::move(uop));
	}

	/// Return an iterator to the first uop of the entry in fetch buffer
	std::list<std::unique_ptr<Uop>>::iterator begin(int index)
	{
		return buffer[index].begin();
	}

	/// Return a past-the-end iterator to the given entry in fetch buffer
	std::list<std::unique_ptr<Uop>>::iterator end(int index)
	{
		return buffer[index].end();
	}

	/// Remove the uop pointed to by the given iterator.
	void Remove(int index, std::list<std::unique_ptr<Uop>>::iterator it);

	/// Check if the given entry in the fetch buffer is empty
	bool IsEmptyEntry(int index)
	{
		if (buffer[index].size() == 0)
			return true;
		else
			return false;
	}

	/// Check if the given entry in the fetch buffer is full(all slots are
	/// full)
	bool IsFullEntry(int index)
	{
		if (buffer[index].size() == num_slots_per_entry)
			return true;
		else
			return false;
	}

	/// Return the id of last fetched wavefront index in fetch buffer
	int getLastFetchedWavefrontIndex() const
	{
		return last_fetched_wavefront_index;
	}

	/// Return the id of last dispatched wavefront index in fetch buffer
	int getLastIssuedWavefrontIndex() const
	{
		return last_issued_wavefront_index;
	}

	/// Return the issued flag for different execution unit
	bool getBranchUnitIssuedFlag() const
	{
		return branch_unit_issued;
	}

	bool getScalarUnitIssuedFlag() const
	{
		return scalar_unit_issued;
	}

	bool getSIMDUnitIssuedFlag() const
	{
		return simd_unit_issued;
	}

	bool getVectorMemoryUnitIssuedFlag() const
	{
		return vector_memory_issued;
	}

	bool getLDSUnitIssuedFlag() const
	{
		return lds_unit_issued;
	}

	void setBranchUnitIssuedFlag(bool value)
	{
		branch_unit_issued = value;
	}

	void setScalarUnitIssuedFlag(bool value)
	{
		scalar_unit_issued = value;
	}

	void setVectorMemoryIssuedFlag(bool value)
	{
		vector_memory_issued = value;
	}

	void setSIMDUnitIssuedFlag(bool value)
	{
		simd_unit_issued = value;
	}

	void setLDSUnitIssuedFlag(bool value)
	{
		lds_unit_issued = value;
	}

	void setLastFatchedWavefrontIndex(int index)
	{
		last_fetched_wavefront_index = index;
 	}

	void setLastIssuedWavefrontIndex(int index)
	{
		last_issued_wavefront_index = index;
	}

	bool IsLastFetchedEntryFull(int index)
	{
		return last_fetched_entry_full;
	}

	void setLastFetchedEntryFull(bool value)
	{
		last_fetched_entry_full = value;
	}

	void ResetExecutionUnitIssueFlag();
};

}

#endif

