use pinocchio::Address;

use crate::utils::{impl_len, impl_load};

pub const VAULT_SEED: &[u8] = b"vault";

#[repr(C)]
#[derive(Clone, Copy)]
pub struct VaultState {
    pub authority: Address,
    pub bump: u8,
    pub _padding: [u8; 7],
}

impl_len!(VaultState);
impl_load!(VaultState);
