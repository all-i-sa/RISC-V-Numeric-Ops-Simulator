# RISC-V-Numeric-Ops-Simulator
___
## Simulator Organization
- LSB
### Internal Flag Conventions for Debugging
| Flag |         Name         |            Purpose            |
|:----:|:--------------------:|:-----------------------------:|
|  Z   |         Zero         |          Result == 0          |
|  N   |       Negative       |      Sign bit (MSB) == 1      |
|  C   |        Carry         |  Add/sub overflow (unsigned)  |
|  O   |       Overflow       |   Add/sub overflow (signed)   |
|  LT  |  Less than (signed)  |     if op1 < op2 (signed)     |
| LTU  | Less than (unsigned) |    if op1 < op2 (unsigned)    |
|  EQ  |        Equals        |          op1 IS op2           |
|  ME  |     Memory error     | access fault/misaligned traps |
