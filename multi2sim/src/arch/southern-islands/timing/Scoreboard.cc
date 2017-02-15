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

#include <cstring>

#include <arch/southern-islands/disassembler/Instruction.h>
#include <arch/southern-islands/emulator/Wavefront.h>

#include "ComputeUnit.h"


namespace SI
{

ScoreBoard::ScoreBoard(int id, ComputeUnit *compute_unit):
		compute_unit(compute_unit), id(id)
{
	scalar_register_table.resize(compute_unit->
					max_wavefronts_per_wavefront_pool);
	vector_register_table.resize(compute_unit->
					max_wavefronts_per_wavefront_pool);
	long_operation_scalar_register_table.resize(compute_unit->
				max_wavefronts_per_wavefront_pool);
	long_operation_vector_register_table.resize(compute_unit->
				max_wavefronts_per_wavefront_pool);
}


void ScoreBoard::ReserveScalarRegister(Wavefront *wavefront,
			unsigned register_index)
{
	int wavefront_pool_entry_index = wavefront->getWavefrontPoolEntry()
			->getIdInWavefrontPool();
	if (!(scalar_register_table[wavefront_pool_entry_index].find(register_index)
			== scalar_register_table[wavefront_pool_entry_index].end()))
	{
		throw misc::Panic(misc::fmt("Southern Island Scoreboard exception: "
				"trying to reserve a reserved register.\n Compute Unit ID = %d,"
				"Wavefront ID = %d, Register index = %u", compute_unit->getIndex(),
				wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool(),
				register_index));
	}

	scalar_register_table[wavefront_pool_entry_index].insert(register_index);
}

void ScoreBoard::ReserveVectorRegister(Wavefront *wavefront,
			unsigned register_index)
{
	int wavefront_pool_entry_index = wavefront->getWavefrontPoolEntry()
			->getIdInWavefrontPool();
	if (!(vector_register_table[wavefront_pool_entry_index].find(register_index)
			== vector_register_table[wavefront_pool_entry_index].end()))
	{
		throw misc::Panic(misc::fmt("Southern Island Scoreboard exception: "
				"trying to reserve a reserved register.\n Compute Unit ID = %d,"
				"Wavefront ID = %d, Register index = %u", compute_unit->getIndex(),
				wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool(),
				register_index));
	}

	vector_register_table[wavefront_pool_entry_index].insert(register_index);
}

void ScoreBoard::ReserveRegisters(Wavefront *wavefront, Uop *uop)
{
	// Get instruction format
	Instruction::Format format = uop->getInstruction()->getFormat();

	// Reserve destination scalar registers
	for (unsigned i = 0; i < 16; i++)
	{
		int index = uop->getDestinationScalarRegisterIndex(i);
		if (index > 0)
		{
			this->ReserveScalarRegister(wavefront, index);
		}
	}

	// Reserve destination vector registers
	for (unsigned i = 0; i < 4; i++)
	{
		int index = uop->getDestinationVectorRegisterIndex(i);
		if (index > 0)
			this->ReserveVectorRegister(wavefront, index);
	}

	// Track long latency scalar operations
	if (format == Instruction::FormatMTBUF || format == Instruction::FormatMUBUF
			|| format == Instruction::FormatMIMG)
	{
		for (unsigned i = 0; i < 16; i++)
		{
			int index = uop->getDestinationScalarRegisterIndex(i);
			if (index > 0)
			{
				this->long_operation_scalar_register_table
				[wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool()].
						insert(index);
			}
		}
	}

	// Track long latency vector operations
	if (format == Instruction::FormatMTBUF || format == Instruction::FormatMUBUF
			|| format == Instruction::FormatMIMG)
	{
		for (unsigned i = 0; i < 4; i++)
		{
			int index = uop->getDestinationVectorRegisterIndex(i);
			if (index > 0)
			{
				this->long_operation_vector_register_table
				[wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool()].
						insert(index);
			}
		}
	}
}


void ScoreBoard::ReleaseScalarRegister(Wavefront *wavefront,
			unsigned register_index)
{
	int wavefront_pool_entry_index = wavefront->getWavefrontPoolEntry()->
				getIdInWavefrontPool();
	if (scalar_register_table[wavefront_pool_entry_index].find(register_index)
			== scalar_register_table[wavefront_pool_entry_index].end())
		return;
	scalar_register_table[wavefront_pool_entry_index].erase(register_index);
}

void ScoreBoard::ReleaseVectorRegister(Wavefront *wavefront,
			unsigned register_index)
{
	int wavefront_pool_entry_index = wavefront->getWavefrontPoolEntry()->
				getIdInWavefrontPool();
	if (vector_register_table[wavefront_pool_entry_index].find(register_index) ==
			vector_register_table[wavefront_pool_entry_index].end())
		return;
	vector_register_table[wavefront_pool_entry_index].erase(register_index);
}

void ScoreBoard::ReleaseRegisters(Wavefront *wavefront, Uop *uop)
{
	// Get instruction format
	Instruction::Format format = uop->getInstruction()->getFormat();

	// Release scalar register
	for (unsigned i = 0; i < 16; i++)
	{
		int index = uop->getDestinationScalarRegisterIndex(i);
		if (index > 0)
			this->ReleaseScalarRegister(wavefront, index);
	}

	// Release vector register
	for (unsigned i = 0; i < 4; i++)
	{
		int index = uop->getDestinationVectorRegisterIndex(i);
		if (index > 0)
			this->ReleaseVectorRegister(wavefront, index);
	}

	// Release long latency scalar operations
	if (format == Instruction::FormatMTBUF || format == Instruction::FormatMUBUF
			|| format == Instruction::FormatMIMG)
	{
		for (unsigned i = 0; i < 16; i++)
		{
			int index = uop->getDestinationScalarRegisterIndex(i);
			if (index > 0)
			{
				this->long_operation_scalar_register_table
				[wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool()].
					erase(index);
			}
		}
	}

	// Release long latency vector operations
	if (format == Instruction::FormatMTBUF || format == Instruction::FormatMUBUF
			|| format == Instruction::FormatMIMG)
	{
		for (unsigned i = 0; i < 14; i++)
		{
			int index = uop->getDestinationVectorRegisterIndex(i);
			if (index > 0)
			{
				this->long_operation_vector_register_table
				[wavefront->getWavefrontPoolEntry()->getIdInWavefrontPool()].
					erase(index);
			}
		}
	}
}

bool ScoreBoard::CheckCollision(Wavefront *wavefront, Uop *uop)
{
	int wavefront_pool_entry_index = wavefront->getWavefrontPoolEntry()->
				getIdInWavefrontPool();

	// Get list of  destination registers
	std::set<int> instruction_scalar_registers;

	// Get list of vector registers
	std::set<int> instruction_vector_registers;

	std::set<int>::iterator index;

	// Insert destination scalar registers
	for (int i = 0; i < 16; i++)
		if (uop->getDestinationScalarRegisterIndex(i) > 0)
			instruction_scalar_registers.insert
				(uop->getDestinationScalarRegisterIndex(i));

	// Insert destination vector registers
	for (int i = 0; i < 4; i++)
		if (uop->getDestinationVectorRegisterIndex(i) > 0)
			instruction_vector_registers.insert
				(uop->getDestinationVectorRegisterIndex(i));

	// Insert source scalar registers
	for (int i = 0; i < 4; i++)
		if (uop->getSourceScalarRegisterIndex(i) > 0)
			instruction_scalar_registers.insert
				(uop->getDestinationScalarRegisterIndex(i));

	// Insert source vector registers
	for (int i = 0; i < 4; i++)
		if (uop->getSourceVectorRegisterIndex(i) > 0)
			instruction_vector_registers.insert
				(uop->getDestinationVectorRegisterIndex(i));

	// Check for collision scalar registers
	for (index = instruction_scalar_registers.begin();
			index != instruction_scalar_registers.end(); index++)
	{
		if (scalar_register_table[wavefront_pool_entry_index].find(*index) !=
				scalar_register_table[wavefront_pool_entry_index].end())
			return true;
	}

	// Check for collision vector registers
	for (index = instruction_vector_registers.begin();
			index != instruction_vector_registers.end(); index++)
	{
		if (vector_register_table[wavefront_pool_entry_index].find(*index) !=
				vector_register_table[wavefront_pool_entry_index].end())
			return true;
	}
	return false;
}


} // namespace SI
