{ Test code generation for long backwards branches in the x86 back end }
{ Evin Robertson <evin@users.sourceforge.net> }

program loop;

Procedure main;
	var g : ShortInt;
begin
	for g := 0 to 10 do begin
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
		write('.'); write('.'); write('.'); write('.');
	end;
	WriteLn('');
end; { main }

begin
main
end.
