/*
 * test input for the base test program
   */

import java.io.*;

public class tc190001001 {

	/**
	 * @param args
	 */

	public static void main(String[] args) {

		int i = 0;

		// Obtain input from STDIN.
		
		try {
			InputStreamReader isr = new InputStreamReader(System.in);
			BufferedReader stdin = new BufferedReader(isr);
			System.out.print("Enter an integer: ");
			String str = stdin.readLine();
			i = Integer.parseInt(str);
		}
		catch (IOException e) {
			System.err.println("IO error");
			System.exit(1);
		}
		catch (NumberFormatException e) {
			System.err.println("Argument must be an integer");
			System.exit(1);
		}

		// The tainted input is used to affect the looping condition.
	    
		for ( int x = 0; x < 10; x++ ) {
			System.out.println(x);
			if ( x == 5 ) {
				x = x + i;
			}
		}

	}

}