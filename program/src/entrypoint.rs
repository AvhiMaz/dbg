#![allow(unexpected_cfgs)]

use pinocchio::{entrypoint, error::ProgramError, AccountView, Address, ProgramResult};

use crate::instruction::{
    close::process_close_instruction, deposit::process_deposit_instruction,
    init::process_init_instruction, withdraw::process_withdraw_instruction,
};

entrypoint!(process_instruction);

pub fn process_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    instruction_data: &[u8],
) -> ProgramResult {
    match instruction_data.split_first() {
        Some((0, rest)) => process_init_instruction(program_id, accounts, rest),
        Some((1, rest)) => process_deposit_instruction(program_id, accounts, rest),
        Some((2, rest)) => process_withdraw_instruction(program_id, accounts, rest),
        Some((3, rest)) => process_close_instruction(program_id, accounts, rest),
        _ => Err(ProgramError::InvalidInstructionData),
    }
}
