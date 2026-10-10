use pinocchio::{error::ProgramError, AccountView, Address, ProgramResult};

use super::{check_vault, move_lamports};

pub fn process_close_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    _instruction_data: &[u8],
) -> ProgramResult {
    let [authority, vault, ..] = accounts else {
        return Err(ProgramError::NotEnoughAccountKeys);
    };

    check_vault(program_id, authority, vault)?;

    let lamports = vault.lamports();
    move_lamports(vault, authority, lamports)?;

    vault.close()
}
