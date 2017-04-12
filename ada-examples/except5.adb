with Ada.Text_IO, Stuff;
use Ada.Text_IO, Stuff;

procedure Except5 is
   Index : INTEGER := 5;
   Add_Error : exception renames Stuff.Funny_Add_Error;
begin
   Add_One(Index);
   Index := Subtract_One(Index);
exception
   when Numeric_Error | Constraint_Error =>
             Put_Line("Numeric error or constraint error raised.");
   when Add_Error =>
             Put_Line("Addition error detected");
   when others =>
             Put_Line("An unknown exception raised");
             raise;      -- Raise it again for the operating system
end Except5;



