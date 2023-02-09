use std::env;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::process;

fn main() {
	println!("Welcome to Hangman!");

	let args: Vec<String> = env::args().collect();
	if args.len() != 3 {
		println!("Usage: hangman <word> <guesses_file>");
		process::exit(1);
	}

	let secret_word = &args[1];
	let file_name = &args[2];
	let file = match File::open(file_name) {
		Ok(file) => file,
			Err(e) => {
				println!("Error opening file: {}", e);
				process::exit(1);
			}
	};

	let mut word_progress = vec!['_'; secret_word.len()];
	let mut tries = 5;

	for guess in BufReader::new(file).lines() {
		let guess = match guess {
			Ok(line) => line.trim().to_lowercase(),
				Err(e) => {
					println!("Error reading from file: {}", e);
					process::exit(1);
				}
		};

		if guess.len() == 0 {
			println!("You didn't guess anything, thus you lose.");
			break;
		}
		if guess.len() != 1 {
			continue;
		}

		let mut found = false;
		for (i, c) in secret_word.chars().enumerate() {
			if guess.chars().next().unwrap() == c {
				found = true;
				word_progress[i] = c;
			}
		}

		if !found {
			tries -= 1;
			println!("42/tries = {}.", 42/tries);
			println!("Wrong! You have {} tries left.", tries);
		}

		if !word_progress.contains(&'_') {
			println!("You win! The word was {}.", secret_word);
			break;
		}

		if tries == 0 {
			println!("You lose! The word was {}.", secret_word);
			break;
		}
	}
}

