use pinocchio::{error::ProgramError, AccountView, Address, ProgramResult};

use crate::{
    error::VaultError,
    state::{VaultState, VAULT_SEED},
    utils::{impl_len, impl_load},
};

pub mod close;
pub mod deposit;
pub mod init;
pub mod withdraw;

#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct AmountInstructionData {
    pub amount: u64,
}

impl_len!(AmountInstructionData);
impl_load!(AmountInstructionData);

fn check_vault<'a>(
    program_id: &Address,
    authority: &AccountView,
    vault: &'a AccountView,
) -> Result<&'a VaultState, ProgramError> {
    if !authority.is_signer() {
        return Err(ProgramError::MissingRequiredSignature);
    }
    if !vault.owned_by(program_id) {
        return Err(ProgramError::InvalidAccountOwner);
    }

    let vault_state = VaultState::load(unsafe { vault.borrow_unchecked() })?;

    if vault_state.authority != *authority.address() {
        return Err(VaultError::Unauthorized.into());
    }

    let seeds: [&[u8]; 2] = [VAULT_SEED, vault_state.authority.as_ref()];
    let expected_vault = Address::derive_address(&seeds, Some(vault_state.bump), program_id);
    if *vault.address() != expected_vault {
        return Err(VaultError::InvalidVault.into());
    }

    Ok(vault_state)
}

fn move_lamports(from: &mut AccountView, to: &mut AccountView, amount: u64) -> ProgramResult {
    let to_lamports = to
        .lamports()
        .checked_add(amount)
        .ok_or(ProgramError::ArithmeticOverflow)?;
    let from_lamports = from
        .lamports()
        .checked_sub(amount)
        .ok_or(ProgramError::InsufficientFunds)?;

    from.set_lamports(from_lamports);
    to.set_lamports(to_lamports);
    Ok(())
}
