/************************************************************************
**
** Copyright (C) The MITRE Corporation 2011
**
**  Author: Mark Troutt
**  Date: 6/13/2011
**
**  Spreadsheet Rev #: 3
**  CWE #: 190
**  Spreadsheet Variant: 26	3	stdin_terminal	3		signed_int	1		loop_complexity_initialization	2		pass_by_value	4
**
**  (x means yes, - means no)
** Tested in MS Windows XP 32bit    		 x
** Tested in MS Windows 7    64bit    		 -
** Tested in RH Linux 32bit                  -
** Tested in RH Linux 64bit                  -
**
**  Revision History
**  6/13/11      Created
**
************************************************************************/

package stonesoup;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * ISBNChecker Main
 * This command line tool calls the ISBNChecker.isValidISBN() method against the
 *  first command line argument.
 *
 * @author MLAH
 */
public class Main {

    public static void main(String[] args) {

    	// Help message
        if (args.length >= 1 && (args[0].equals("-h") || args[0].equals("--h") || args[0].equals("help"))) {
            System.out.println("Usage: java -jar ISBNChecker.jar [isbn_number]");
            System.out.println("  [isbn_number] may be any valid ISBN-10 or ISBN-13 number");
            System.exit(1);
        }
        else if(args.length == 0){
        	try{
        	    BufferedReader bufferRead = new BufferedReader(new InputStreamReader(System.in));
        	    String isbn = bufferRead.readLine(); //STONESOUP:STDIN_terminal
         
        	    System.out.println(ISBNChecker.isValidISBN(isbn));
        	}
        	catch(IOException e)
        	{
        		e.printStackTrace();
        	}
        	
        }
        else if(args.length != 1){
        	System.out.println("Usage: java -jar ISBNChecker.jar [isbn_number]");
            System.out.println("  [isbn_number] may be any valid ISBN-10 or ISBN-13 number");
            System.exit(1);
        }
        else{
        	System.out.println(ISBNChecker.isValidISBN(args[0]));
        }
    }
}
