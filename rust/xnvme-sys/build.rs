use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    // Emit link options for linking system dependencies
    system_deps::Config::new().probe()?;

    Ok(())
}
