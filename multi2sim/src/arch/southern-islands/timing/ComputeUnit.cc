/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
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

#include <arch/southern-islands/disassembler/Instruction.h>
#include <arch/southern-islands/emulator/Emulator.h>
#include <arch/southern-islands/emulator/NDRange.h>
#include <arch/southern-islands/emulator/Wavefront.h>
#include <arch/southern-islands/emulator/WorkGroup.h>
#include <memory/Module.h>

#include "ComputeUnit.h"
#include "Timing.h"
#include "WavefrontPool.h"


namespace SI
{

int ComputeUnit::num_wavefront_pools = 4;
int ComputeUnit::max_work_groups_per_wavefront_pool = 10;
int ComputeUnit::max_wavefronts_per_wavefront_pool = 10; 
int ComputeUnit::fetch_latency = 1;
int ComputeUnit::fetch_width = 1;
int ComputeUnit::fetch_buffer_size = 10;
int ComputeUnit::issue_latency = 1;
int ComputeUnit::issue_width = 5;
int ComputeUnit::max_instructions_issued_per_type = 1;
int ComputeUnit::lds_size = 65536;
int ComputeUnit::lds_alloc_size = 64;
int ComputeUnit::lds_latency = 2;                                                      
int ComputeUnit::lds_block_size = 64;                                                  
int ComputeUnit::lds_num_ports = 2; 
	

ComputeUnit::ComputeUnit(int index, Gpu *gpu) :
		gpu(gpu),
		index(index),
		scalar_unit(this),
		branch_unit(this),
		lds_unit(this),
		vector_memory_unit(this)
{
	// Create the Lds module
	lds_module = misc::new_unique<mem::Module>(
			misc::fmt("LDS[%d]", index), 
			mem::Module::TypeLocalMemory,
			lds_num_ports, 
			lds_block_size,
			lds_latency);

	// Create wavefront pools, and SIMD units
	wavefront_pools.resize(num_wavefront_pools);
	fetch_buffers.resize(num_wavefront_pools);
	speculation_fetch_buffers.resize(num_wavefront_pools);
	pre_execution_buffers.resize(num_wavefront_pools);
	simd_units.resize(num_wavefront_pools);
	scoreboard.resize(num_wavefront_pools);
	for (int i = 0; i < num_wavefront_pools; i++)
	{
		wavefront_pools[i] = misc::new_unique<WavefrontPool>(i, this);
		fetch_buffers[i] = misc::new_unique<FetchBuffer>(i, this);
		speculation_fetch_buffers[i] = misc::new_unique<FetchBuffer>(i, this);
		pre_execution_buffers[i] = misc::new_unique<FetchBuffer>(i,this);
		simd_units[i] = misc::new_unique<SimdUnit>(this);
		scoreboard[i] = misc::new_unique<ScoreBoard>(i, this);
	}
}


void ComputeUnit::IssueToExecutionUnit(FetchBuffer *fetch_buffer,
		ExecutionUnit *execution_unit, int index, int &instruction_issued)
{
	// Find associated uop
	auto it = fetch_buffer->begin(index);
	Uop *uop = it->get();

	// Skip uops that have not completed fetch
	if (timing->getCycle() >= uop->fetch_ready)
	{

		long long compute_unit_id = uop->getIdInComputeUnit();
		int wavefront_id = uop->getWavefront()->getId();
		long long id_in_wavefront = uop->getIdInWavefront();

		//Issue to execution unit, erase from fetch buffer
		execution_unit->Issue(std::move(*it));


/*		/// test
		//if (fetch_buffer->getId() == 0 && uop->getComputeUnit()->getIndex() == 0)
		if (uop->getComputeUnit()->getIndex() == 0)
		{
			//if (uop->getWavefront()->getId() <=3 )
				std::cout<<"Wavefront ID  "<<uop->getWavefront()->getId()<<
				" Issue pc" << uop->getPC() <<"in cycle"<<
						timing->getCycle()<<std::endl;
		}

*/
		fetch_buffer->Remove(index,it);

/*		/// test
		if (uop->getWavefront()->getId() == 0)
		{
			uop->getWavefront()->num_issued_instructions++;
			std::cout<<"number of issued instructions " <<
					uop->getWavefront()->num_issued_instructions << "in cycle"
					<<timing->getCycle() <<"PC"<<uop->getPC()<<std::endl;
		}
*/
		// Issue successfully, set flag to 1
		instruction_issued = 1;

		// Trace
		Timing::trace << misc::fmt("si.inst "
				"id=%lld "
				"cu=%d "
				"wf=%d "
				"uop_id=%lld "
				"stg=\"i\"\n",
				compute_unit_id,
				index,
				wavefront_id,
				id_in_wavefront);
	}
}


void ComputeUnit::Issue(FetchBuffer *fetch_buffer,
			WavefrontPool *wavefront_pool)
{
	timing = Timing::getInstance();

	// Set up variables
	int instructions_processed = 0;

	// Number of unavailable units in current cycle. Maximum is 5.
	int total_unavailable_units = 0;

	FetchBuffer *speculation_fetch_buffer =
			speculation_fetch_buffers[fetch_buffer->getId()].get();

	if (Timing::issue_mode == 1)
	{
	for (int i = 0; i < max_wavefronts_per_wavefront_pool; i++)
	{
		int instructions_issued_in_current_wavefront = 0;

		// Only issue a fixed number of instructions per cycle
		//if (instructions_processed == issue_width - total_unavailable_units)
		if (instructions_processed == issue_width)
			break;

		// Get wavefront pool entry index
		unsigned index = (fetch_buffer->getLastIssuedWavefrontIndex() + i)
						% max_wavefronts_per_wavefront_pool;

		// Get wavefront pool entry
		WavefrontPoolEntry *wavefront_pool_entry = (*(wavefront_pool->begin() +
					index)).get();

		// Wavefront is ready but waiting on outstanding
		// memory instructions
		if (wavefront_pool_entry->mem_wait)
		{
			// No outstanding accesses

			if (!wavefront_pool_entry->lgkm_cnt &&
				!wavefront_pool_entry->exp_cnt  &&
				!wavefront_pool_entry->vm_cnt)
/*
			if (wavefront_pool_entry->lgkm_cnt ==
						wavefront_pool_entry->remain_lgkm_cnt &&
					wavefront_pool_entry->exp_cnt ==
							wavefront_pool_entry->remain_exp_cnt &&
					wavefront_pool_entry->vm_cnt ==
							wavefront_pool_entry->remain_vm_cnt)
*/
			{
					wavefront_pool_entry->mem_wait = false;

					// Set execution mode to normal
					if (wavefront_pool_entry->execution_mode == 1)
					{
						wavefront_pool_entry->execution_mode = 0;
						wavefront_pool_entry->specuation_to_normal = 1;
						//if (num_issued_instructions_in_speculation_mode > 0)
						//	this->num_total_speculation_mode++;
					}
			/*		Timing::pipeline_debug << misc::fmt(
							"wg=%d/wf=%d "
							"Mem-wait:Done\n",
							wavefront->
							getWorkGroup()->
							getId(),
							wavefront->getId());
			*/
			}
			else
			{

				/// Stall cycle
				if (wavefront_pool_entry->execution_mode == 0)
				{

					/// stall cycle
					this->long_latency_stall_cycles++;

					//continue;
					break;


				}
					// TODO show a waiting state in Visualization
					// tool for the wait.
/*
					Timing::pipeline_debug << misc::fmt(
							"wg=%d/wf=%d "
							"Waiting-Mem\n",
							wavefront->getWorkGroup()->
							getId(),
							wavefront->getId());
*/
			}

		}

		// Wavefront is ready but waiting at barrier
		if (wavefront_pool_entry->wait_for_barrier)
			continue;

		// Check if the slot in the fetch buffer entry is valid for issue.
		if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
		{
			if (speculation_fetch_buffer->IsEmptyEntry(index))
				/// Stall cycle
				continue;
				//break;
		}
		else
		{
			if (fetch_buffer->IsEmptyEntry(index))
				/// Stall cycle
				continue;
				//break;
		}

		// Get wavefront
		Wavefront *wavefront = wavefront_pool_entry->getWavefront();

		// No wavefront
		if (!wavefront)
			continue;

		// Find the associated uop
		auto it = fetch_buffer->begin(index);
		if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
			it = speculation_fetch_buffer->begin(index);
		Uop *uop = it->get();

		// Make sure all instructions are finished before s_endpgm
/*
		if (uop->wavefront_last_instruction)
		{
			if (uop->getWavefront()->inflight_instructions != 1)
				break;
		}
*/
		// Read registers index used and instruction format in new
		// instruction
		uop->setInstructionRegistersIndex();

		// Check collision
		if (scoreboard[wavefront_pool->getId()]->CheckCollision(wavefront, uop))
		{
			if (scoreboard[wavefront_pool->getId()]->
								IsLongOpCollision(wavefront, uop))
				wavefront_pool_entry->long_operation_wait = true;

			/// Stall cycle
			continue;
			//break;
		}

		// Issue instructions to branch unit
		if (branch_unit.isValidUop(uop))
		{
			if (branch_unit.canIssue())
			{
				if (!fetch_buffer->getBranchUnitIssuedFlag())
				{
					if (instructions_issued_in_current_wavefront == 0)
					{
						if (!timing->IsHintFileEmpty() &&
								wavefront_pool_entry->execution_mode)
						{
							IssueToExecutionUnit(speculation_fetch_buffer,
									&branch_unit, index,
									instructions_issued_in_current_wavefront);
							this->num_total_speculation_instructions++;
							this->num_branch_speculation_instructions++;
							num_issued_instructions_in_speculation_mode++;
						}
						else
							IssueToExecutionUnit(fetch_buffer, &branch_unit,
								index, instructions_issued_in_current_wavefront);

						fetch_buffer->setBranchUnitIssuedFlag(true);
						instructions_processed++;
						scoreboard[wavefront_pool->getId()]->
								ReserveRegisters(wavefront, uop);
						continue;
					}
				}
			}
			else
				total_unavailable_units ++;
		}

		// Issue instructions to scalar unit
		if (scalar_unit.isValidUop(uop))
		{
			if (scalar_unit.canIssue())
			{
				if (!fetch_buffer->getScalarUnitIssuedFlag())
				{
					if (instructions_issued_in_current_wavefront == 0)
					{
						if (!timing->IsHintFileEmpty() &&
										wavefront_pool_entry->execution_mode)
						{
							IssueToExecutionUnit(speculation_fetch_buffer,
								&scalar_unit, index,
									instructions_issued_in_current_wavefront);
							this->num_total_speculation_instructions++;
							this->num_scalar_speculation_instructions++;
							if (uop->scalar_memory_read)
								this->num_scalar_memory_speculation_instructions++;
							num_issued_instructions_in_speculation_mode++;
						}
						else
							IssueToExecutionUnit(fetch_buffer, &scalar_unit,
								index, instructions_issued_in_current_wavefront);

						fetch_buffer->setScalarUnitIssuedFlag(true);
						instructions_processed++;
						scoreboard[wavefront_pool->getId()]->
								ReserveRegisters(wavefront, uop);

						// if uop is in memory wait state, the instruction is
						// waitcnt. Set the execution mode to pre-execution mode
						// since we need to wait waitcnt finish to issue the
						// instructions after it.
						if (uop->memory_wait)
						{
							uop->getWavefrontPoolEntry()->mem_wait = true;
/*							if (uop->lgkm_cnt == 31 )
								uop->getWavefrontPoolEntry()->remain_lgkm_cnt =
										0;
							else
								uop->getWavefrontPoolEntry()->remain_lgkm_cnt =
									uop->lgkm_cnt;

							if (uop->vm_cnt == 15)
								uop->getWavefrontPoolEntry()->remain_vm_cnt =
										0;
							else
								uop->getWavefrontPoolEntry()->remain_vm_cnt =
									uop->vm_cnt;

							if (uop->exp_cnt == 7)
								uop->getWavefrontPoolEntry()->remain_exp_cnt =
										0;
							else
								uop->getWavefrontPoolEntry()->remain_exp_cnt =
									uop->exp_cnt;
*/
							if (!timing->IsHintFileEmpty())
							{
								wavefront_pool_entry->execution_mode = 1;
								num_issued_instructions_in_speculation_mode = 0;
								num_total_speculation_mode++;
								// When there is a mode transferring, flash
								// the fetch buffer
								//fetch_buffer->ClearFetchBufferEntry(index);

								// put all uop in fetch buffer to preexectuion buffer
							}
						}

/*						if (uop->getInstruction()->getFormat() ==
									Instruction::FormatSMRD)
						{
								uop->getWavefrontPoolEntry()->lgkm_cnt++;
								if(wavefront_pool_entry->execution_mode == 1)
									uop->getWavefrontPoolEntry()->
										remain_lgkm_cnt++;
						}
*/
						if (uop->at_barrier)
						{
							// Set a flag to wait until all wavefronts have
							// reached the barrier
							assert(!uop->getWavefrontPoolEntry()->wait_for_barrier);
							wavefront_pool_entry->wait_for_barrier = true;
						}
						continue;
					}
				}
			}
			else
				total_unavailable_units ++;
		}

#if 0
		if (!fetch_buffer->getSIMDUnitIssuedFlag())
		{
			for (auto &simd_unit : simd_units)
				IssueToExecutionUnit(fetch_buffer, simd_unit.get());
		}
#endif

		// Issue instructions to SIMD units
		int id = fetch_buffer->getId();
		if (simd_units[id]->isValidUop(uop))
		{
			if (simd_units[id]->canIssue())
			{
				if (!fetch_buffer->getSIMDUnitIssuedFlag())
				{

					if (instructions_issued_in_current_wavefront == 0)
					{
						if (!timing->IsHintFileEmpty() &&
										wavefront_pool_entry->execution_mode)
						{
							IssueToExecutionUnit(speculation_fetch_buffer,
									simd_units[id].get(), index,
									instructions_issued_in_current_wavefront);
							this->num_total_speculation_instructions++;
							this->num_simd_speculation_instructions++;
							num_issued_instructions_in_speculation_mode++;
						}
						else
							IssueToExecutionUnit(fetch_buffer,
									simd_units[id].get(), index,
									instructions_issued_in_current_wavefront);
						fetch_buffer->setSIMDUnitIssuedFlag(true);
						instructions_processed++;
						scoreboard[wavefront_pool->getId()]->
								ReserveRegisters(wavefront, uop);
						continue;
					}
				}
			}
			else
				total_unavailable_units++;
		}

		// Issue instructions to vector memory unit
		if (vector_memory_unit.isValidUop(uop))
		{
			if (vector_memory_unit.canIssue())
			{
				if (!fetch_buffer->getVectorMemoryUnitIssuedFlag())
				{
					if (instructions_issued_in_current_wavefront == 0)
					{
						if (!timing->IsHintFileEmpty() &&
										wavefront_pool_entry->execution_mode)
						{
							IssueToExecutionUnit(speculation_fetch_buffer,
									&vector_memory_unit, index,
									instructions_issued_in_current_wavefront);
							this->num_total_speculation_instructions++;
							this->num_vector_memory_speculation_instructions++;
							num_issued_instructions_in_speculation_mode++;
						}
						else
							IssueToExecutionUnit(fetch_buffer,
									&vector_memory_unit, index,
									instructions_issued_in_current_wavefront);

						fetch_buffer->setVectorMemoryIssuedFlag(true);
						instructions_processed++;
						scoreboard[wavefront_pool->getId()]->
								ReserveRegisters(wavefront, uop);
						continue;
					}
				}
			}
			else
				total_unavailable_units++;
		}

		// Issue instructions to LDS unit
		if (lds_unit.isValidUop(uop))
		{
			if (lds_unit.canIssue())
			{
				if (!fetch_buffer->getLDSUnitIssuedFlag())
				{
					if (instructions_issued_in_current_wavefront == 0)
					{
						if (!timing->IsHintFileEmpty() &&
								wavefront_pool_entry->execution_mode)
						{
							IssueToExecutionUnit(speculation_fetch_buffer,
									&lds_unit, index,
									instructions_issued_in_current_wavefront);
							this->num_total_speculation_instructions++;
							this->num_lds_speculation_instructions++;
							num_issued_instructions_in_speculation_mode++;
						}
						else
							IssueToExecutionUnit(fetch_buffer,
									&lds_unit, index,
									instructions_issued_in_current_wavefront);

						fetch_buffer->setLDSUnitIssuedFlag(true);
						instructions_processed++;
						scoreboard[wavefront_pool->getId()]->
								ReserveRegisters(wavefront, uop);
						continue;
					}
				}
			}
			else
				total_unavailable_units++;
		}

#if 0
		// Update visualization states for all instructions not issued
		for (auto it = fetch_buffer->begin(),
				e = fetch_buffer->end();
				it != e;
				++it)
		{
			// Get Uop
			Uop *uop = it->get();

			// Skip uops that have not completed fetch
			if (timing->getCycle() < uop->fetch_ready)
				continue;

			// Trace
			Timing::trace << misc::fmt("si.inst "
					"id=%lld "
					"cu=%d "
					"wf=%d "
					"uop_id=%lld "
					"stg=\"s\"\n",
					uop->getIdInComputeUnit(),
					index,
					uop->getWavefront()->getId(),
					uop->getIdInWavefront());
		}
#endif
	}

	int last_issued_wavefront = fetch_buffer->getLastIssuedWavefrontIndex();
	last_issued_wavefront = (last_issued_wavefront + 1) %
				max_wavefronts_per_wavefront_pool;
	fetch_buffer->setLastIssuedWavefrontIndex(last_issued_wavefront);
	if (instructions_processed > 0)
	{
		this->last_issue_cycle = timing->getCycle();
		this->wavefront_pool_issued_cycles++;
	}

	/*
	if (this->index == 0 && wavefront_pool->getId() == 0)
	{
		this->wavefront_pool_cycles++;
		if (instructions_processed > 0)
			this->wavefront_pool_issued_cycles++;
	}
*/
	}
	else if (Timing::issue_mode == 0)
	{
		int normal_list_size = wavefront_pool->getNormalListSize();
		int speculation_list_size = wavefront_pool->getSpeculationListSize();
		int total_size = normal_list_size + speculation_list_size;
		std::list<int> normal_to_speculation_value;
		std::list<int> speculation_to_normal_value;

		// !!!!
		/*
		if (normal_list_size > 0)
			total_size = normal_list_size;
		else
			total_size= speculation_list_size;
		 */
		//for (int i = 0; i < max_wavefronts_per_wavefront_pool; i++)
		for (int i = 0; i < total_size; i++)
		{
			int instructions_issued_in_current_wavefront = 0;

			// Only issue a fixed number of instructions per cycle
			//if (instructions_processed == issue_width - total_unavailable_units)
			if (instructions_processed == issue_width)
				break;

			// Get wavefront pool entry index
			unsigned index, list_index;

			if (i < normal_list_size)
			{
				list_index = (fetch_buffer->getLastIssuedNormalListIndex()+ i)
							% normal_list_size;
				index = wavefront_pool->getNormalListIndexValue(list_index);
			}
			else
			{
				list_index = (fetch_buffer->getLastIssuedSpeculationListIndex() +
						(i-normal_list_size)) % speculation_list_size;
				index = wavefront_pool->getSpeculationListIndexValue(list_index);
			}

/*
			if (normal_list_size > 0)
			{
				list_index = (fetch_buffer->getLastIssuedNormalListIndex()+ i)
							% normal_list_size;
				index = wavefront_pool->getNormalListIndexValue(list_index);
			}
			else
			{

				list_index = (fetch_buffer->getLastIssuedSpeculationListIndex()
							+ i) % speculation_list_size;
				index = wavefront_pool->getSpeculationListIndexValue(list_index);

			}
*/


			// Get wavefront pool entry
			WavefrontPoolEntry *wavefront_pool_entry = (*(wavefront_pool->begin() +
						index)).get();

			// Wavefront is ready but waiting on outstanding
			// memory instructions
			if (wavefront_pool_entry->mem_wait)
			{
				// No outstanding accesses
				if (!wavefront_pool_entry->lgkm_cnt &&
					!wavefront_pool_entry->exp_cnt &&
					!wavefront_pool_entry->vm_cnt)
/*				if (wavefront_pool_entry->lgkm_cnt ==
						wavefront_pool_entry->remain_lgkm_cnt &&
					wavefront_pool_entry->exp_cnt ==
							wavefront_pool_entry->remain_exp_cnt &&
					wavefront_pool_entry->vm_cnt ==
							wavefront_pool_entry->remain_vm_cnt)
*/
				{
						wavefront_pool_entry->mem_wait = false;

						// Set execution mode to normal
						if (wavefront_pool_entry->execution_mode == 1)
						{
							wavefront_pool_entry->execution_mode = 0;
							wavefront_pool_entry->specuation_to_normal = 1;
							//if (num_issued_instructions_in_speculation_mode > 0)
							//	this->num_total_speculation_mode++;

							speculation_to_normal_value.push_back(index);
						}
				/*		Timing::pipeline_debug << misc::fmt(
								"wg=%d/wf=%d "
								"Mem-wait:Done\n",
								wavefront->
								getWorkGroup()->
								getId(),
								wavefront->getId());
				*/
				}
				else
				{

					if (wavefront_pool_entry->execution_mode == 0)
					{

						/// stall cycle
						this->long_latency_stall_cycles++;
						break;
						// continue;
					}
						// TODO show a waiting state in Visualization
						// tool for the wait.
	/*
						Timing::pipeline_debug << misc::fmt(
								"wg=%d/wf=%d "
								"Waiting-Mem\n",
								wavefront->getWorkGroup()->
								getId(),
								wavefront->getId());
	*/
				}

			}

			///  Only update the wavefront status
			/// Only fetch normal wavefront when there are normal wavefront available
			if (normal_list_size > 0 && i >= normal_list_size)
				continue;


			// Wavefront is ready but waiting at barrier
			if (wavefront_pool_entry->wait_for_barrier)
				continue;

			// Check if the slot in the fetch buffer entry is valid for issue.
			if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
			{
				if (speculation_fetch_buffer->IsEmptyEntry(index))
					continue;
			}
			else
			{
				if (fetch_buffer->IsEmptyEntry(index))
					continue;
			}

			// Get wavefront
			Wavefront *wavefront = wavefront_pool_entry->getWavefront();

			// No wavefront
			if (!wavefront)
				continue;

			// Find the associated uop
			auto it = fetch_buffer->begin(index);
			if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
				it = speculation_fetch_buffer->begin(index);
			Uop *uop = it->get();

			// Make sure all instructions are finished before s_endpgm
	/*
			if (uop->wavefront_last_instruction)
			{
				if (uop->getWavefront()->inflight_instructions != 1)
					break;
			}
	*/
			// Read registers index used and instruction format in new
			// instruction
			uop->setInstructionRegistersIndex();

			// Check collision
			if (scoreboard[wavefront_pool->getId()]->CheckCollision(wavefront, uop))
			{
				if (scoreboard[wavefront_pool->getId()]->
									IsLongOpCollision(wavefront, uop))
					wavefront_pool_entry->long_operation_wait = true;

				// break;
				continue;
			}

			// Issue instructions to branch unit
			if (branch_unit.isValidUop(uop))
			{
				if (branch_unit.canIssue())
				{
					if (!fetch_buffer->getBranchUnitIssuedFlag())
					{
						if (instructions_issued_in_current_wavefront == 0)
						{
							if (!timing->IsHintFileEmpty() &&
									wavefront_pool_entry->execution_mode)
							{
								IssueToExecutionUnit(speculation_fetch_buffer,
										&branch_unit, index,
										instructions_issued_in_current_wavefront);
								this->num_total_speculation_instructions++;
								this->num_branch_speculation_instructions++;
								num_issued_instructions_in_speculation_mode++;
							}
							else
								IssueToExecutionUnit(fetch_buffer, &branch_unit,
									index, instructions_issued_in_current_wavefront);

							fetch_buffer->setBranchUnitIssuedFlag(true);
							instructions_processed++;
							scoreboard[wavefront_pool->getId()]->
									ReserveRegisters(wavefront, uop);
							continue;
						}
					}
				}
				else
					total_unavailable_units ++;
			}

			// Issue instructions to scalar unit
			if (scalar_unit.isValidUop(uop))
			{
				if (scalar_unit.canIssue())
				{
					if (!fetch_buffer->getScalarUnitIssuedFlag())
					{
						if (instructions_issued_in_current_wavefront == 0)
						{
							if (!timing->IsHintFileEmpty() &&
											wavefront_pool_entry->execution_mode)
							{
								IssueToExecutionUnit(speculation_fetch_buffer,
									&scalar_unit, index,
										instructions_issued_in_current_wavefront);
								this->num_total_speculation_instructions++;
								this->num_scalar_speculation_instructions++;
								if (uop->scalar_memory_read)
									this->num_scalar_memory_speculation_instructions++;
								num_issued_instructions_in_speculation_mode++;
							}
							else
								IssueToExecutionUnit(fetch_buffer, &scalar_unit,
									index, instructions_issued_in_current_wavefront);

							fetch_buffer->setScalarUnitIssuedFlag(true);
							instructions_processed++;
							scoreboard[wavefront_pool->getId()]->
									ReserveRegisters(wavefront, uop);

							// if uop is in memory wait state, the instruction is
							// waitcnt. Set the execution mode to pre-execution mode
							// since we need to wait waitcnt finish to issue the
							// instructions after it.
							if (uop->memory_wait)
							{
								uop->getWavefrontPoolEntry()->mem_wait = true;
/*								if (uop->lgkm_cnt == 31 )
									uop->getWavefrontPoolEntry()->remain_lgkm_cnt =
											0;
								else
									uop->getWavefrontPoolEntry()->remain_lgkm_cnt =
										uop->lgkm_cnt;

								if (uop->vm_cnt == 15)
									uop->getWavefrontPoolEntry()->remain_vm_cnt =
											0;
								else
									uop->getWavefrontPoolEntry()->remain_vm_cnt =
										uop->vm_cnt;

								if (uop->exp_cnt == 7)
									uop->getWavefrontPoolEntry()->remain_exp_cnt =
											0;
								else
									uop->getWavefrontPoolEntry()->remain_exp_cnt =
										uop->exp_cnt;
*/
								if (!timing->IsHintFileEmpty())
								{
									wavefront_pool_entry->execution_mode = 1;
									num_issued_instructions_in_speculation_mode
										= 0;
									num_total_speculation_mode++;
									normal_to_speculation_value.push_back(index);
									// When there is a mode transferring, flash
									// the fetch buffer
									//fetch_buffer->ClearFetchBufferEntry(index);

									// put all uop in fetch buffer to preexectuion buffer
								}
							}

/*							if (uop->getInstruction()->getFormat() ==
										Instruction::FormatSMRD)
							{
									uop->getWavefrontPoolEntry()->lgkm_cnt++;
									if(wavefront_pool_entry->execution_mode == 1)
										uop->getWavefrontPoolEntry()->
											remain_lgkm_cnt++;
							}
*/
							if (uop->at_barrier)
							{
								// Set a flag to wait until all wavefronts have
								// reached the barrier
								assert(!uop->getWavefrontPoolEntry()->wait_for_barrier);
								wavefront_pool_entry->wait_for_barrier = true;
							}
							continue;
						}
					}
				}
				else
					total_unavailable_units ++;
			}

	#if 0
			if (!fetch_buffer->getSIMDUnitIssuedFlag())
			{
				for (auto &simd_unit : simd_units)
					IssueToExecutionUnit(fetch_buffer, simd_unit.get());
			}
	#endif

			// Issue instructions to SIMD units
			int id = fetch_buffer->getId();
			if (simd_units[id]->isValidUop(uop))
			{
				if (simd_units[id]->canIssue())
				{
					if (!fetch_buffer->getSIMDUnitIssuedFlag())
					{

						if (instructions_issued_in_current_wavefront == 0)
						{
							if (!timing->IsHintFileEmpty() &&
											wavefront_pool_entry->execution_mode)
							{
								IssueToExecutionUnit(speculation_fetch_buffer,
										simd_units[id].get(), index,
										instructions_issued_in_current_wavefront);
								this->num_total_speculation_instructions++;
								this->num_simd_speculation_instructions++;
								num_issued_instructions_in_speculation_mode++;
							}
							else
								IssueToExecutionUnit(fetch_buffer,
										simd_units[id].get(), index,
										instructions_issued_in_current_wavefront);
							fetch_buffer->setSIMDUnitIssuedFlag(true);
							instructions_processed++;
							scoreboard[wavefront_pool->getId()]->
									ReserveRegisters(wavefront, uop);
							continue;
						}
					}
				}
				else
					total_unavailable_units++;
			}

			// Issue instructions to vector memory unit
			if (vector_memory_unit.isValidUop(uop))
			{
				if (vector_memory_unit.canIssue())
				{
					if (!fetch_buffer->getVectorMemoryUnitIssuedFlag())
					{
						if (instructions_issued_in_current_wavefront == 0)
						{
							if (!timing->IsHintFileEmpty() &&
											wavefront_pool_entry->execution_mode)
							{
								IssueToExecutionUnit(speculation_fetch_buffer,
										&vector_memory_unit, index,
										instructions_issued_in_current_wavefront);
								this->num_total_speculation_instructions++;
								this->num_vector_memory_speculation_instructions++;
								num_issued_instructions_in_speculation_mode++;
							}
							else
								IssueToExecutionUnit(fetch_buffer,
										&vector_memory_unit, index,
										instructions_issued_in_current_wavefront);

							fetch_buffer->setVectorMemoryIssuedFlag(true);
							instructions_processed++;
							scoreboard[wavefront_pool->getId()]->
									ReserveRegisters(wavefront, uop);
							continue;
						}
					}
				}
				else
					total_unavailable_units++;
			}

			// Issue instructions to LDS unit
			if (lds_unit.isValidUop(uop))
			{
				if (lds_unit.canIssue())
				{
					if (!fetch_buffer->getLDSUnitIssuedFlag())
					{
						if (instructions_issued_in_current_wavefront == 0)
						{
							if (!timing->IsHintFileEmpty() &&
									wavefront_pool_entry->execution_mode)
							{
								IssueToExecutionUnit(speculation_fetch_buffer,
										&lds_unit, index,
										instructions_issued_in_current_wavefront);
								this->num_total_speculation_instructions++;
								this->num_lds_speculation_instructions++;
								num_issued_instructions_in_speculation_mode++;
							}
							else
								IssueToExecutionUnit(fetch_buffer,
										&lds_unit, index,
										instructions_issued_in_current_wavefront);

							fetch_buffer->setLDSUnitIssuedFlag(true);
							instructions_processed++;
							scoreboard[wavefront_pool->getId()]->
									ReserveRegisters(wavefront, uop);
							continue;
						}
					}
				}
				else
					total_unavailable_units++;
			}


			if (timing->IsHintFileEmpty())
			{
				if(instructions_processed == 1)
					break;
			}

	#if 0
			// Update visualization states for all instructions not issued
			for (auto it = fetch_buffer->begin(),
					e = fetch_buffer->end();
					it != e;
					++it)
			{
				// Get Uop
				Uop *uop = it->get();

				// Skip uops that have not completed fetch
				if (timing->getCycle() < uop->fetch_ready)
					continue;

				// Trace
				Timing::trace << misc::fmt("si.inst "
						"id=%lld "
						"cu=%d "
						"wf=%d "
						"uop_id=%lld "
						"stg=\"s\"\n",
						uop->getIdInComputeUnit(),
						index,
						uop->getWavefront()->getId(),
						uop->getIdInWavefront());
			}
	#endif
		}

		//int last_issued_wavefront = fetch_buffer->getLastIssuedWavefrontIndex();
		//last_issued_wavefront = (last_issued_wavefront + 1) %
		//			max_wavefronts_per_wavefront_pool;
		//fetch_buffer->setLastIssuedWavefrontIndex(last_issued_wavefront);

		if (normal_to_speculation_value.size() > 0)
		{
			for (auto i = normal_to_speculation_value.begin();
						i != normal_to_speculation_value.end(); i++)
			{
				int value;
				// Get the value
				value = *i;

				// Erase the index in normal list
				wavefront_pool->eraseNormalList(value);

				// Add the index to speculation list
				wavefront_pool->push_backSpeculationList(value);
			}
			std::cout<<"!!!!!"<<wavefront_pool->getNormalListSize()<<"in cycle "<<timing->getCycle()
					<<std::endl;
			std::cout<<"!!!!!"<<wavefront_pool->getSpeculationListSize()<<"in cycle "<<timing->getCycle()
					<<std::endl;
		}

		if (speculation_to_normal_value.size() > 0)
		{
			for (auto i = speculation_to_normal_value.begin();
					i!= speculation_to_normal_value.end(); i++)
			{
				int value;

				value = *i;

				// Erase the index in speculation list
				wavefront_pool->eraseSpeculationList(value);

				// Add the index to normal list
				wavefront_pool->push_backNormalList(value);
			}

			std::cout<<"*****"<<wavefront_pool->getNormalListSize()<<"in cycle "<<timing->getCycle()
							<<std::endl;
			std::cout<<"*****"<<wavefront_pool->getSpeculationListSize()<<"in cycle "<<timing->getCycle()
							<<std::endl;
		}

		int last_issued_normal_list_index =
					fetch_buffer->getLastIssuedNormalListIndex();
		if (wavefront_pool->getNormalListSize() > 0)
			last_issued_normal_list_index = (last_issued_normal_list_index + 1)
				% wavefront_pool->getNormalListSize();
		else
			last_issued_normal_list_index = 0;

		fetch_buffer->setLastIssuedNormalListIndex
			(last_issued_normal_list_index);

		int last_issued_speculation_list_index =
					fetch_buffer->getLastIssuedSpeculationListIndex();
		if (wavefront_pool->getSpeculationListSize() > 0)
			last_issued_speculation_list_index = (last_issued_speculation_list_index
				+ 1) % wavefront_pool->getSpeculationListSize();
		else
			last_issued_speculation_list_index = 0;

		fetch_buffer->setLastIssuedSpeculationListIndex
			(last_issued_speculation_list_index);

		if (instructions_processed > 0)
			this->last_issue_cycle = timing->getCycle();
	}
}


void ComputeUnit::Fetch(FetchBuffer *fetch_buffer,
		WavefrontPool *wavefront_pool)
{
	timing = Timing::getInstance();

	// Checks
	assert(fetch_buffer);
	assert(wavefront_pool);
	assert(fetch_buffer->getId() == wavefront_pool->getId());

	// Set up variables
	int instructions_processed = 0;

	FetchBuffer *speculation_fetch_buffer = speculation_fetch_buffers
				[fetch_buffer->getId()].get();

	FetchBuffer *pre_execution_buffer = pre_execution_buffers
				[fetch_buffer->getId()].get();

	// Fetch the instructions
	for (int i = 0; i < max_wavefronts_per_wavefront_pool; i++)
	{

#if 0
		// Stall if fetch buffer is full
		assert(fetch_buffer->getSize() <= fetch_buffer_size);
		if (fetch_buffer->getSize() == fetch_buffer_size)
			break;
#endif

		// Only fetch a fixed number of instructions per cycle
		if (instructions_processed == fetch_width)
			break;

		// Get wavefront pool entry index.
		unsigned index = (fetch_buffer->getLastFetchedWavefrontIndex() + i)
						% max_wavefronts_per_wavefront_pool;

		// Get wavefront pool entry
		WavefrontPoolEntry *wavefront_pool_entry = (*(wavefront_pool->begin() +
						index)).get();

		// when i=0 check if last fetched entry is full, if full, switch to next
		// entry
		if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
		{
			if (speculation_fetch_buffer->IsFullEntry(index) == 1 && i == 0)
			{
				index = (index + 1) % max_wavefronts_per_wavefront_pool;
				// Reread wavefront pool entry
				wavefront_pool_entry = (*(wavefront_pool->begin() +
							index)).get();
			}
		}
		else
		{
			if (fetch_buffer->IsFullEntry(index) == 1 && i == 0)
			{
				index = (index + 1) % max_wavefronts_per_wavefront_pool;
				// Reread wavefront pool entry
				wavefront_pool_entry = (*(wavefront_pool->begin() +
							index)).get();
			}
		}

		// Check if the entry is ready to fetch, if the buffer entry is full,
		// break the loop
		if (!timing->IsHintFileEmpty() && wavefront_pool_entry->execution_mode)
		{
			if (speculation_fetch_buffer->IsFullEntry(index))
			{
				//speculation_fetch_buffer->setLastFetchedWavefrontIndex(index);
				fetch_buffer->setLastFetchedWavefrontIndex(index);
				break;
			}
		}
		else
		{
			if (fetch_buffer->IsFullEntry(index))
			{
				fetch_buffer->setLastFetchedWavefrontIndex(index);
				break;
			}
		}

		// Get wavefront
		Wavefront *wavefront = wavefront_pool_entry->getWavefront();

		// No wavefront
		if (!wavefront)
			continue;

		// Check wavefront
		assert(wavefront->getWavefrontPoolEntry());
		assert(wavefront->getWavefrontPoolEntry() ==
				wavefront_pool_entry);

#if 0
		// This should always be checked, regardless of how many
		// instructions have been fetched
		if (wavefront_pool_entry->ready_next_cycle)
		{
			wavefront_pool_entry->ready = true;
			wavefront_pool_entry->ready_next_cycle = false;
			continue;
		}
#endif

#if 0
		// Wavefront is not ready (previous instructions is still
		// in flight
		if (!wavefront_pool_entry->ready)
			continue;
#endif

		// If the wavefront finishes, there still may be outstanding
		// memory operations, so if the entry is marked finished
		// the wavefront must also be finished, but not vice-versa
		if (wavefront_pool_entry->wavefront_finished)
		{
			assert(wavefront->getFinished());
			continue;
		}

		// Wavefront is finished but other wavefronts from the
		// workgroup remain.  There may still be outstanding
		// memory operations, but no more instructions should
		// be fetched.
		if (wavefront->getFinished())
		{
			continue;
		}

		// Wavefront is ready but waiting at barrier
	//	if (wavefront_pool_entry->wait_for_barrier)
		//	continue;

		if (!timing->IsHintFileEmpty())
		{
			if (wavefront_pool_entry->execution_mode == 1)
				///compare pre-execution pc to see if there is
				// continuous instructions to execute????????????????????????
				// mode transfer flash the buffer?
			{
				long long pre_fetch_pc;

				/// check if there is any uops in fetch buffer, if there is
				/// move to pre_execution buffer
				if (!fetch_buffer->IsEmptyEntry(index))
				{
					while(!fetch_buffer->IsEmptyEntry(index))
					{
						auto it = fetch_buffer->begin(index);
						Uop *uop1 = it->get();

						// decrease inflight instructions
						uop1->getWorkGroup()->inflight_instructions--;
						uop1->getWavefront()->inflight_instructions--;

/*						/// test
						if (wavefront->getId() == 0)
						{
						std::cout << "Entry Size" <<
								fetch_buffer->getEntrySize(index) <<std::endl;

						std::cout << uop1->getPC() << "wavefront id "<<
								wavefront->getId() <<std::endl;
						std::cout << "inflight_instructions" <<
								uop1->getWavefront()->inflight_instructions <<
								std::endl;
						}
*/
						wavefront->num_fetched_instructions--;

						// Add the uop to pre_execution buffer
						pre_execution_buffer->addUop(index, std::move(*it));

						// Remove the uop from fetch buffer
						fetch_buffer->Remove(index,it);
					}
					//测一下fetch_buffer size
					//clear entry!!!!
				}

				/// how about waiting pc to change that!!!!!!!!!!!!
				pre_fetch_pc = wavefront_pool_entry->pre_fetch_pc;

				Timing::hint_format hint;
				if (timing->instruction_hint.count(pre_fetch_pc))
					hint = timing->instruction_hint[pre_fetch_pc];
				else
				{
					hint.mode = 0;
					hint.offset1 = 0;
					pre_fetch_pc = 0;
				}

				unsigned int hint_mode;
				int pc_offset;
				hint_mode = hint.mode;
				if (hint_mode == 0)
					pc_offset = hint.offset1;
				//else pc_offset = hint.offset2;
				else if (hint_mode == 1) // branch instruction
					{
						if (wavefront->getPC() == pre_fetch_pc + 4)
							pc_offset = hint.offset1;
						else
							pc_offset = hint.offset2;
					}

				long long target_pc = pre_fetch_pc + pc_offset;

				if (target_pc >= wavefront->getPC())
				{
					while(wavefront->getPC() <= target_pc)
					{
						long long pc = wavefront->getPC();
						wavefront->Execute();

						// Create uop
						auto uop = misc::new_unique<Uop>(
								wavefront,
								wavefront_pool_entry,
								timing->getCycle(),
								wavefront->getWorkGroup(),
								fetch_buffer->getId());
						uop->vector_memory_read = wavefront->vector_memory_read;
						uop->vector_memory_write = wavefront->vector_memory_write;
						uop->vector_memory_atomic = wavefront->vector_memory_atomic;
						uop->scalar_memory_read = wavefront->scalar_memory_read;
						uop->lds_read = wavefront->lds_read;
						uop->lds_write = wavefront->lds_write;
						uop->wavefront_last_instruction = wavefront->finished;
						uop->memory_wait = wavefront->memory_wait;
						uop->at_barrier = wavefront->isBarrierInstruction();
						uop->setInstruction(wavefront->getInstruction());
						uop->vector_memory_global_coherency =
								wavefront->vector_memory_global_coherency;
						uop->lgkm_cnt = wavefront->getLgkmcnt();
						uop->vm_cnt = wavefront->getVmcnt();
						uop->exp_cnt = wavefront->getExpcnt();
						uop->setPC(pc);

						// Update last memory accesses
						for (auto it = wavefront->getWorkItemsBegin(),
								e = wavefront->getWorkItemsEnd();
								it != e;
								++it)
						{
							// Get work item
							WorkItem *work_item = it->get();

							// Get uop work item info
							Uop::WorkItemInfo *work_item_info;
							work_item_info =
								&uop->work_item_info_list
										[work_item->getIdInWavefront()];

							// Global memory
							work_item_info->global_memory_access_address =
									work_item->global_memory_access_address;
							work_item_info->global_memory_access_size =
									work_item->global_memory_access_size;

							// LDS
							work_item_info->lds_access_count =
								work_item->lds_access_count;
							for (int j = 0; j < work_item->lds_access_count; j++)
							{
								work_item_info->lds_access[j].type =
									work_item->lds_access[j].type;
								work_item_info->lds_access[j].addr =
									work_item->lds_access[j].addr;
								work_item_info->lds_access[j].size =
									work_item->lds_access[j].size;
							}
						}

						if (uop->getPC() != target_pc )
							pre_execution_buffer->addUop(index,std::move(uop));
						else
						{
							// Access instruction cache. Record the time when the
							// instruction will have been fetched, as per the latency
							// of the instruction memory.
							uop->fetch_ready = timing->getCycle() + fetch_latency;

							// Insert uop into fetch buffer
							uop->getWorkGroup()->inflight_instructions++;
							uop->getWavefront()->inflight_instructions++;

/*
							/// test
							if (uop->getWavefront()->getId() == 0)
								std::cout<< "specualtion fetch pc" <<
										uop->getPC()<<"at cycle "
										<< timing->getCycle()<< std::endl;

							/// test
							if (wavefront->getId() == 0)
							{
								wavefront->num_fetched_instructions++;
								std::cout<<"number of fetched instructions" <<
										wavefront->num_fetched_instructions <<
										"in cycle"<<timing->getCycle()<<"PC "<<
										uop->getPC()<<std::endl;

							}
*/
							// Add uop to speculation buffer
							speculation_fetch_buffer->
								addUop(index,std::move(uop));
						}
					}
				}
				else if (target_pc == 0)  /// ??????? if there is no more preexecution instruction by hint
				{
					continue;
					//break;
				}
				else if (target_pc < wavefront->getPC())
				{

					for (auto it = pre_execution_buffer->begin(index),
							   e = pre_execution_buffer->end(index);
										it != e;
										++it)
					{
						Uop *uop = it->get();
						if(uop->getPC() == target_pc)
						{

							// Access instruction cache. Record the time when the
							// instruction will have been fetched, as per the latency
							// of the instruction memory.
							uop->fetch_ready = timing->getCycle() + fetch_latency;

							// Insert uop into fetch buffer
							uop->getWorkGroup()->inflight_instructions++;
							uop->getWavefront()->inflight_instructions++;

							// Set new normal execution pc
							wavefront_pool_entry->normal_fetch_pc =
										uop->getPC();

/*
							/// test
							if (uop->getWavefront()->getId() == 0)
								std::cout<< "specualtion fetch pc" <<
										uop->getPC()<<"at cycle "
										<< timing->getCycle()<< std::endl;

							/// test
							if (wavefront->getId() == 0)
							{
								wavefront->num_fetched_instructions++;
								std::cout<<"number of fetched instructions" <<
										wavefront->num_fetched_instructions
										<< "in cycle"
										<<timing->getCycle() <<"PC "
										<<uop->getPC()<<std::endl;
							}
*/
							// Add the uop to fetch buffer
							speculation_fetch_buffer
								->addUop(index, std::move(*it));

							// Remove the uop from pre-execution buffer
							pre_execution_buffer->Remove(index, it);
							break;
						}
					}
				}
				wavefront_pool_entry->pre_fetch_pc = target_pc;
			}
			else if (wavefront_pool_entry->execution_mode == 0)
			{
				//if (wavefront_pool_entry->normal_execution_pc <
					//		wavefront_pool_entry->pre_execution_pc)
				if (!pre_execution_buffer->IsEmptyEntry(index) &&
						!speculation_fetch_buffer->IsEmptyEntry(index))
				{
					Uop *uop, *uop1, *uop2;
					auto it = pre_execution_buffer->begin(index);
					auto e = speculation_fetch_buffer->begin(index);
					auto f = speculation_fetch_buffer->begin(index);
					uop = it->get();
					uop1 = e->get();

					// check speculation
					e++;
					while (e != speculation_fetch_buffer->end(index))
					{
						uop2 = e->get();
						if (uop1->getPC() > uop2->getPC())
						{
							uop1 = e->get();
							f = e;
						}
						++e;
					}

					if (uop->getPC() < uop1->getPC())
					{
						// Access instruction cache. Record the time when the
						// instruction will have been fetched, as per the latency
						// of the instruction memory.
						uop->fetch_ready = timing->getCycle() + fetch_latency;

						// Insert uop into fetch buffer
						uop->getWorkGroup()->inflight_instructions++;
						uop->getWavefront()->inflight_instructions++;

						// Set new normal execution pc
						wavefront_pool_entry->normal_fetch_pc = uop->getPC();

/*
						/// test
						if (uop->getWavefront()->getId() == 0)
							std::cout<< "fetch pc" <<
									uop->getPC()<<"at cycle "
									<< timing->getCycle()<< std::endl;

						/// test
						if (wavefront->getId() == 0)
						{
							wavefront->num_fetched_instructions++;
							std::cout<<"number of fetched instructions" <<
									wavefront->num_fetched_instructions
									<< "in cycle"<<timing->getCycle()
									<<"PC "<<uop->getPC()
									<<std::endl;
						}
*/
						// Add the uop to fetch buffer
						fetch_buffer->addUop(index, std::move(*it));

						// Remove from pre-execution buffer
						pre_execution_buffer->Remove(index, it);
					}
					else
					{
						// Access instruction cache. Record the time when the
						// instruction will have been fetched, as per the latency
						// of the instruction memory.
						uop1->fetch_ready = timing->getCycle() + fetch_latency;


						// Insert uop into fetch buffer
						// do not add inlight instructions number any more.
						// uop1->getWorkGroup()->inflight_instructions++;
						// uop1->getWavefront()->inflight_instructions++;

						// Set new normal execution pc
						wavefront_pool_entry->normal_fetch_pc = uop1->getPC();

/*
						/// test
						if (uop->getWavefront()->getId() == 0)
							std::cout<< "fetch pc" <<
									uop1->getPC()<<"at cycle "
									<< timing->getCycle()<< std::endl;
*/
						wavefront->num_fetched_instructions--;

/*
						/// test
						if (wavefront->getId() == 0)
						{
							wavefront->num_fetched_instructions++;
							std::cout<<"number of fetched instructions" <<
									wavefront->num_fetched_instructions
									<< "in cycle"<<timing->getCycle()
									<<"PC "<<uop1->getPC()<<std::endl;

						}
*/
						// Add the uop to fetch buffer
						fetch_buffer->addUop(index, std::move(*f));

						// Remove from speculation fetch buffer
						speculation_fetch_buffer->Remove(index, f);
					}
				}
				else if (!pre_execution_buffer->IsEmptyEntry(index) &&
							speculation_fetch_buffer->IsEmptyEntry(index))
				{
					Uop *uop;
					auto it = pre_execution_buffer->begin(index);
					uop = it->get();

					// Access instruction cache. Record the time when the
					// instruction will have been fetched, as per the latency
					// of the instruction memory.
					uop->fetch_ready = timing->getCycle() + fetch_latency;

					// Insert uop into fetch buffer
					uop->getWorkGroup()->inflight_instructions++;
					uop->getWavefront()->inflight_instructions++;

					// Set new normal execution pc
					wavefront_pool_entry->normal_fetch_pc = uop->getPC();

/*
					/// test
					if (uop->getWavefront()->getId() == 0)
						std::cout<< "fetch pc" <<
								uop->getPC()<<"at cycle "
								<< timing->getCycle()<< std::endl;

					/// test
					if (wavefront->getId() == 0)
					{
						wavefront->num_fetched_instructions++;
						std::cout<<"number of fetched instructions" <<
								wavefront->num_fetched_instructions << "in cycle"
								<<timing->getCycle()<<"PC "
								<<uop->getPC()<<std::endl;

					}
*/
					// Add the uop to fetch buffer
					fetch_buffer->addUop(index, std::move(*it));

					// Remove from pre-execution buffer
					pre_execution_buffer->Remove(index, it);

				}
				else if (pre_execution_buffer->IsEmptyEntry(index) &&
						!speculation_fetch_buffer->IsEmptyEntry(index))
				{
					Uop *uop, *uop1;
					auto it = speculation_fetch_buffer->begin(index);
					auto e = speculation_fetch_buffer->begin(index);

					uop = it->get();

					// check speculation
					it++;
					while (it != speculation_fetch_buffer->end(index))
					{
						uop1 = it->get();
						if (uop->getPC() > uop1->getPC())
						{
							uop = it->get();
							e = it;
						}
						++it;
					}
					// Access instruction cache. Record the time when the
					// instruction will have been fetched, as per the latency
					// of the instruction memory.
					uop->fetch_ready = timing->getCycle() + fetch_latency;

					// Insert uop into fetch buffer
					// do not add inflight instruction number any more
					//uop->getWorkGroup()->inflight_instructions++;
					//uop->getWavefront()->inflight_instructions++;

					// Set new normal execution pc
					wavefront_pool_entry->normal_fetch_pc = uop->getPC();

/*
					/// test
					if (uop->getWavefront()->getId() == 0)
						std::cout<< "fetch pc" <<
								uop->getPC()<<"at cycle "
								<< timing->getCycle()<< std::endl;
*/

					wavefront->num_fetched_instructions--;

/*
					/// test
					if (wavefront->getId() == 0)
					{
						wavefront->num_fetched_instructions++;
						std::cout<<"number of fetched instructions" <<
								wavefront->num_fetched_instructions << "in cycle"
								<<timing->getCycle()<<"PC"
								<<uop->getPC()<<std::endl;

					}
*/
					// Add the uop to fetch buffer
					fetch_buffer->addUop(index, std::move(*e));

					// Remove from pre-execution buffer
					speculation_fetch_buffer->Remove(index, e);
				}
				else
				{
					long long pc = wavefront->getPC();

					// Emulate instructions
					wavefront->Execute();

					// Create uop
					auto uop = misc::new_unique<Uop>(
							wavefront,
							wavefront_pool_entry,
							timing->getCycle(),
							wavefront->getWorkGroup(),
							fetch_buffer->getId());
					uop->vector_memory_read = wavefront->vector_memory_read;
					uop->vector_memory_write = wavefront->vector_memory_write;
					uop->vector_memory_atomic = wavefront->vector_memory_atomic;
					uop->scalar_memory_read = wavefront->scalar_memory_read;
					uop->lds_read = wavefront->lds_read;
					uop->lds_write = wavefront->lds_write;
					uop->wavefront_last_instruction = wavefront->finished;
					uop->memory_wait = wavefront->memory_wait;
					uop->at_barrier = wavefront->isBarrierInstruction();
					uop->setInstruction(wavefront->getInstruction());
					uop->vector_memory_global_coherency =
							wavefront->vector_memory_global_coherency;
					uop->lgkm_cnt = wavefront->getLgkmcnt();
					uop->vm_cnt = wavefront->getVmcnt();
					uop->exp_cnt = wavefront->getExpcnt();
					uop->setPC(pc);

					// Checks
					assert(wavefront->getWorkGroup() && uop->getWorkGroup());

					// Convert instruction name to string
					if (Timing::trace || Timing::pipeline_debug)
					{
						std::string instruction_name = wavefront->
								getInstruction()->getName();
						misc::StringSingleSpaces(instruction_name);

						// Trace
						Timing::trace << misc::fmt("si.new_inst "
								"id=%lld "
								"cu=%d "
								"ib=%d "
								"wf=%d "
								"uop_id=%lld "
								"stg=\"f\" "
								"asm=\"%s\"\n",
								uop->getIdInComputeUnit(),
								index,
								uop->getWavefrontPoolId(),
								uop->getWavefront()->getId(),
								uop->getIdInWavefront(),
								instruction_name.c_str());

						// Debug
						Timing::pipeline_debug << misc::fmt(
								"wg=%d/wf=%d cu=%d wfPool=%d "
								"inst=%lld asm=%s id_in_wf=%lld\n"
								"\tinst=%lld (Fetch)\n",
								uop->getWavefront()->getWorkGroup()->
								getId(),
								uop->getWavefront()->getId(),
								index,
								uop->getWavefrontPoolId(),
								uop->getId(),
								instruction_name.c_str(),
								uop->getIdInWavefront(),
								uop->getId());
					}

					// Update last memory accesses
					for (auto it = wavefront->getWorkItemsBegin(),
							e = wavefront->getWorkItemsEnd();
							it != e;
							++it)
					{
						// Get work item
						WorkItem *work_item = it->get();

						// Get uop work item info
						Uop::WorkItemInfo *work_item_info;
						work_item_info =
							&uop->work_item_info_list[work_item->getIdInWavefront()];

						// Global memory
						work_item_info->global_memory_access_address =
								work_item->global_memory_access_address;
						work_item_info->global_memory_access_size =
								work_item->global_memory_access_size;

						// LDS
						work_item_info->lds_access_count =
							work_item->lds_access_count;
						for (int j = 0; j < work_item->lds_access_count; j++)
						{
							work_item_info->lds_access[j].type =
								work_item->lds_access[j].type;
							work_item_info->lds_access[j].addr =
								work_item->lds_access[j].addr;
							work_item_info->lds_access[j].size =
								work_item->lds_access[j].size;
						}
					}

					// Access instruction cache. Record the time when the
					// instruction will have been fetched, as per the latency
					// of the instruction memory.
					uop->fetch_ready = timing->getCycle() + fetch_latency;

					/// Set pre-fetch pc when the wait_cnt instruction if
					/// fetched
					if (uop->memory_wait)
					{
						wavefront_pool_entry->pre_fetch_pc = uop->getPC();
					}

					// Insert uop into fetch buffer
					uop->getWorkGroup()->inflight_instructions++;
					uop->getWavefront()->inflight_instructions++;

					// Set new normal execution pc
					wavefront_pool_entry->normal_fetch_pc = uop->getPC();

/*
					/// test
					if (uop->getWavefront()->getId() == 0)
						std::cout<< "fetch pc" <<
								uop->getPC()<<"at cycle "
								<< timing->getCycle()<< std::endl;

					/// test
					if (wavefront->getId() == 0)
					{
						wavefront->num_fetched_instructions++;
						std::cout<<"number of fetched instructions" <<
							wavefront->num_fetched_instructions << "in cycle"
								<<timing->getCycle()<<"PC "
								<<uop->getPC()<<std::endl;

					}
*/
					fetch_buffer->addUop(index, std::move(uop));
				}
			}
		}
		else
		{
			long long pc = wavefront->getPC();

			// Emulate instructions
			wavefront->Execute();

			// Create uop
			auto uop = misc::new_unique<Uop>(
					wavefront,
					wavefront_pool_entry,
					timing->getCycle(),
					wavefront->getWorkGroup(),
					fetch_buffer->getId());
			uop->vector_memory_read = wavefront->vector_memory_read;
			uop->vector_memory_write = wavefront->vector_memory_write;
			uop->vector_memory_atomic = wavefront->vector_memory_atomic;
			uop->scalar_memory_read = wavefront->scalar_memory_read;
			uop->lds_read = wavefront->lds_read;
			uop->lds_write = wavefront->lds_write;
			uop->wavefront_last_instruction = wavefront->finished;
			uop->memory_wait = wavefront->memory_wait;
			uop->at_barrier = wavefront->isBarrierInstruction();
			uop->setInstruction(wavefront->getInstruction());
			uop->vector_memory_global_coherency =
					wavefront->vector_memory_global_coherency;
			uop->lgkm_cnt = wavefront->getLgkmcnt();
			uop->vm_cnt = wavefront->getVmcnt();
			uop->exp_cnt = wavefront->getExpcnt();
			uop->setPC(pc);

/*
			/// test
			if (uop->getWavefront()->getId() == 0)
				std::cout<< "specualtion fetch pc" <<
						uop->getPC()<<"at cycle "
						<< timing->getCycle()<< std::endl;

			/// test
			if (wavefront->getId() == 0)
			{
				wavefront->num_fetched_instructions++;
				std::cout<<"number of fetched instructions" <<
						wavefront->num_fetched_instructions <<
						"in cycle"<<timing->getCycle()<<"PC "<<
						uop->getPC()<<std::endl;

			}
*/

			// Checks
			assert(wavefront->getWorkGroup() && uop->getWorkGroup());

			// Convert instruction name to string
			if (Timing::trace || Timing::pipeline_debug)
			{
				std::string instruction_name = wavefront->
						getInstruction()->getName();
				misc::StringSingleSpaces(instruction_name);

				// Trace
				Timing::trace << misc::fmt("si.new_inst "
						"id=%lld "
						"cu=%d "
						"ib=%d "
						"wf=%d "
						"uop_id=%lld "
						"stg=\"f\" "
						"asm=\"%s\"\n",
						uop->getIdInComputeUnit(),
						index,
						uop->getWavefrontPoolId(),
						uop->getWavefront()->getId(),
						uop->getIdInWavefront(),
						instruction_name.c_str());

				// Debug
				Timing::pipeline_debug << misc::fmt(
						"wg=%d/wf=%d cu=%d wfPool=%d "
						"inst=%lld asm=%s id_in_wf=%lld\n"
						"\tinst=%lld (Fetch)\n",
						uop->getWavefront()->getWorkGroup()->
						getId(),
						uop->getWavefront()->getId(),
						index,
						uop->getWavefrontPoolId(),
						uop->getId(),
						instruction_name.c_str(),
						uop->getIdInWavefront(),
						uop->getId());
			}

			// Update last memory accesses
			for (auto it = wavefront->getWorkItemsBegin(),
					e = wavefront->getWorkItemsEnd();
					it != e;
					++it)
			{
				// Get work item
				WorkItem *work_item = it->get();

				// Get uop work item info
				Uop::WorkItemInfo *work_item_info;
				work_item_info =
					&uop->work_item_info_list[work_item->getIdInWavefront()];

				// Global memory
				work_item_info->global_memory_access_address =
						work_item->global_memory_access_address;
				work_item_info->global_memory_access_size =
						work_item->global_memory_access_size;

				// LDS
				work_item_info->lds_access_count =
					work_item->lds_access_count;
				for (int j = 0; j < work_item->lds_access_count; j++)
				{
					work_item_info->lds_access[j].type =
						work_item->lds_access[j].type;
					work_item_info->lds_access[j].addr =
						work_item->lds_access[j].addr;
					work_item_info->lds_access[j].size =
						work_item->lds_access[j].size;
				}
			}

			// Access instruction cache. Record the time when the
			// instruction will have been fetched, as per the latency
			// of the instruction memory.
			uop->fetch_ready = timing->getCycle() + fetch_latency;

/*			/// test
			if (uop->getPC() == 68)
				std::cout << uop->fetch_ready << std::endl;
*/
			// Insert uop into fetch buffer
			uop->getWorkGroup()->inflight_instructions++;
			uop->getWavefront()->inflight_instructions++;

			fetch_buffer->addUop(index, std::move(uop));
		}

			// Check if the current entry is full
			instructions_processed++;
			num_total_instructions++;
			if (instructions_processed != 0)
				fetch_buffer->setLastFetchedWavefrontIndex(index);
	}
}


void ComputeUnit::MapWorkGroup(WorkGroup *work_group)
{
	// Checks
	assert(work_group);
	assert((int) work_groups.size() <= gpu->getWorkGroupsPerComputeUnit());
	assert(!work_group->id_in_compute_unit);

	// Find an available slot
	while (work_group->id_in_compute_unit < gpu->getWorkGroupsPerComputeUnit()
			&& (work_group->id_in_compute_unit < 
			(int) work_groups.size()) && 
			(work_groups[work_group->id_in_compute_unit] != nullptr))
		work_group->id_in_compute_unit++;

	// Checks
	assert(work_group->id_in_compute_unit <
			gpu->getWorkGroupsPerComputeUnit());

	// Save timing simulator
	timing = Timing::getInstance();

	// Debug
	Emulator::scheduler_debug << misc::fmt("@%lld available slot %d "
			"found in compute unit %d\n",
			timing->getCycle(),
			work_group->id_in_compute_unit,
			index);

	// Insert work group into the list
	AddWorkGroup(work_group);

	// Checks
	assert((int) work_groups.size() <= gpu->getWorkGroupsPerComputeUnit());
	
	// If compute unit is not full, add it back to the available list
	if ((int) work_groups.size() < gpu->getWorkGroupsPerComputeUnit())
	{
		if (!in_available_compute_units)
			gpu->InsertInAvailableComputeUnits(this);
	}

	// Assign wavefront identifiers in compute unit
	int wavefront_id = 0;
	for (auto it = work_group->getWavefrontsBegin();
			it != work_group->getWavefrontsEnd();
			++it)
	{
		// Get wavefront pointer
		Wavefront *wavefront = it->get();

		// Set wavefront Id
		wavefront->id_in_compute_unit = work_group->id_in_compute_unit *
				work_group->getWavefrontsInWorkgroup() +
				wavefront_id;

		// Update internal counter
		wavefront_id++;
	}

	// Set wavefront pool for work group
	int wavefront_pool_id = work_group->id_in_compute_unit %
			num_wavefront_pools;
	work_group->wavefront_pool = wavefront_pools[wavefront_pool_id].get();

	// Check if the wavefronts in the work group can fit into the wavefront
	// pool
	assert((int) work_group->getWavefrontsInWorkgroup() <=
			max_wavefronts_per_wavefront_pool);

	// Insert wavefronts into an instruction buffer
	work_group->wavefront_pool->MapWavefronts(work_group);

	// Increment count of mapped work groups
	num_mapped_work_groups++;

	num_mapped_wavefronts = num_mapped_wavefronts +
				work_group->getNumWavefronts();

	// Debug info
	Emulator::scheduler_debug << misc::fmt("\t\tfirst wavefront=%d, "
			"count=%d\n"
			"\t\tfirst work-item=%d, count=%d\n",
			work_group->getWavefront(0)->getId(),
			work_group->getNumWavefronts(),
			work_group->getWorkItem(0)->getId(),
			work_group->getNumWorkItems());

	// Trace info
	Timing::trace << misc::fmt("si.map_wg "
				   "cu=%d "
				   "wg=%d "
				   "wi_first=%d "
				   "wi_count=%d "
				   "wf_first=%d "
				   "wf_count=%d\n",
				   index, work_group->getId(),
				   work_group->getWorkItem(0)->getId(),
				   work_group->getNumWorkItems(),
				   work_group->getWavefront(0)->getId(),
				   work_group->getNumWavefronts());
}

void ComputeUnit::AddWorkGroup(WorkGroup *work_group)
{
	// Add a work group only if the id in compute unit is the id for a new 
	// work group in the compute unit's list
	int index = work_group->id_in_compute_unit;
	if (index == (int) work_groups.size() &&
			(int) work_groups.size() < 
			gpu->getWorkGroupsPerComputeUnit())
	{
		work_groups.push_back(work_group);
	}
	else
	{
		// Make sure an entry is emptied up
		assert(work_groups[index] == nullptr);
		assert(work_groups.size() <=
				(unsigned) gpu->getWorkGroupsPerComputeUnit());

		// Set the new work group to the empty entry
		work_groups[index] = work_group;
	}

	// Save iterator 
	auto it = work_groups.begin();
	std::advance(it, work_group->getIdInComputeUnit()); 
	work_group->compute_unit_work_groups_iterator = it;

	// Debug info
	Emulator::scheduler_debug << misc::fmt("\twork group %d "
			"added\n",
			work_group->getId());
}


void ComputeUnit::RemoveWorkGroup(WorkGroup *work_group)
{
	// Debug info
	Emulator::scheduler_debug << misc::fmt("@%lld work group %d "
			"removed from compute unit %d slot %d\n",
			timing->getCycle(),
			work_group->getId(),
			index,
			work_group->id_in_compute_unit);

	// Erase work group                                                      
	assert(work_group->compute_unit_work_groups_iterator != 
			work_groups.end());
	work_groups[work_group->id_in_compute_unit] = nullptr;
}


void ComputeUnit::UnmapWorkGroup(WorkGroup *work_group)
{
	// Get Gpu object
	Gpu *gpu = getGpu();

	// Add work group register access statistics to compute unit
	num_sreg_reads += work_group->getSregReadCount();
	num_sreg_writes += work_group->getSregWriteCount();
	num_vreg_reads += work_group->getVregReadCount();
	num_vreg_writes += work_group->getVregWriteCount();

	// Remove the work group from the list
	assert(work_groups.size() > 0);
	RemoveWorkGroup(work_group);

	// Unmap wavefronts from instruction buffer
	work_group->wavefront_pool->UnmapWavefronts(work_group);
	
	// If compute unit is not already in the available list, place
	// it there. The vector list of work groups does not shrink,
	// when we unmap a workgroup.
	assert((int) work_groups.size() <= gpu->getWorkGroupsPerComputeUnit());
	if (!in_available_compute_units)
		gpu->InsertInAvailableComputeUnits(this);

	// Trace
	Timing::trace << misc::fmt("si.unmap_wg cu=%d wg=%d\n", index,
			work_group->getId());

	// Remove the work group from the running work groups list
	NDRange *ndrange = work_group->getNDRange();
	ndrange->RemoveWorkGroup(work_group);
}


void ComputeUnit::UpdateFetchVisualization(FetchBuffer *fetch_buffer)
{
	for (int i = 0; i < fetch_buffer->getBufferSize(); i++)
	{
		if (fetch_buffer->getEntrySize(i) > 0)
		{
			for (auto it = fetch_buffer->begin(i),
					e = fetch_buffer->end(i);
					it != e;
					++it)
			{
				// Get uop
				Uop *uop = it->get();
				assert(uop);

				// Skip all uops that have not yet completed the fetch
				if (timing->getCycle() < uop->fetch_ready)
					break;

				// Trace
				Timing::trace << misc::fmt("si.inst "
						"id=%lld "
						"cu=%d "
						"wf=%d "
						"uop_id=%lld "
						"stg=\"s\"\n",
						uop->getIdInComputeUnit(),
						index,
						uop->getWavefront()->getId(),
						uop->getIdInWavefront());
			}
		}
	}
}


void ComputeUnit::Run()
{
	// Return if no work groups are mapped to this compute unit
	if (!work_groups.size())
		return;
	
	// Save timing simulator
	timing = Timing::getInstance();

	// Issue buffer chosen to issue this cycle
	int active_issue_buffer = timing->getCycle() % num_wavefront_pools;
	assert(active_issue_buffer >= 0 && active_issue_buffer < num_wavefront_pools);

	// SIMDs
	for (auto &simd_unit : simd_units)
		simd_unit->Run();

	// Vector memory
	vector_memory_unit.Run();

	// LDS unit
	lds_unit.Run();

	// Scalar unit
	scalar_unit.Run();

	// Branch unit
	branch_unit.Run();

	// Reset issue flag
	fetch_buffers[active_issue_buffer]->ResetExecutionUnitIssueFlag();

	if (!timing->IsHintFileEmpty())
		speculation_fetch_buffers[active_issue_buffer]->
				ResetExecutionUnitIssueFlag();

	// Issue from the active issue buffer
	Issue(fetch_buffers[active_issue_buffer].get(),
				wavefront_pools[active_issue_buffer].get());

	// Update visualization in non-active issue buffers
	for (int i = 0; i < (int) simd_units.size(); i++)
	{
		if (i != active_issue_buffer)
		{
			UpdateFetchVisualization(fetch_buffers[i].get());
		}
	}

	// Fetch
	for (int i = 0; i < num_wavefront_pools; i++)
		Fetch(fetch_buffers[i].get(), wavefront_pools[i].get());
}


} // Namespace SI

