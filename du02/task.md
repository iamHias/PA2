# Datum
## Úkol 
- Třída Datum
- Pravidla Gregoriánského kalendáře
## Třída
CDate
- constructor (y,m,d) + validation
- 
- 

## Tip & Trick
### Operator Chain Reaction
> Relational operators ( C++20 )
```c++
    auto operator <=> (const CDate& other) const = default;
```
Compiler will generate 6 different comparision operators for me based on the order of the member var.
> Arithmetic operators
It is best to implement += and -= first, and then the operator + will just use this method and keep your code not repeated.
> Prefix and postfix
Implement prefix then reusing that for postfix
