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

#include <arch/southern-islands/emulator/Wavefront.h>
#include <arch/southern-islands/emulator/WorkGroup.h>

#include "ComputeUnit.h"
#include "Uop.h"
#include "WavefrontPool.h"


namespace SI
{

long long Uop::id_counter = 0;


Uop::Uop(Wavefront *wavefront, WavefrontPoolEntry *wavefront_pool_entry,
		long long cycle_created,
		WorkGroup *work_group,
		int wavefront_pool_id) :
		wavefront(wavefront),
		wavefront_pool_entry(wavefront_pool_entry),
		work_group(work_group),
		wavefront_pool_id(wavefront_pool_id)
{
	// Assign unique identifier
	id = ++id_counter;
	id_in_wavefront = wavefront->getUopId();
	compute_unit = wavefront_pool_entry->getWavefrontPool()->getComputeUnit();
	id_in_compute_unit = compute_unit->getUopId();
	
	// Allocate room for the work-item info structures
	work_item_info_list.resize(WorkGroup::WavefrontSize);

	// Initialize source scalar register index
	for (int i = 0; i < 4; i++)
		source_scalar_register_index[i] = -1;

	// Initialize destination scalar register index
	for (int i = 0; i < 16; i++)
		destination_scalar_register_index[i] = -1;

	// Initialize source vector register index
	for (int i = 0; i < 6; i++)
		source_vector_register_index[i] = -1;

	// Initialize destination vector register index
	for (int i = 0; i < 4; i++)
		destination_vector_register_index[i] = -1;
}


void Uop::setInstructionRegistersIndex()
{
	switch(this->instruction.getFormat())
	{
	case (Instruction::FormatInvalid):
	{
		break;
	}

	case (Instruction::FormatSOP2):
	{
		Instruction::BytesSOP2 format = instruction.getBytes()->sop2;

		if (instruction.getOpcode() == Instruction::Opcode_S_ADD_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_ADD_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_SUB_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_MIN_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_MAX_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_MAX_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CSELECT_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setSourceScalarRegisterIndex(2, Instruction::RegisterScc);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_AND_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_AND_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc1);
			this->setSourceScalarRegisterIndex(3, format.ssrc1 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_OR_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_OR_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc1);
			this->setSourceScalarRegisterIndex(3, format.ssrc1 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_XOR_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc1);
			this->setSourceScalarRegisterIndex(3, format.ssrc1 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_ANDN2_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc1);
			this->setSourceScalarRegisterIndex(3, format.ssrc1 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_NAND_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc1);
			this->setSourceScalarRegisterIndex(3, format.ssrc1 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LSHL_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LSHR_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if(instruction.getOpcode() == Instruction::Opcode_S_ASHR_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if(instruction.getOpcode() == Instruction::Opcode_S_MUL_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if(instruction.getOpcode() == Instruction::Opcode_S_BFE_I32)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		break;
	}

	case (Instruction::FormatSOPK):
	{
		Instruction::BytesSOPK format = instruction.getBytes()->sopk;
		if (instruction.getOpcode() == Instruction::Opcode_S_MOVK_I32)
		{
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMPK_LE_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_ADDK_I32)
		{
			this->setSourceScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_MULK_I32)
		{
			this->setSourceScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
							Instruction::RegisterScc);
			break;
		}
		break;
	}

	case (Instruction::FormatSOP1):
	{
		Instruction::BytesSOP1 format = instruction.getBytes()->sop1;
		if (instruction.getOpcode() == Instruction::Opcode_S_MOV_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_MOV_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_NOT_B32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1,
					Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_WQM_B64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_SWAPPC_B64)
		{
			this->setSourceScalarRegisterIndex(0, format.ssrc0);
			this->setSourceScalarRegisterIndex(1, format.ssrc0 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			break;
		}
		else if (instruction.getOpcode() ==
						Instruction::Opcode_S_AND_SAVEEXEC_B64)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterExec);
			this->setSourceScalarRegisterIndex(1, Instruction::RegisterExec + 1);
			this->setSourceScalarRegisterIndex(2, format.ssrc0);
			this->setSourceScalarRegisterIndex(3, format.ssrc0 + 1);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2,
						Instruction::RegisterExec);
			this->setDestinationScalarRegisterIndex(3,
						Instruction::RegisterExec + 1);
			this->setDestinationScalarRegisterIndex(4,
						Instruction::RegisterScc);
			break;
		}
		break;
	}

	case (Instruction::FormatSOPC):
	{
		Instruction::BytesSOPC format = instruction.getBytes()->sopc;
		if (instruction.getOpcode() == Instruction::Opcode_S_CMP_EQ_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMP_GT_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if(instruction.getOpcode() == Instruction::Opcode_S_CMP_GE_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if(instruction.getOpcode() == Instruction::Opcode_S_CMP_LT_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMP_LE_I32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMP_GT_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMP_GE_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CMP_LE_U32)
		{
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(0, format.ssrc0);
			if (format.ssrc0 != 0xFF)
				this->setSourceScalarRegisterIndex(1, format.ssrc1);
			this->setDestinationScalarRegisterIndex(0,
							Instruction::RegisterScc);
			break;
		}
		break;
	}

	case (Instruction::FormatSOPP):
	{
		if (instruction.getOpcode() == Instruction::Opcode_S_ENDPGM)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_BRANCH)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_SCC0)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_SCC1)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterScc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_VCCZ)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterVccz);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_VCCNZ)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterVccz);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_EXECZ)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterExec);
			this->setSourceScalarRegisterIndex(1, Instruction::RegisterExecz);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_CBRANCH_EXECNZ)
		{
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterExecz);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_BARRIER)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_WAITCNT)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_PHI)
		{
			break;
		}
		break;
	}

	case (Instruction::FormatSMRD):
	{
		Instruction::BytesSMRD format = instruction.getBytes()->smrd;
		if (instruction.getOpcode() == Instruction::Opcode_S_BUFFER_LOAD_DWORD)
		{
			if (format.imm == 0)
				this->setSourceScalarRegisterIndex(0,format.offset);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_S_BUFFER_LOAD_DWORDX2)
		{
			if (format.imm == 0)
				this->setSourceScalarRegisterIndex(0,format.offset);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_S_BUFFER_LOAD_DWORDX4)
		{
			if (format.imm == 0)
				this->setSourceScalarRegisterIndex(0,format.offset);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			this->setDestinationScalarRegisterIndex(1, format.sdst + 1);
			this->setDestinationScalarRegisterIndex(2, format.sdst + 2);
			this->setDestinationScalarRegisterIndex(3, format.sdst + 3);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_S_BUFFER_LOAD_DWORDX8)
		{
			if (format.imm == 0)
				this->setSourceScalarRegisterIndex(0,format.offset);
			for (int i = 0; i < 8; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_S_BUFFER_LOAD_DWORDX16)
		{
			if (format.imm == 0)
				this->setSourceScalarRegisterIndex(0,format.offset);
			for (int i = 0; i < 16; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LOAD_DWORDX2)
		{
			for (int i = 0; i < 2; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LOAD_DWORDX4)
		{
			for (int i = 0; i < 4; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LOAD_DWORDX8)
		{
			for (int i = 0; i < 8; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_S_LOAD_DWORDX16)
		{
			for (int i = 0; i < 16; i++)
				this->setDestinationScalarRegisterIndex(i, format.sdst + i);
			break;
		}
		break;
	}

	case (Instruction::FormatVOP2):
	{
		Instruction::BytesVOP2 format = instruction.getBytes()->vop2;
		if (instruction.getOpcode() == Instruction::Opcode_V_CNDMASK_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			this->setSourceScalarRegisterIndex(0, Instruction::RegisterVcc);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ADD_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SUB_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SUBREV_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_MAC_LEGACY_F32)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_MUL_LEGACY_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_I32_I24)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MIN_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MIN_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MIN_U32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX_U32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LSHRREV_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ASHRREV_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LSHL_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LSHLREV_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_AND_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_OR_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_XOR_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_BFM_B32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAC_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			this->setSourceVectorRegisterIndex(1, format.vdst);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(2, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MADMK_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ADD_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SUB_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SUBREV_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CVT_PKRTZ_F16_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		break;
	}

	case (Instruction::FormatVOP1):
	{
		Instruction::BytesVOP1 format = instruction.getBytes()->vop1;
		if (instruction.getOpcode() == Instruction::Opcode_V_NOP)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MOV_B32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_READFIRSTLANE_B32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_I32_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F64_I32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F32_I32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F32_U32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_U32_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_I32_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F32_F64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F64_F32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CVT_F64_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_TRUNC_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_FLOOR_F32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LOG_F32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_RCP_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_RCP_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_RSQ_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SQRT_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_SIN_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_COS_F32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_NOT_B32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_FFBH_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_FRACT_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MOVRELD_B32)
		{
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, Instruction::RegisterM0);
			Instruction::Register m0;
			m0.as_uint = this->wavefront->getSregUint(Instruction::RegisterM0);
			this->setDestinationVectorRegisterIndex(0, format.vdst +
						m0.as_uint);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MOVRELS_B32)
		{
			this->setSourceRegisterIndex(0, Instruction::RegisterM0);
			Instruction::Register m0;
			m0.as_uint = this->wavefront->getSregUint(Instruction::RegisterM0);
			this->setSourceRegisterIndex(1, format.src0 + m0.as_uint);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		break;
	}

	case (Instruction::FormatVOPC):
	{
		Instruction::BytesVOPC format = instruction.getBytes()->vopc;
		if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LT_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GT_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GE_F32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NGT_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NEQ_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LT_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_EQ_F64)
		{
			break;
		}

		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LE_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GT_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NGE_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NEQ_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NLT_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LT_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_EQ_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LE_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GT_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NE_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GE_I32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_CLASS_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LT_U32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_EQ_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_LE_U32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GT_U32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc1);
			if (format.src0 != 0xFF)
				this->setSourceRegisterIndex(1, format.src0);
			this->setDestinationScalarRegisterIndex(0,
						Instruction::RegisterVcc);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_NE_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_CMP_GE_U32)
		{
			break;
		}
		break;
	}

	case (Instruction::FormatVOP3a):
	{
		Instruction::BytesVOP3A format = instruction.getBytes()->vop3a;
		if (instruction.getOpcode() == Instruction::Opcode_V_CNDMASK_B32_VOP3a)
		{
			this->setSourceScalarRegisterIndex(0, format.src2);
			this->setSourceRegisterIndex(1, format.src0);
			this->setSourceRegisterIndex(2, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ADD_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_SUBREV_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_MUL_LEGACY_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_MUL_I32_I24_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAD_F32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAD_U32_U24)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_BFE_U32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_BFE_I32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_BFI_B32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_FMA_F32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_FMA_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ALIGNBIT_B32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_DIV_FIXUP_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LSHL_B64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setSourceRegisterIndex(2, format.src1);
			this->setSourceRegisterIndex(3, format.src1 + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MIN_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_LO_U32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_DIV_FMAS_F64)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_TRIG_PREOP_F64)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_HI_U32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_LO_I32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_FRACT_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LT_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_EQ_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LE_F32_VOP3a)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_GT_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_NLE_F32_VOP3a)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_NEQ_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_NLT_F32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_OP16_F64_VOP3a)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LT_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_EQ_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LE_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_GT_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_NE_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_GE_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMPX_EQ_I32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			this->setDestinationScalarRegisterIndex(1,
						Instruction::RegisterExec);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_CLASS_F64_VOP3a)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LT_U32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LE_U32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_GT_U32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LG_U32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_GE_U32_VOP3a)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setDestinationScalarRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_CMP_LT_U64_VOP3a)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MAX3_I32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MED3_I32)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src1);
			this->setSourceRegisterIndex(2, format.src2);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LSHR_B64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setSourceRegisterIndex(2, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ASHR_I64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setSourceRegisterIndex(2, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_ADD_F64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setSourceRegisterIndex(2, format.src1);
			this->setSourceRegisterIndex(3, format.src1 + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_MUL_F64)
		{
			this->setSourceRegisterIndex(0, format.src0);
			this->setSourceRegisterIndex(1, format.src0 + 1);
			this->setSourceRegisterIndex(2, format.src1);
			this->setSourceRegisterIndex(3, format.src1 + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_LDEXP_F64)
		{
			break;
		}
		break;
	}

	case (Instruction::FormatVOP3b):
	{
		Instruction::BytesVOP3B format = instruction.getBytes()->vop3b;
		if (instruction.getOpcode() == Instruction::Opcode_V_ADDC_U32_VOP3b)
		{
			this->setSourceScalarRegisterIndex(0, format.src2);
			this->setSourceRegisterIndex(1, format.src0);
			this->setSourceRegisterIndex(2, format.src1);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationScalarRegisterIndex(0, format.sdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_DIV_SCALE_F64)
		{
			break;
		}
		break;
	}

	case (Instruction::FormatVINTRP):
	{
		Instruction::BytesVINTRP format = instruction.getBytes()->vintrp;
		if (instruction.getOpcode() == Instruction::Opcode_V_INTERP_P1_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc);
			this->setSourceRegisterIndex(1, Instruction::RegisterM0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_V_INTERP_P2_F32)
		{
			this->setSourceVectorRegisterIndex(0, format.vsrc);
			this->setSourceVectorRegisterIndex(1, format.vdst);
			this->setSourceRegisterIndex(2, Instruction::RegisterM0);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_V_INTERP_MOV_F32)
		{
			break;
		}
		break;
	}

	case (Instruction::FormatDS):
	{
		Instruction::BytesDS format = instruction.getBytes()->ds;
		if (instruction.getOpcode() == Instruction::Opcode_DS_ADD_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_INC_U32)
		{
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_WRITE2_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setSourceVectorRegisterIndex(1, format.data0);
			this->setSourceVectorRegisterIndex(2, format.data1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_WRITE_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setSourceVectorRegisterIndex(1, format.data0);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_WRITE_B8)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setSourceVectorRegisterIndex(1, format.data0);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_WRITE_B16)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setSourceVectorRegisterIndex(1, format.data0);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ2_B32)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			this->setDestinationVectorRegisterIndex(1, format.vdst + 1);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ_I8)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ_U8)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ_I16)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		else if (instruction.getOpcode() == Instruction::Opcode_DS_READ_U16)
		{
			this->setSourceVectorRegisterIndex(0, format.addr);
			this->setDestinationVectorRegisterIndex(0, format.vdst);
			break;
		}
		break;
	}

	case (Instruction::FormatMUBUF):
	{
		Instruction::BytesMUBUF format = instruction.getBytes()->mubuf;
		if (instruction.getOpcode() == Instruction::Opcode_BUFFER_LOAD_SBYTE)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_BUFFER_LOAD_DWORD)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_BUFFER_STORE_BYTE)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_BUFFER_STORE_DWORD)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			this->setSourceVectorRegisterIndex(1, format.vdata);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(2, format.vaddr + 1);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_BUFFER_ATOMIC_ADD)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			this->setSourceVectorRegisterIndex(1, format.vdata);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(2, format.vaddr + 1);
			if (format.glc)
				this->setDestinationVectorRegisterIndex(0, format.vdata);
			break;
		}
		break;
	}

	case (Instruction::FormatMTBUF):
	{
		Instruction::BytesMTBUF format = instruction.getBytes()->mtbuf;
		if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_LOAD_FORMAT_X)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(1, format.vaddr + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_LOAD_FORMAT_XY)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(1, format.vaddr + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			this->setDestinationVectorRegisterIndex(1, format.vdata + 1);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_LOAD_FORMAT_XYZ)
		{
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_LOAD_FORMAT_XYZW)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(1, format.vaddr + 1);
			this->setDestinationVectorRegisterIndex(0, format.vdata);
			this->setDestinationVectorRegisterIndex(1, format.vdata + 1);
			this->setDestinationVectorRegisterIndex(2, format.vdata + 2);
			this->setDestinationVectorRegisterIndex(3, format.vdata + 3);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_STORE_FORMAT_X)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			this->setSourceVectorRegisterIndex(1, format.vdata);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(2, format.vaddr + 1);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_STORE_FORMAT_XY)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			this->setSourceVectorRegisterIndex(1, format.vdata);
			this->setSourceVectorRegisterIndex(2, format.vdata + 1);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(3, format.vaddr + 1);
			break;
		}
		else if (instruction.getOpcode() ==
					Instruction::Opcode_TBUFFER_STORE_FORMAT_XYZW)
		{
			this->setSourceScalarRegisterIndex(0, format.srsrc * 4);
			this->setSourceScalarRegisterIndex(1, format.srsrc * 4 + 1);
			this->setSourceScalarRegisterIndex(2, format.srsrc * 4 + 2);
			this->setSourceScalarRegisterIndex(3, format.srsrc * 4 + 3);
			this->setSourceScalarRegisterIndex(4, format.soffset);
			this->setSourceVectorRegisterIndex(0, format.vaddr);
			this->setSourceVectorRegisterIndex(1, format.vdata);
			this->setSourceVectorRegisterIndex(2, format.vdata + 1);
			this->setSourceVectorRegisterIndex(3, format.vdata + 1);
			this->setSourceVectorRegisterIndex(4, format.vdata + 1);
			if (format.idxen && format.offen)
				this->setSourceVectorRegisterIndex(5, format.vaddr + 1);
			break;
		}
		break;
	}

	case (Instruction::FormatMIMG):
	{
		break;
	}

	case (Instruction::FormatEXP):
	{
		break;
	}

	case (Instruction::FormatCount):
	{
		break;
	}
	}
}

}

