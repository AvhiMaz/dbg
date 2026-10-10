use pinocchio::{
    error::ProgramError,
    sysvars::{rent::Rent, Sysvar},
    AccountView, Address, ProgramResult,
};

use super::{check_vault, move_lamports, AmountInstructionData};
use crate::{error::VaultError, state::VaultState};

pub fn process_withdraw_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    instruction_data: &[u8],
) -> ProgramResult {
    let [authority, vault, ..] = accounts else {
        return Err(ProgramError::NotEnoughAccountKeys);
    };

    check_vault(program_id, authority, vault)?;

    let amount = AmountInstructionData::load(instruction_data)?.amount;
    if amount == 0 {
        return Err(ProgramError::InvalidInstructionData);
    }

    let rent_minimum = Rent::get()?.minimum_balance_unchecked(VaultState::LEN);
    let available = vault.lamports().saturating_sub(rent_minimum);
    if amount > available {
        return Err(VaultError::InsufficientBalance.into());
    }

    move_lamports(vault, authority, amount)
}
