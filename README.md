# PL0-GUI
A PL0 compiler with GUI implemented with C++ MFC. It is capable of translating PL0 Code into an intermediate language "PCode" and interpreting it. It also shows some of the internal structures: the result of lexical analysis, the compiled Pcode and the stack trace during a run.

# Usage
1. Prepare a input file. The example given computes the factorial of n.
```pascal
var f, n;
procedure fact;
	var ans1;
	begin
		ans1:=n;
		n:= n-1;
		if n < 0 then f := -1;
		if n = 0 then f := 1
		else call fact;
		f:=f*ans1;
		write(f);
	end;
begin
	read(n);
	call fact;
end.
```

2. Select file, compile PCode, manually type the input, and then run.
![eg](https://raw.githubusercontent.com/yzhq97/PL0-GUI/master/snapshot.PNG)
