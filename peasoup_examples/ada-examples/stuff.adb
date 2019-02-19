with Ada.Text_IO;
with Stuff;
use Ada.Text_IO;
use Stuff;

package body Stuff is

   procedure Add_One(Number : in out INTEGER) is
   begin
      Number := Number + 1;
   exception
      when Funny_Add_Error => Put_Line("Funny add error raised");
      when Constraint_Error => Put_Line("Constraint error raised");
   end Add_One;

   function Subtract_One(Number : INTEGER) return INTEGER is
      Funny_Subtract_Error : exception;
   begin
      raise Funny_Subtract_Error;
      return Number - 1;
   exception
      when Funny_Add_Error =>
                         Put_Line("Funny add error raised");
                         raise;
      when Funny_Subtract_Error =>
                         Put_Line("Funny subtract error raised");
                         raise;
   end Subtract_One;

begin
   null;
exception
-- Numeric_Error is obsolete in Ada 95.  It is another name for
--  the exception Constraint_Error.
-- when Numeric_Error =>
--                  Put_Line("Numeric error during elaboration");
   when Constraint_Error =>
                 Put_Line("Constraint_Error during elaboration");
   when Funny_Add_Error =>
                  Put_Line("Funny Add error during elaboration");
-- when Funny_Subtract_Error =>
--           Put_Line("Funny subtract error during elaboration");
end Stuff;

