use pinocchio::{
    cpi::{Seed, Signer},
    error::ProgramError,
    sysvars::{rent::Rent, Sysvar},
    AccountView, Address, ProgramResult,
};
use pinocchio_system::instructions::CreateAccount;

use crate::{
    state::{VaultState, VAULT_SEED},
    utils::{impl_len, impl_load},
};

#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct InitInstructionData {
    pub bump: u8,
}

impl_len!(InitInstructionData);
impl_load!(InitInstructionData);

pub fn process_init_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    instruction_data: &[u8],
) -> ProgramResult {
    let [authority, vault, system_program, ..] = accounts else {
        return Err(ProgramError::NotEnoughAccountKeys);
    };

    if !authority.is_signer() {
        return Err(ProgramError::MissingRequiredSignature);
    }
    if *system_program.address() != pinocchio_system::ID {
        return Err(ProgramError::IncorrectProgramId);
    }

    let data = InitInstructionData::load(instruction_data)?;
    let bump = [data.bump];
    let seeds = [
        Seed::from(VAULT_SEED),
        Seed::from(authority.address().as_ref()),
        Seed::from(&bump),
    ];

    CreateAccount {
        from: authority,
        to: vault,
        space: VaultState::LEN as u64,
        owner: program_id,
        lamports: Rent::get()?.minimum_balance_unchecked(VaultState::LEN),
    }
    .invoke_signed(&[Signer::from(&seeds[..])])?;

    let vault_state = VaultState::load_mut(unsafe { vault.borrow_unchecked_mut() })?;
    vault_state.authority = *authority.address();
    vault_state.bump = data.bump;

    Ok(())
}
