                                       -- Chapter 17 - Program 2
with Ada.Text_IO, Ada.Integer_Text_IO;
use Ada.Text_IO, Ada.Integer_Text_IO;

procedure Except2 is

   procedure Divide_Loop(Index : in     INTEGER) is
   Divide_Result : INTEGER;
   begin
      Put("Index is");
      Put(Index, 3);
      Put(" and the answer is");
      Divide_Result := 25 / (4 - Index);
      Put(Divide_Result, 4);
      New_Line;
   exception
      when Constraint_Error => Put(" Divide by zero error");
                               Put_Line(" in loop 1.");
   end Divide_Loop;

   procedure New_Divide_Loop(Index : in     INTEGER) is
      My_Own_Exception : exception;
      Divide_Result : INTEGER;
   begin
      Put("Index is");
      Put(Index, 3);
      Put(" and the answer is");
      if Index = 4 then
         raise My_Own_Exception;
      end if;
      Divide_Result := 25 / (4 - Index);
      Put(Divide_Result, 4);
      New_Line;
   exception
      when My_Own_Exception => Put(" Divide by zero error");
                               Put_Line(" in loop 3.");
      when Constraint_Error => Put_Line("This shouldn't happen.");
                               Put_Line("But is included anyway.");
      when others => Put_Line("Some other exception found.");
   end New_Divide_Loop;

begin
   Put_Line("Begin program here.");
   for Count in 1..6 loop              -- begin loop number 1
      Divide_Loop(Count);
   end loop;
   Put_Line("End of first loop.");

   for Count in 1..6 loop              -- begin loop number 2
      declare
         Divide_Result : INTEGER;
      begin
         Put("Count is");
         Put(Count, 3);
         Put(" and the answer is");
         Divide_Result := 25 / (4 - Count);
         Put(Divide_Result, 4);
         New_Line;
      exception
         when Constraint_Error => Put(" Divide by zero error");
                                  Put_Line(" in loop 2.");
      end;
   end loop;
   Put_Line("End of second loop.");

   for Count in 1..6 loop              -- begin loop number 3
      New_Divide_Loop(Count);
   end loop;
   Put_Line("End of program.");

end Except2;




-- Result of Execution

-- Begin program here.
-- Index is  1 and the answer is   8
-- Index is  2 and the answer is  12
-- Index is  3 and the answer is  25
-- Index is  4 and the answer is Divide by zero error in loop 1.
-- Index is  5 and the answer is -25
-- Index is  6 and the answer is -12
-- End of first loop.
-- Count is  1 and the answer is   8
-- Count is  2 and the answer is  12
-- Count is  3 and the answer is  25
-- Count is  4 and the answer is Divide by zero error in loop 2.
-- Count is  5 and the answer is -25
-- Count is  6 and the answer is -12
-- End of second loop.
-- Index is  1 and the answer is   8
-- Index is  2 and the answer is  12
-- Index is  3 and the answer is  25
-- Index is  4 and the answer is Divide by zero error in loop 3.
-- Index is  5 and the answer is -25
-- Index is  6 and the answer is -12
-- End of program.

