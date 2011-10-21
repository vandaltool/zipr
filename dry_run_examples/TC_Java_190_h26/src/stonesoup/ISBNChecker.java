// © 2011 The MITRE Corporation. ALL RIGHTS RESERVED

package stonesoup;

/**
 * This class contains methods for checking the validity of ISBN numbers
 *
 * @author MLAH
 */
public class ISBNChecker {
	
	static byte stringBytes[] = new byte[25];//Developer felt there was no reason to have more than 25 characters...
	
	public static void passMeValue(char input, int whereAmI){//STONESOUP:signed_int //STONESOUP:CROSSOVER_POINT if crash = true
		byte overflowMe = (byte)(104 + whereAmI); // STONESOUP:TRIGGER_POINT
		if(overflowMe < 0){
			System.out.println("Overflow occurred");
			System.exit(0);
		}
		stringBytes[whereAmI] = (byte) input;
	}

    /**
     * This method checks the input String and returns whether or not it is a
     *  valid ISBN-10 or ISBN-13 number
     *
     * Reference and formulas from:
     *  http://en.wikipedia.org/wiki/International_Standard_Book_Number#ISBN-10
     *  http://en.wikipedia.org/wiki/International_Standard_Book_Number#ISBN-13
     * 
     * Example ISBN-13: 978-0-306-40615-7
     * Example ISBN-10: 0-306-40615-2
     *
     * @param isbn the input ISBN number to check, as a String
     * @return whether the input String is a valid ISBN number
     */
    public static boolean isValidISBN (String isbn) {
    	
    	char temp[] = isbn.toCharArray();
    	int u = 1;
    	for(int j=(u+50)*2/100-1; j<temp.length; j++){ //STONESOUP:loop_complexity_initialization
    		passMeValue(temp[j], j); //STONESOUP:INTERACTION_POINT //STONESOUP:pass_by_value
    	}

        // Accept and remove spaces and dashes from input
        isbn = isbn.replaceAll("[ -]", "");

        if (isbn.length() == 13) {
            // Validate ISBN-13 string

            // ISBN-13 number must be all digits
            try {
                Long.parseLong(isbn);
            } catch (NumberFormatException e) {
                return false;
            }

            // Convert String to ints for calculation of check digit
            int[] isbnInts = new int[13];
            for (int i=0; i<isbn.length(); i++) {
                isbnInts[i] = Character.getNumericValue(isbn.charAt(i));
            }

            // Validate check digit
            if (isbnInts[12] == (10 - ((isbnInts[0]+3*isbnInts[1]+isbnInts[2]+3*isbnInts[3]+isbnInts[4]+3*isbnInts[5]+isbnInts[6]+3*isbnInts[7]+isbnInts[8]+3*isbnInts[9]+isbnInts[10]+3*isbnInts[11]) % 10)) % 10) {
                return true;
            } else {
                return false;
            }

        } else if (isbn.length() == 10) {
            // Validate ISBN-10 string

            // ISBN-10 number must be 9 digits followed by either another digit or an 'X'
            try {
                Integer.parseInt(isbn.substring(0, 9));
                if (!(Character.isDigit(isbn.charAt(9)) || isbn.charAt(9) == 'X' || isbn.charAt(9) == 'x'))
                    throw new NumberFormatException("Last character of ISBN-10 number is not a digit or an 'X'");
            } catch (NumberFormatException e) {
                return false;
            }

            // Convert String to ints for calculation of check digit
            int[] isbnInts = new int[10];
            for (int i=0; i<isbn.length(); i++) {
                if (i == 9 && (isbn.charAt(9) == 'X' || isbn.charAt(9) == 'x')) {
                    isbnInts[i] = 10;
                } else {
                    isbnInts[i] = Character.getNumericValue(isbn.charAt(i));
                }
            }

            // Validate check digit
            if (isbnInts[9] == (isbnInts[0]+2*isbnInts[1]+3*isbnInts[2]+4*isbnInts[3]+5*isbnInts[4]+6*isbnInts[5]+7*isbnInts[6]+8*isbnInts[7]+9*isbnInts[8]) % 11) {
                return true;
            } else {
                return false;
            }
        } else {
            // isbn is not 10 or 13 characters long
            return false;
        }
    }
}