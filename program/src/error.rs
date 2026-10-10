use pinocchio::error::ProgramError;

#[repr(u32)]
pub enum VaultError {
    InsufficientBalance = 1,
    InvalidVault = 2,
    Unauthorized = 3,
}

impl From<VaultError> for ProgramError {
    fn from(e: VaultError) -> Self {
        ProgramError::Custom(e as u32)
    }
}
