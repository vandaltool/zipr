                                       -- Chapter 17 - Program 6

with Ada.Text_IO, Ada.Integer_Text_IO, Ada.Exceptions;
use Ada.Text_IO, Ada.Integer_Text_IO;

procedure Occur is

   Except_ID : Ada.Exceptions.EXCEPTION_OCCURRENCE;

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
      when Except_ID: Constraint_Error => 
          Put_Line(" Divide by zero error.");
          New_Line;
          Put_Line("  Exception name:");
          Put_Line(Ada.Exceptions.Exception_Name(Except_ID));
          New_Line;
          Put_Line("  Exception information:");
          Put_Line(Ada.Exceptions.Exception_Information(Except_ID));
          New_Line;

      when Except_ID: Storage_Error =>
          Put_Line(" Storage error detected.");
          Put_Line(Ada.Exceptions.Exception_Information(Except_ID));
          New_Line;

      when Except_ID: others =>
          Put_Line(" Unknown exception detected.");
          Put_Line(Ada.Exceptions.Exception_Information(Except_ID));
          New_Line;

   end Divide_Loop;

begin
   Put_Line("Begin program here.");
   Divide_Loop;
   Put_Line("End of program.");
end Occur;




-- Result of Execution

-- Begin program here.
-- Index is  1 and the answer is  8
-- Index is  2 and the answer is 12
-- Index is  3 and the answer is 25
-- Index is  4 and the answer is Divide by zero error.
--
--   Exception name:
-- Constraint_Error
--
--   Exception message:
-- Constraint_Error (divide by zero)
--
-- End of program.

