MOVC R1,#10
MOVC R5,#45
MOVC R10,#230
ADD R6,R10,R5
MOVC R13,#20
SUB R12,R10,R6
SUBL R12,R6,#10
STORE R6,R1,#22
LOAD R14,R13,#12
MOVC R0,#12
MOVC R8,#22
STORE R13,R13,#10
MUL R10,R0,R8
AND R1,R10,R14
HALT 
EXOR R10,R12,R13
LOAD R6,R13,#12
MOVC R12,#10
AND R8,R6,R5
SUB R6,R0,R14