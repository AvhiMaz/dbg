use pinocchio::{AccountView, Address, ProgramResult};

pub fn process_init_instruction(
    program_id: &Address,
    accounts: &mut [AccountView],
    instruction_data: &[u8],
) -> ProgramResult {
    Ok(())
}
