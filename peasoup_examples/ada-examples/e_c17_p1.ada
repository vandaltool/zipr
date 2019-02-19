                                       -- Chapter 17 - Program 1
with Ada.Text_IO, Ada.Integer_Text_IO;
use Ada.Text_IO, Ada.Integer_Text_IO;

procedure Except1 is

   procedure Divide_Loop is
   Divide_Result : INTEGER;
   begin
      for Index in 1..8 loop
         Put("Index is");
         Put(Index, 3);
         Put(" and the answer is");
         Divide_Result := 25 / (4 - Index);
         Put(Divide_Result, 3);
         New_Line;
      end loop;
   exception
      when Constraint_Error => Put_Line(" Divide by zero error.");
   end Divide_Loop;

begin
   Put_Line("Begin program here.");
   Divide_Loop;
   Put_Line("End of program.");
end Except1;




-- Result of Execution

-- Begin program here.
-- Index is  1 and the answer is  8
-- Index is  2 and the answer is 12
-- Index is  3 and the answer is 25
-- Index is  4 and the answer is Divide by zero error.
-- End of program.

