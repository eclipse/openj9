%macro template_start 1
global %1
global %1 %+ Size
%1:
%endmacro

%macro template_end 1
%1 %+ _end:
%1 %+ Size: dw %1 %+ _end - %1
%endmacro