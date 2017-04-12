                                       -- Chapter 17 - Program 3
with Ada.Text_IO, Ada.Integer_Text_IO;
use Ada.Text_IO, Ada.Integer_Text_IO;

procedure Except3 is

   procedure Divide_By_Zero(Count : INTEGER) is
      Divide_Result : INTEGER;
   begin
      Put("Count is");
      Put(Count, 3);
      Put(" and the answer is");
      Divide_Result := 25 / (Count - 4);
      Put(Divide_Result, 4);
      New_Line;
   exception
      when Constraint_Error => Put_Line(" Divide by zero occurred");
   end Divide_By_Zero;

   procedure Raise_An_Error(Count : INTEGER) is
      My_Own_Error : exception;
      Another_Result : INTEGER;
   begin
      Put("Count is");
      Put(Count, 3);
      Another_Result := 35 / (Count - 6);  -- untested divide by zero
      if Count = 3 then
         raise My_Own_Error;
      end if;
      Put_Line(" and is a legal value");
   exception
      when My_Own_Error => Put_Line(" My own error occurred");
   end Raise_An_Error;

begin
   Put_Line("Begin program here.");
   for Count in 1..7 loop
      Divide_By_Zero(Count);
      Raise_An_Error(Count);
   end loop;
   Put_Line("End of program.");

   exception
      when Constraint_Error => Put(" Constraint error detected at");
                               Put_Line(" the main program level.");
                               Put_Line("Program terminated.");
end Except3;




-- Result of Execution

-- Begin program here.
-- Count is  1 and the answer is  -8
-- Count is  1 and is a legal value
-- Count is  2 and the answer is -12
-- Count is  2 and is a legal value
-- Count is  3 and the answer is -25
-- Count is  3 My own error occurred
-- Count is  4 and the answer is Divide by zero occurred
-- Count is  4 and is a legal value
-- Count is  5 and the answer is  25
-- Count is  5 and is a legal value
-- Count is  6 and the answer is 12
-- Count is  6 Constraint error detected at the main program level.
-- Program terminated.


