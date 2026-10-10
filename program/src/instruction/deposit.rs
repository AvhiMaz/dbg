use pinocchio::{error::ProgramError, AccountView, Address, ProgramResult};
use pinocchio_system::instructions::Transfer;

use super::{check_vault, AmountInstructionData};

pub fn process_deposit_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    instruction_data: &[u8],
) -> ProgramResult {
    let [authority, vault, system_program, ..] = accounts else {
        return Err(ProgramError::NotEnoughAccountKeys);
    };

    if *system_program.address() != pinocchio_system::ID {
        return Err(ProgramError::IncorrectProgramId);
    }
    check_vault(program_id, authority, vault)?;

    let amount = AmountInstructionData::load(instruction_data)?.amount;
    if amount == 0 {
        return Err(ProgramError::InvalidInstructionData);
    }

    Transfer {
        from: authority,
        to: vault,
        lamports: amount,
    }
    .invoke()
}
