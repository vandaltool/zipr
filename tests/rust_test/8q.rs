// Function to print the solution
fn print_solution(board: &[i32]) {
    for i in 0..8 {
        for j in 0..8 {
            print!(" {} ", if board[i] == j { "Q" } else { "." });
        }
        println!();
    }
}

// Function to check if the placement of a queen is safe
fn is_safe(board: &[i32], row: i32, col: i32) -> bool {
    for i in 0..row {
        if board[i as usize] == col || (i - row).abs() == (board[i as usize] - col).abs() {
            return false;
        }
    }
    true
}

// Function to solve the 8 queens problem
fn solve_8_queens(board: &mut [i32], row: i32) -> bool {
    if row == 8 {
        return true;
    }

    for col in 0..8 {
        if is_safe(board, row, col) {
            board[row as usize] = col;

            if solve_8_queens(board, row + 1) {
                return true;
            }

            board[row as usize] = -1;
        }
    }

    false
}

fn main() {
    let mut board = [-1; 8];

    if solve_8_queens(&mut board, 0) {
        println!("Solution found:");
        print_solution(&board);
    } else {
        println!("No solution found.");
    }
}

