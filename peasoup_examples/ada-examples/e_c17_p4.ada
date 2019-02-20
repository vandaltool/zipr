                                       -- Chapter 17 - Program 4
with Ada.Text_IO;
use Ada.Text_IO;

procedure Except4 is

   procedure Try_It is
      VALUE : constant := 8;
      subtype LIMIT_RANGE is INTEGER range 14..33;
      Funny : LIMIT_RANGE := VALUE;
   begin
      Put_Line("We are in the Try_It procedure");
   exception
      when Constraint_Error =>
              Put_Line("Constraint error occurred");
              Put_Line("Detected in procedure Try_It");
   end Try_It;

   procedure Try_To_Fix_It is
   begin
      Put_Line("We are in the Try_To_Fix_It procedure.");
      Try_It;
   exception
      when Constraint_Error =>
              Put_Line("Constraint error occurred");
              Put_Line("Detected in procedure Try_To_Fix_It");
   end Try_To_Fix_It;


begin
   Put_Line("Begin program here.");
   for Count in 1..4 loop
      Put_Line("Ready to call Try_To_Fix_It.");
      Try_To_Fix_It;
      New_Line;
   end loop;
   Put_Line("End of program.");

   exception
      when Constraint_Error =>
              Put     ("Range error detected at the");
              Put_Line(" main program level.");
              Put_Line("Program terminated.");
end Except4;




-- Result of execution

-- Begin program here.
-- Ready to call Try_To_Fix_It.
-- We are in the Try_To_Fix_It procedure.
-- Constraint error occurred
-- Detected in procedure Try_To-Fix_It
--
-- Ready to call Try_To_Fix_It.
-- We are in the Try_To_Fix_It procedure.
-- Constraint error occurred
-- Detected in procedure Try_To-Fix_It
--
-- Ready to call Try_To_Fix_It.
-- We are in the Try_To_Fix_It procedure.
-- Constraint error occurred
-- Detected in procedure Try_To-Fix_It
--
-- Ready to call Try_To_Fix_It.
-- We are in the Try_To_Fix_It procedure.
-- Constraint error occurred
-- Detected in procedure Try_To-Fix_It
--
-- End of program.

