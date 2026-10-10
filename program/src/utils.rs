macro_rules! impl_len {
    ($t: ty) => {
        impl $t {
            #[allow(dead_code)]
            pub const LEN: usize = core::mem::size_of::<Self>();
        }
    };
}

macro_rules! impl_load {
    ($t: ty) => {
        impl $t {
            #[allow(dead_code)]
            pub fn load(data: &[u8]) -> Result<&Self, pinocchio::error::ProgramError> {
                Self::check_castable(data.len(), data.as_ptr())?;
                Ok(unsafe { &*(data.as_ptr() as *const Self) })
            }

            #[allow(dead_code)]
            pub fn load_mut(data: &mut [u8]) -> Result<&mut Self, pinocchio::error::ProgramError> {
                Self::check_castable(data.len(), data.as_ptr())?;
                Ok(unsafe { &mut *(data.as_mut_ptr() as *mut Self) })
            }

            fn check_castable(
                len: usize,
                ptr: *const u8,
            ) -> Result<(), pinocchio::error::ProgramError> {
                if len != core::mem::size_of::<$t>()
                    || !(ptr as usize).is_multiple_of(core::mem::align_of::<$t>())
                {
                    return Err(pinocchio::error::ProgramError::InvalidAccountData);
                }
                Ok(())
            }
        }
    };
}

pub(crate) use impl_len;
pub(crate) use impl_load;
