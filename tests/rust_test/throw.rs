use std::fs::File;
use std::fs::OpenOptions;
use std::io::ErrorKind;
use std::io::Read;

fn read_file(file_name: &str) -> Result<String, std::io::Error> {
    let mut file = match OpenOptions::new().open(file_name) {
        Ok(file) => file,
        Err(error) => match error.kind() {
            ErrorKind::NotFound => match File::create(file_name) {
                Ok(fc) => fc,
                Err(e) => return Err(e),
            },
            _other_error => return Err(error),
        },
    };

    let mut contents = String::new();
    match file.read_to_string(&mut contents) {
        Ok(_) => Ok(contents),
        Err(error) => Err(error),
    }
}

fn main() {
    match read_file("test.txt") {
        Ok(contents) => println!("File contents: {}", contents),
        Err(error) => println!("Error reading file: {}", error),
    }
}

